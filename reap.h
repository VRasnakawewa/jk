#ifndef REAP_H
#define REAP_H

#include "ev.h"

void reap(struct evLoop *loop,
          void *callerData,
          char *hostname,
          char *port,
          char *path,
          char **headers,
          char **params,
          void onResponseFn(struct evLoop *loop,
                            void *callerData,
                            char *response,
                            size_t responseLen,
                            int error,
                            const char *errMsg));

#endif
