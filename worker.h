#ifndef CLIENT_H
#define CLIENT_H

#include "jk.h"
#include "jnet.h"

#define WORKER_RETRIES_MAX 4

struct work {
    u64 pieceIndex;
    unsigned char pieceHash[20];
    u64 downloaded;
};

struct worker {
    char *id;
    struct jk *jk;
    int stoped;
    int chocked;
    int sockfd;
    int retries;
    u64 recvlen;
    unsigned char sendbuf[512];
    u64 sendpos;
    u64 sendlen;
    unsigned char recvbuf[512];
};

struct worker *workerNew(char *id, struct jk *jk);
void workerDestroy(void *worker);
void nbWorkerConnect(struct evLoop *loop, struct worker *worker);

#endif
