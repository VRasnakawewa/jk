#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

#include "ev.h"

void initEvLoop(struct evLoop *loop)
{
    loop->stop = 0;
    loop->fileEventHead = NULL;
    loop->timeEventHead = NULL;
}

int addFileEventEvLoop(struct evLoop *loop,
        int fd, int mask, procFileEventFn *proc, void *data)
{
    struct fileEvent *e; 

    e = malloc(sizeof(*e));
    if (!e) return 0;
    e->fd = fd; 
    e->mask = mask;
    e->proc = proc;
    e->data = data;
    e->next = loop->fileEventHead;
    loop->fileEventHead = e;
    return 1;
}
void removeFileEventEvLoop(struct evLoop *loop, int fd, int mask)
{
    struct fileEvent *curr = loop->fileEventHead;
    struct fileEvent *prev = NULL;

    while (curr) {
        if (curr->fd == fd && curr->mask == mask) {
            if (!prev)
                loop->fileEventHead = curr->next;
            else
                prev->next = curr->next;
            free(curr);
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}

int processEventsEvLoop(struct evLoop *loop)
{
    fd_set rfds; 
    fd_set wfds; 
    fd_set efds; 
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    FD_ZERO(&efds);

    int maxfd = 0;
    int numfd = 0;
    struct fileEvent *fe = loop->fileEventHead;

    while (fe) {
        if (fe->mask & EV_READABLE)  FD_SET(fe->fd, &rfds);
        if (fe->mask & EV_WRITABLE)  FD_SET(fe->fd, &wfds);
        if (fe->mask & EV_EXCEPTION) FD_SET(fe->fd, &efds);
        if (maxfd < fe->fd) 
            maxfd = fe->fd;
        numfd++; 
        fe = fe->next;
    }

    int processed = 0;
    int r = select(maxfd+1, &rfds, &wfds, &efds, NULL);
    if (r > 0) {
        fe = loop->fileEventHead; 
        while (fe) {
            int mask = 0;
            if (fe->mask & EV_READABLE && FD_ISSET(fe->fd, &rfds))
                mask |= EV_READABLE;
            if (fe->mask & EV_WRITABLE && FD_ISSET(fe->fd, &wfds))
                mask |= EV_WRITABLE;
            if (fe->mask & EV_EXCEPTION && FD_ISSET(fe->fd, &efds))
                mask |= EV_EXCEPTION;

            if (mask) {
                int fd = fe->fd;
                fe->proc(loop, fe->fd, fe->data, mask);
                processed++;
                fe = loop->fileEventHead;
                FD_CLR(fd, &rfds);
                FD_CLR(fd, &wfds);
                FD_CLR(fd, &efds);
            } else {
                fe = fe->next;
            }
        }
    }
    return processed;
}

void runEvLoop(struct evLoop *loop)
{
    while (!loop->stop)
        processEventsEvLoop(loop);
}

void stopEvLoop(struct evLoop *loop)
{
    loop->stop = 1;
}

