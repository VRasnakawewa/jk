#include <ctype.h>

#include "urlencode.h"

size_t urlencode(char *dest, char *src, size_t len)
{
    static const char hex[] = "0123456789ABCDEF";

    size_t i = 0; 
    size_t j = 0; 
    while (i < len) {
        unsigned char c = src[i++];

        if (isalnum(c)
            || c == '-' 
            || c == '_'
            || c == '.'
            || c == '~')
            dest[j++] = c;
        else if (c == ' ')
            dest[j++] = '+';
        else {
            dest[j++] = '%';
            dest[j++] = hex[c >>  4];
            dest[j++] = hex[c & 0xF];
        }
    }
    dest[j] = 0;

    return i;
}

