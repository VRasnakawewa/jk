#ifndef REAP_H
#define REAP_H

#include "ev.h"

void reap(struct evLoop *loop,
          const char *hostname,
          const char *path,
          char **headers,
          char **params,
          void (*onSuccess)(struct evLoop *loop, char *response),
          void (*onError)(struct evLoop *loop, char *error));

#endif
