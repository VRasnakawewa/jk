#include <ctype.h>

#include "urlencode.h"

size_t urlencode(char *dest, char *src, size_t len)
{
    static const char hex[] = "0123456789ABCDEF";

    size_t i = 0; 
    while (i < len) {
        unsigned char c = src[i];

        if (isalnum(c)
            || c == '-' 
            || c == '_'
            || c == '.'
            || c == '~')
            dest[i++] = c;
        else if (c == ' ')
            dest[i++] = '+';
        else {
            dest[i++] = '%';
            dest[i++] = hex[c >>  4];
            dest[i++] = hex[c & 0xF];
        }
    }
    dest[i] = 0;

    return i;
}

