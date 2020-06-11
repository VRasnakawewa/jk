#include "util.h"

int toi64(i64 *v, unsigned char *s, u64 start, u64 end) 
{
    int sign = 1;
    *v = 0; 
    for (u64 i = start; i < end; i++) {
        unsigned char c = s[i];
        if (c >= '0' && c <= '9') {
            *v = *v * 10 + (c-'0');
            continue;
        }
        if (i == start && c == '-')
            sign = -1;
        else if (i == start && c == '+')
            ;
        else if (c == '.')
            break;
        else {
            *v = 0;
            return 0;
        }
    }
    return 1;
}

int tou64(u64 *v, unsigned char *s, u64 start, u64 end) 
{
    *v = 0; 
    for (u64 i = start; i < end; i++) {
        unsigned char c = s[i];
        if (c >= '0' && c <= '9') {
            *v = *v * 10 + (c-'0');
            continue;
        }
        if (i == start && c == '-') {
            *v = 0;
            return 0;
        }
        else if (i == start && c == '+')
            ;
        else if (c == '.')
            break;
        else {
            *v = 0;
            return 0;
        }
    }
    return 1;
}

