#ifndef CLIENT_H
#define CLIENT_H

#include "jk.h"
#include "jnet.h"

struct worker {
    char *id;
    struct jk *jk;
    int stoped;
    int chocked;
    int sockfd;
};

struct worker *workerNew(char *id, struct jk *jk);
void workerDestroy(void *worker);
void nbWorkerConnect(struct evLoop *loop, struct worker *worker);

#endif
