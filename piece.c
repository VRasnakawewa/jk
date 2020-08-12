/*
 * MIT License
 * 
 * Copyright (c) 2020 Sujanan Bhathiya
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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
