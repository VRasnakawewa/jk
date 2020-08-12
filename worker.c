/*
 * MIT License
 * 
 * Copyright (c) 2020 Sujanan Bhathiya
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/tcp.h>

#include "worker.h"

#define PSTR "BitTorrent protocol"

#define MSG_CHOKE 0
#define MSG_UNCHOKE 1
#define MSG_INTERESTED 2
#define MSG_NOT_INTERESTED 3
#define MSG_HAVE 4
#define MSG_BITFIELD 5
#define MSG_REQUEST 6
#define MSG_PIECE 7
#define MSG_CANCEL 8

struct worker *workerNew(char *id, struct jk *jk)
{
    struct worker *worker;

    worker = malloc(sizeof(*worker));
    if (!worker) return NULL;
    worker->id = id;
    worker->jk = jk;
    worker->sockfd = -1;
    worker->chocked = 0;
    worker->sendpos = 0;
    worker->sendlen = 0;
    worker->recvlen = 0;
    return worker;
}

void workerDestroy(void *worker) { free(worker); }

void workerResetSendBuf(struct worker *worker)
{
    worker->sendpos = 0;
    worker->sendlen = 0;
    memset(worker->sendbuf,0,sizeof(worker->sendbuf));
}

void workerResetRecvBuf(struct worker *worker)
{
    worker->recvlen = 0;
    memset(worker->recvbuf,0,sizeof(worker->recvbuf));
}

static int buildHandshake(unsigned char *handshake,
                           const unsigned char *infoHash,
                           const unsigned char *peerId)
{
    int i = 0;

    handshake[i] = 19;
    i += 1;
    memcpy(handshake + i, PSTR, 19);
    i += 19;
    memset(handshake + i, 0, 8);
    i += 8;
    memcpy(handshake + i, infoHash, 20);
    i += 20;
    memcpy(handshake + i, peerId, 20);
    i += 20;

    return i;
}

static int verifyHandshake(const char *handshake, const char *infoHash)
{
    return handshake[0] == 19 &&
        !memcmp(handshake + 1, PSTR, 19) &&
        !memcmp(handshake + 1+19+8, infoHash, 20);
}

static void nbOnHandshakeRecv(struct evLoop *loop,
        int sockfd, void *data, int mask)
{
    struct worker *worker = (struct worker *)data;

    u64 n = recv(worker->sockfd, worker->recvbuf + worker->recvlen,
        sizeof(worker->recvbuf) - worker->recvlen, 0);
    if (n == -1) {
        jkLog(LT_DEBUG, "recv: (%s) %s", worker->id, strerror(errno));
        close(sockfd);
        removeFileEventEvLoop(loop, sockfd, mask);
        mapRemove(worker->jk->workers, worker->id);
        return;
    }
    if (n == 0) {
        removeFileEventEvLoop(loop, sockfd, mask);
        if (verifyHandshake(worker->recvbuf, worker->jk->infoHash)) {
            jkLog(LT_INFO, "🤝 with %s", worker->id);
            workerResetSendBuf(worker);
            workerResetRecvBuf(worker);
            return;
        }
        return;
    }
    worker->recvlen += n; 
}

static void nbOnHandshakeSend(struct evLoop *loop,
        int sockfd, void *data, int mask)
{
    struct worker *worker = (struct worker *)data;

    /* all sent */
    if (worker->sendpos == worker->sendlen) {
        removeFileEventEvLoop(loop, sockfd, mask);
        if (!addFileEventEvLoop(loop,
                sockfd,
                EV_READABLE,
                nbOnHandshakeRecv,
                worker)) {
            jkLog(LT_ERROR, strerror(errno));
            stopEvLoop(loop);
        }
        return;
    }
     
    u64 n = send(worker->sockfd, worker->sendbuf + worker->sendpos,
        worker->sendlen, 0);
    if (n == -1) {
        jkLog(LT_DEBUG, "send: (%s) %s", worker->id, strerror(errno));
        close(sockfd);
        removeFileEventEvLoop(loop, sockfd, mask);
        mapRemove(worker->jk->workers, worker->id);
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
        jkLog(LT_DEBUG, "connect (%s): %s", worker->id, strerror(err));
        close(sockfd);
        removeFileEventEvLoop(loop, sockfd, mask);
        mapRemove(worker->jk->workers, worker->id);
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

