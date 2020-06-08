#ifndef EV_H
#define EV_H

#include "common.h"

struct evLoop;

typedef void procFileEventFn(struct evLoop *loop,
        int fd, void *data, int mask);
typedef int procTimeEventFn(struct evLoop *loop,
        u64 id, void *data);

struct fileEvent {
    int fd;
    int mask;
    procFileEventFn *proc;
    void *data;
    struct fileEvent *next;
};

struct timeEvent {
    u64 id;
    long whenSec;
    long whenMilSec;
    procTimeEventFn *proc; 
    void *data; 
    struct timeEvent *next;
};

struct evLoop {
    struct fileEvent *fileEventHead;
    struct timeEvent *timeEventHead;
    int stop;
};

#define EV_READABLE  1
#define EV_WRITABLE  2
#define EV_EXCEPTION 4

void initEvLoop(struct evLoop *loop);
void destroyEvLoop(struct evLoop *loop);
int addFileEventEvLoop(struct evLoop *loop,
        int fd, int mask, procFileEventFn *proc, void *data);
void removeFileEventEvLoop(struct evLoop *loop, int fd, int mask);
u64 addTimeEventEvLoop(struct evLoop *loop,
        long long milSec, void *data);
int removeTimeEventEvLoop(struct evLoop *loop, u64 id);
int processEventsEvLoop(struct evLoop *loop);
void runEvLoop(struct evLoop *loop);

#endif
