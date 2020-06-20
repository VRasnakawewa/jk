#ifndef CLIENT_H
#define CLIENT_H

#include "jk.h"
#include "jnet.h"

struct worker {
    char *id;
    struct jk *jk;
    int sockfd;
    int chocked;
    u64 sendpos;
    u64 sendlen;
    u64 recvlen;
    unsigned char sendbuf[128];
    unsigned char recvbuf[128];
};

struct worker *workerNew(char *id, struct jk *jk);
void workerDestroy(void *worker);
void workerResetSendBuf(struct worker *worker);
void workerResetRecvBuf(struct worker *worker);
void nbWorkerConnect(struct evLoop *loop, struct worker *worker);

#endif
