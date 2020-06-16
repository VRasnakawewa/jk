#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "client.h"

struct client *clientNew(char *id, struct jk *jk)
{
    struct client *client;

    client = malloc(sizeof(*client));
    if (!client) return NULL;
    client->id = id;
    client->jk = jk;
    client->sockfd = -1;
    client->chocked = 0;
    return client;
}

void clientDestroy(void *client) { free(client); }


