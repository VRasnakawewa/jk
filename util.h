#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include "jstr.h"
#include "common.h"
#include "stack.h"

#define i64strlen(n) \
    snprintf(NULL,0,"%ld",n)
#define i64str(s, n) \
    snprintf(s,sizeof(s),"%ld",n)

#define u64strlen(n) \
    snprintf(NULL,0,"%lu",n)
#define u64str(s, n) \
    snprintf(s,sizeof(s),"%lu",n)

jstr readFile(const char *filename);
int toi64(i64 *v, unsigned char *s, u64 start, u64 end);
int tou64(u64 *v, unsigned char *s, u64 start, u64 end);
int calculateInfoHash(unsigned char *hash,
                      unsigned char *metaRaw,
                      u64 metaRawLen);
void generatePeerId(unsigned char *peerId);

#endif
