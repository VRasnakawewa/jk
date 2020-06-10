#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "common.h"
#include "ben.h"

#define READ_CHUNK_SIZE 1024
jstr readTorrentFile(const char *filename)
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

    data = readTorrentFile(filename);
    if (!data) return 1;
    r = benDecode(&node, data);
    if (r != BEN_OK) {
        fprintf(stderr, "error: couldn't decode the torrent: %s\n", filename);
        return 1;
    }
    struct map *map = BEN_AS_MAP(node);
    jstr tracker = BEN_AS_STR(getValMap(map, "announce"));
    printf("%s\n", JSTR(tracker));

    benDestroy(node);
    return 0;
}
