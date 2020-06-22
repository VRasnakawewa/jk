#ifndef JSTR_H
#define JSTR_H

#include <stdio.h>

typedef char * jstr;

jstr newJstr(char *str, size_t len);
jstr newEmptyJstr();
size_t lenJstr(const jstr str);
void destroyJstr(void *str);

#endif
