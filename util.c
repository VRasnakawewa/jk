#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <openssl/sha.h>

#include "util.h"

#define READ_CHUNK_SIZE 1024
jstr readFile(const char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (!file) 
        return NULL;
    fseek(file, 0, SEEK_END);
    u64 len = ftell(file);
    fseek(file, 0, SEEK_SET);
    char buf[len+1];
    fread(buf, len, 1, file);
    fclose(file);
    return newJstr(buf, len);
}

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

int calculateInfoHash(unsigned char *hash,
                      unsigned char *metaRaw,
                      u64 metaRawLen)
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

void generatePeerId(unsigned char *peerId)
{
    pid_t pid = getpid();
    static const char numbers[] = "0123456789";
    peerId[0] = '-';
    peerId[1] = 'J';
    peerId[2] = 'K';
    peerId[3] = '1';
    peerId[4] = '0';
    peerId[5] = '0';
    peerId[6] = '0';
    peerId[7] = '-';
    srand(pid + 0);  peerId[ 8] = numbers[rand() % (sizeof(numbers)-1)]; 
    srand(pid + 1);  peerId[ 9] = numbers[rand() % (sizeof(numbers)-1)]; 
    srand(pid + 2);  peerId[10] = numbers[rand() % (sizeof(numbers)-1)]; 
    srand(pid + 3);  peerId[11] = numbers[rand() % (sizeof(numbers)-1)]; 
    srand(pid + 4);  peerId[12] = numbers[rand() % (sizeof(numbers)-1)]; 
    srand(pid + 5);  peerId[13] = numbers[rand() % (sizeof(numbers)-1)]; 
    srand(pid + 6);  peerId[14] = numbers[rand() % (sizeof(numbers)-1)]; 
    srand(pid + 7);  peerId[15] = numbers[rand() % (sizeof(numbers)-1)]; 
    srand(pid + 8);  peerId[16] = numbers[rand() % (sizeof(numbers)-1)]; 
    srand(pid + 9);  peerId[17] = numbers[rand() % (sizeof(numbers)-1)]; 
    srand(pid +10);  peerId[18] = numbers[rand() % (sizeof(numbers)-1)]; 
    srand(pid +11);  peerId[19] = numbers[rand() % (sizeof(numbers)-1)]; 
}

