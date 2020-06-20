#ifndef PIECE_H
#define PIECE_H

#include "common.h"
#include "jstr.h"
#include "piece.h"

#define PIECE_ALLOC_FAILED(piece) (!(piece) || (!(piece)->buf))

struct piece {
    u64 index;
    u64 bufcap;
    u64 buflen;
    unsigned char *buf; 
    unsigned char hash[20];
    jstr path;
};

#endif
