#ifndef JSTR_H
#define JSTR_H

#include <stdio.h>

typedef char * jstr;

#define JSTR_INVALID_BYTE 0xC0

#define JSTR_VALID(j) ((unsigned char)(*(j)) == JSTR_INVALID_BYTE)

#define JSTR(j) ((JSTR_VALID(j)) ? ((j)+1) : (j))

jstr newJstr(char *str, size_t len);
jstr newEmptyJstr();
size_t lenJstr(const jstr str);
int cmpJstr(const jstr str1, const jstr str2);
jstr pushJstr(jstr str, jstr newstr);
void destroyJstr(jstr str);

#endif
