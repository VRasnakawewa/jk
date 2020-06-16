#ifndef CLIENT_H
#define CLIENT_H

#include "jk.h"

struct client {
    char *id;
    struct jk *jk;
    int stoped;
    int chocked;
    int sockfd;
};

struct client *clientNew(char *id, struct jk *jk);
void clientDestroy(void *client);

#endif
