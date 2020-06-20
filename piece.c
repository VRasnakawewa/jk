#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "piece.h"

struct piece *pieceNew(u64 index, u64 bufcap,
        unsigned char *hash, jstr path)
{
    struct piece *piece; 

    piece = malloc(sizeof(*piece));
    if (!piece) return NULL;
    piece->index = index;
    piece->buf = NULL;
    piece->buflen = 0;
    piece->bufcap = bufcap;
    strncpy(piece->hash, hash, sizeof(piece->hash));
    piece->path = path;
    return piece;
}

void pieceDestroy(struct piece *piece)
{
    if (piece) free(piece->buf);
    free(piece);
}

void pieceAllocBuf(struct piece *piece)
{
    if (!piece->buf)
        piece->buf = malloc(piece->bufcap);
}
