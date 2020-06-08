#include "map.h"
#include "jstr.h"

struct buffer {
    u64 len;
    u64 pos;
    jstr data;
};

static int findBuffer(struct buffer *buf, unsigned char c, u64 *index)
{
    for (*index = buf->pos; *index < buf->len; (*index)++) {
        if (buf->data[*index] == c)
            return BEN_OK;
    }
    *index = 0;
    return BEN_DECODE_ERROR;
}

static int decodeNextI64Buffer(struct buffer *buf)
{
}

static int decodeNextJstrBuffer(struct buffer *buf)
{
}

static int decodeNextListBuffer(struct buffer *buf)
{
}

