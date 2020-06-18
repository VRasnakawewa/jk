#ifndef REAP_H
#define REAP_H

#include "ev.h"

typedef char *reapStrerrorFn(int errcode);

typedef void onResponseFn(struct evLoop *loop,
                          void *callerData,
                          char *response,
                          size_t responseLen,
                          int errcode,
                          reapStrerrorFn *reapStrerror);

void reap(struct evLoop *loop,
          void *callerData,
          char *hostname,
          char *port,
          char *path,
          char **headers,
          char **params,
          onResponseFn *onResponse);

#endif
