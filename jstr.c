/*
 * MIT License
 * 
 * Copyright (c) 2020 Sujanan Bhathiya
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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

