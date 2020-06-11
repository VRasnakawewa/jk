#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>

#include "tracker.h"

int calculateInfoHash(unsigned char *hash, unsigned char *metaRaw, u64 metaRawLen)
{
    unsigned char *info = strstr(metaRaw, "infod");
    if (!info) return 0;
    info += strlen("info");
    u64 offset = info - metaRaw;

    STACK(s, metaRawLen-offset);
    STACK_INIT(s);
    u64 i = 0;
    while (i < metaRawLen) {
        switch (info[i]) {
        case 'i':
            while (i < metaRawLen && info[i] != 'e') i++;
            if (i == metaRawLen) return 0;
            i++;
            break;
        case 'l':
        case 'd':
            STACK_PUSH(s,info[i]);
            i++;
            break;
        case 'e':
            STACK_POP(s);
            i++;
            break;
        default: {
            u64 start = i;
            while (i < metaRawLen && info[i] != ':') i++;
            if (i == metaRawLen) return 0;
            u64 len;
            if (!tou64(&len, info, start, i)) return 0;
            i = i + 1 + len;
            break;
        }
        }
        if (STACK_EMPTY(s))
            break;
    }

    u64 infolen = i;
    SHA1(info, infolen, hash);
    return 1;
}

