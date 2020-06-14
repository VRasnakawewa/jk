#ifndef REAP_H
#define REAP_H

#include "ev.h"

void reap(struct evLoop *loop,
          void *callerData,
          const char *hostname,
          const char *port,
          const char *path,
          char **headers,
          char **params,
          void onResponseFn(struct evLoop *loop,
                            void *callerData,
                            char *response,
                            int error,
                            const char *errMsg));

#endif
