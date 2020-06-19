#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/tcp.h>

#include "worker.h"

struct worker *workerNew(char *id, struct jk *jk)
{
    struct worker *worker;

    worker = malloc(sizeof(*worker));
    if (!worker) return NULL;
    worker->id = id;
    worker->jk = jk;
    worker->sockfd = -1;
    worker->chocked = 0;
    worker->retries = 0;
    worker->sendpos = 0;
    worker->sendlen = 0;
    worker->recvlen = 0;
    return worker;
}

void workerDestroy(void *worker) { free(worker); }

static int buildHandshake(unsigned char *handshake,
                           const unsigned char *infoHash,
                           const unsigned char *peerId)
{
    int i = 0;

    handshake[i] = 19;
    i += 1;
    memcpy(handshake + i, "BitTorrent protocol", 19);
    i += 19;
    memset(handshake + i, 0, 8);
    i += 8;
    memcpy(handshake + i, infoHash, 20);
    i += 20;
    memcpy(handshake + i, peerId, 20);
    i += 20;

    return i;
}

static void nbOnHandshakeSend(struct evLoop *loop,
        int sockfd, void *data, int mask)
{
    struct worker *worker = (struct worker *)data;

    /* all sent */
    if (worker->sendpos == worker->sendlen) {
        removeFileEventEvLoop(loop, sockfd, mask);
        return;
    }
     
    u64 n = send(worker->sockfd, worker->sendbuf + worker->sendpos,
        worker->sendlen, 0);
    if (n == -1) {
        int err = errno;
        close(sockfd);
        removeFileEventEvLoop(loop, sockfd, mask);
        /* retry */
        if (err == EAGAIN) {
            /* retry limit exceeded. remove the worker */
            if (worker->retries >= WORKER_RETRIES_MAX) {
                worker->retries = 0; 
                mapRemove(worker->jk->workers, worker->id);
                return;
            }
            jkLog(LT_DEBUG, strerror(errno));
            nbWorkerConnect(loop, data);
            return;
        }
        jkLog(LT_ERROR, strerror(errno));
        stopEvLoop(loop);
        return;
    }
    worker->sendpos += n;
}

static void nbOnConnect(struct evLoop *loop,
        int sockfd, void *data, int mask)
{
    struct worker *worker = (struct worker *)data;

    int err = jnetGetSocketErrno(sockfd);
    if (err) {
        close(sockfd);
        removeFileEventEvLoop(loop, sockfd, mask);
        /* retry */
        if (err == EAGAIN || err == ETIMEDOUT || ECONNREFUSED) {
            /* retry limit exceeded. remove the worker */
            if (worker->retries >= WORKER_RETRIES_MAX) {
                worker->retries = 0; 
                mapRemove(worker->jk->workers, worker->id);
                return;
            }
            worker->retries++;
            jkLog(LT_DEBUG, strerror(err));
            nbWorkerConnect(loop, worker);
            return;
        }
        jkLog(LT_ERROR, strerror(err));
        stopEvLoop(loop);
        return;
    }

    /* build the handshake */
    worker->sendlen = buildHandshake(worker->sendbuf,
            worker->jk->infoHash, worker->jk->id);

    removeFileEventEvLoop(loop, sockfd, mask);
    if (!addFileEventEvLoop(loop,
            sockfd,
            EV_WRITABLE,
            nbOnHandshakeSend,
            worker)) {
        jkLog(LT_ERROR, strerror(errno));
        stopEvLoop(loop);
    }
}

void nbWorkerConnect(struct evLoop *loop, struct worker *worker)
{
    /* extract ip and port from worker id
     * worker id format: 'ip:port' */
    int sep = strlen(worker->id);
    while (--sep >= 0 && worker->id[sep] != ':');
    char ip[sep+1];
    ip[sep] = 0;
    strncpy(ip, worker->id, sep);
    char *port = worker->id + sep + 1;

    struct addrinfo *info;
    int r = jnetResolveNumericHost(&info, ip, port);
    if (r) {
        jkLog(LT_ERROR, gai_strerror(r));
        goto error;
    }
    worker->sockfd = jnetSocket(info);
    if (worker->sockfd == -1) {
        jkLog(LT_WARNING, strerror(errno));
        goto error;
    }
    /* connect to the peer */
    r = jnetConnect(worker->sockfd, info);
    if (r == -1) {
        jkLog(LT_WARNING, strerror(errno));
        goto error;
    }
    if (!addFileEventEvLoop(loop,
            worker->sockfd,
            EV_WRITABLE,
            nbOnConnect,
            worker)) {
        goto error;
    }
    freeaddrinfo(info);
    return;

error:
    freeaddrinfo(info);
    if (worker->sockfd != -1)
        close(worker->sockfd);
    stopEvLoop(loop);
}

