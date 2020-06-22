#include <stdio.h>
#include <stdlib.h>

#include "util.h"
#include "ben.h"

/* ---------------------------------------------------------
 * Decoding
 * --------------------------------------------------------- */

struct buffer {
    u64 len;
    u64 pos;
    unsigned char *data;
};

static int bufferDecodeNext(struct buffer *buf, struct benNode *node);
static void benDestroyBenNode(void *node);

static int bufferFind(struct buffer *buf, unsigned char c, u64 *index)
{
    unsigned char *data = buf->data;

    for (*index = buf->pos; *index < buf->len; (*index)++) {
        if (data[*index] == c)
            return BEN_OK;
    }
    *index = 0;
    return BEN_ERR;
}

static int bufferDecodeNextI64(struct buffer *buf, i64 *num)
{
    int r;
    unsigned char *data = buf->data;

    u64 end;
    r = bufferFind(buf, 'e', &end);
    if (r != BEN_OK) return r;
    r = toi64(num, data, buf->pos+1, end);
    if (!r) return BEN_ERR;
    buf->pos += end + 1 - buf->pos;
    return BEN_OK;
}

static int bufferDecodeNextJstr(struct buffer *buf, jstr *str)
{
    int r;
    unsigned char *data = buf->data;

    u64 sep;
    r = bufferFind(buf, ':', &sep);
    if (r != BEN_OK) return r;
    u64 len;
    r = tou64(&len, data, buf->pos, sep);
    if (!r) return BEN_ERR;
    u64 end = ++sep + len;
    buf->pos = end;
    *str = newJstr(data+sep, end-sep);
    return (*str) ? BEN_OK : BEN_ERR;
}

static int bufferDecodeNextList(struct buffer *buf, struct list *list)
{
    int r;
    unsigned char *data = buf->data;

    buf->pos++;
    while (data[buf->pos] != 'e') {
        struct benNode *node;

        node = malloc(sizeof(*node));
        if (!node) return BEN_ERR;
        r = bufferDecodeNext(buf, node);
        if (r != BEN_OK) {
            free(node);
            return r;
        }
        listAdd(list, node);
        if (LIST_ALLOC_FAILED(list)) {
            free(node);
            return BEN_ERR;
        }
    }
    buf->pos++;

    return BEN_OK;
}

static int bufferDecodeNextMap(struct buffer *buf, struct map *map)
{
    int r;
    unsigned char *data = buf->data;

    buf->pos++;
    while (data[buf->pos] != 'e') {
        struct benNode *node; 
        jstr key;

        node = malloc(sizeof(*node));
        if (!node) return BEN_ERR;
        r = bufferDecodeNextJstr(buf, &key);
        if (r != BEN_OK) {
            free(node);
            return r;
        }
        r = bufferDecodeNext(buf, node);
        if (r != BEN_OK) {
            free(node);
            return r;
        }
        mapPut(map, key, node);
        if (MAP_ALLOC_FAILED(map)) {
            free(node);
            destroyJstr(key);
            return BEN_ERR;
        }
    }
    buf->pos++;

    return BEN_OK;
}

static int bufferDecodeNext(struct buffer *buf, struct benNode *node)
{
    int r;
    unsigned char *data = buf->data;

    switch (data[buf->pos]) {
    case 'l': {
        node->type = BEN_TYPE_LIST;
        struct list *list = listNew(16, benDestroyBenNode);
        if (!list) return BEN_ERR;
        node->value.l = list;
        r = bufferDecodeNextList(buf, node->value.l);
        if (r != BEN_OK) {
            listDestroy(list);
            return r;
        }
        return BEN_OK;
    }
    case 'd': {
        node->type = BEN_TYPE_MAP;
        struct map *map = mapNew(8, 0.75, destroyJstr, benDestroyBenNode);
        if (!map) return BEN_ERR;
        node->value.m = map;
        r = bufferDecodeNextMap(buf, node->value.m);
        if (r != BEN_OK) {
            mapDestroy(map);
            return r;
        }
        return BEN_OK;
    }
    case 'i':
        node->type = BEN_TYPE_I64;
        return bufferDecodeNextI64(buf, &node->value.i);
    default:
        node->type = BEN_TYPE_JSTR;
        r = bufferDecodeNextJstr(buf, &node->value.s);
        if (r != BEN_OK) {
            destroyJstr(node->value.s);
            return r;
        }
    }
}

int benDecode(struct benNode *node, unsigned char *data, u64 dataLen)
{
    int r;
    struct buffer buf;

    buf.pos = 0;
    buf.len = dataLen;
    buf.data = data;

    r = bufferDecodeNext(&buf, node);
    return r;
}

static void benDestroyBenNode(void *node)
{
    if (!node) return;

    struct benNode *n = (struct benNode *)node;
    switch (n->type) {
    case BEN_TYPE_I64:
        break;
    case BEN_TYPE_JSTR:
        if (n->value.s) {
            destroyJstr(n->value.s);
            n->value.s = NULL;
        }
        break;
    case BEN_TYPE_LIST:
        if (n->value.l) {
            listDestroy(n->value.l);
            n->value.l = NULL;
        }
        break;
    case BEN_TYPE_MAP:
        if (n->value.m) {
            mapDestroy(n->value.m);
            n->value.m = NULL;
        }
        break;
    default:
        break;
    }
    free(n);
}

