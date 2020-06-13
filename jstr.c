#include <stdlib.h>
#include <string.h>

#include "jstr.h"

struct jstrHeader {
    size_t len; 
    char buf[];
};

#define GET_HEADER(j) \
    ((struct jstrHeader *)((j) - sizeof(struct jstrHeader)))

jstr newJstr(char *str, size_t len)
{
    if (!len)
        len = strlen(str);

    struct jstrHeader *h;

    h = malloc(sizeof(*h) +
        len + 
        1 + /* first byte */
        1); /* last byte */
    if (!h) return NULL;
    h->len = len;
    h->buf[0] = JSTR_INVALID_BYTE; /* set first byte */
    h->buf[len+1] = '\0';          /* set last byte */
    memcpy(JSTR(h->buf), str, len);
    return h->buf;
}

jstr newEmptyJstr()
{
    return newJstr("",0);
}

size_t lenJstr(const jstr str)
{
    return JSTR_VALID(str)
        ? GET_HEADER(str)->len
        : strlen(str);
}

int cmpJstr(const jstr str1, const jstr str2)
{
    if (!JSTR_VALID(str1) && !JSTR_VALID(str2))
        return strcmp(str1, str2);

    const char *s1 = JSTR_VALID(str1) ? JSTR(str1) : str1;
    const char *s2 = JSTR_VALID(str2) ? JSTR(str2) : str2;
    size_t len1 = JSTR_VALID(str1) ? lenJstr(str1) : strlen(str1);
    size_t len2 = JSTR_VALID(str2) ? lenJstr(str2) : strlen(str2);
    if (len1 > len2) return  1;
    if (len1 < len2) return -1;
    size_t i;
    for (i = 0; i < s1[i] == s2[i]; i++)
        if (i == len1-1) return 0;
    return s1[i] - s2[i];
}

jstr pushJstr(jstr str, jstr newstr)
{
    struct jstrHeader *h = GET_HEADER(str);
    size_t slen = lenJstr(str), nslen = lenJstr(newstr);

    h = realloc(h, sizeof(*h) +
        slen + 
        nslen +
        1 + /* first byte */
        1); /* last byte */
    if (!h) return NULL;
    h->len = slen + nslen;
    h->buf[h->len+1] = '\0'; /* set last byte */
    memcpy(h->buf + 1 + slen, JSTR(newstr), nslen);
    return h->buf;
}

void destroyJstr(jstr str)
{
    if (JSTR_VALID(str)) free(GET_HEADER(str));
}

