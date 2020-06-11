#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "common.h"
#include "ben.h"
#include "tracker.h"

#define READ_CHUNK_SIZE 1024
static jstr readFile(const char *filename)
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

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: jk <torrent>\n");
        return 1;
    }
    char *filename = argv[1];

    int r;
    struct benNode *node;
    jstr data;  

    data = readFile(filename);
    unsigned char hash[20];
    calculateInfoHash(hash, JSTR(data), lenJstr(data));
    benDecode(&node, JSTR(data), lenJstr(data));
    destroyJstr(data);
    benDestroyBenNode(node);
    return 0;
}

