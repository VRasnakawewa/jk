#ifndef URLENCODE_H
#define URLENCODE_H

#include <stdio.h>

#define URLENCODE_DEST_LEN(srclen) ((srclen) * 3 + 1)

size_t urlencode(char *dest, char *src, size_t len);

#endif
