#ifndef UTIL_H
#define UTIL_H

#include "jstr.h"
#include "common.h"
#include "stack.h"

jstr readFile(const char *filename);
int toi64(i64 *v, unsigned char *s, u64 start, u64 end);
int tou64(u64 *v, unsigned char *s, u64 start, u64 end);
int calculateInfoHash(unsigned char *hash,
                      unsigned char *metaRaw,
                      u64 metaRawLen);
void generatePeerId(unsigned char *peerId);

#endif
