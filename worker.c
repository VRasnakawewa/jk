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
    return worker;
}

void workerDestroy(void *worker) { free(worker); }

static void nbOnConnect(struct evLoop *loop,
        int sockfd, void *data, int mask)
{
    struct worker *worker = (struct worker *)data;

    printf("%s CONNECTED\n", worker->id);

    removeFileEventEvLoop(loop, sockfd, mask);
    close(sockfd);
}

void nbWorkerConnect(struct evLoop *loop, struct worker *worker)
{
    int sep = strlen(worker->id);
    while (--sep >= 0 && worker->id[sep] != ':');

    char ip[sep+1];
    ip[sep] = 0;
    strncpy(ip, worker->id, sep);
    char *port = worker->id + sep + 1;

    struct addrinfo *info;
    int r = jnetResolveNumericHost(&info, ip, port);
    worker->sockfd = jnetSocket(info);
    if (worker->sockfd == -1) {
        jkLog(LT_WARNING, strerror(errno));
        close(worker->sockfd);
        return;
    }
    r = jnetConnect(worker->sockfd, info);
    if (r == -1) {
        jkLog(LT_WARNING, strerror(errno));
        close(worker->sockfd);
        return;
    }
    if (!addFileEventEvLoop(loop,
            worker->sockfd,
            EV_WRITABLE,
            nbOnConnect,
            worker)) {
        close(worker->sockfd);
        return;
    }
}

