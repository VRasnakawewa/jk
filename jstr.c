#include <stdlib.h>
#include <string.h>

#include "jstr.h"

struct jstrHeader {
    size_t len; 
    char buf[];
};

#define GET_HEADER(j) \
    ((struct jstrHeader *)((j)-sizeof(struct jstrHeader)))

jstr newJstr(char *str, size_t len)
{
    if (!len)
        len = strlen(str);

    struct jstrHeader *h;

    h = malloc(sizeof(*h) +
        len + 
        1); /* last byte */
    if (!h) return NULL;
    h->len = len;
    h->buf[len] = '\0';          /* set last byte */
    memcpy(h->buf, str, len);
    return h->buf;
}

jstr newEmptyJstr()
{
    return newJstr("",0);
}

size_t lenJstr(const jstr str)
{
    return (GET_HEADER(str))->len;
}

void destroyJstr(void *str)
{
    free(GET_HEADER(str));
}

