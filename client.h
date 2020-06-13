#ifndef CLIENT_H
#define CLIENT_H

#include "ev.h"

struct client {
    struct evLoop *loop;
    int sockfd; 
    int choked;
    char *peerIp;
    char *peerPort;
};

void clientConnect(struct evLoop *loop,
                   char *peerIp,
                   char *peerPort,
                   void onConnect(struct evLoop *loop,
                                  struct client *client,
                                  int error));
void clientRead(struct evLoop *loop,
                void onRead(struct evLoop *loop,
                            char *message,
                            int error));
void clientSendRequest(struct evLoop *loop,
                       void onSendRequest(struct evLoop *loop, int error));

#endif
