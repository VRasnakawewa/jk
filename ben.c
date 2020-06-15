#include <stdio.h>
#include <stdlib.h>

#include "util.h"
#include "ben.h"

/* ---------------------------------------------------------
 * Encoding
 * --------------------------------------------------------- */

int benEncodeI64(jstr *data, i64 num)
{
    if (!(*data)) {
        *data = newEmptyJstr();
        if (!(*data)) return BEN_ERR;
    }

    char str[snprintf(NULL, 0, "i%lde", num) + 1];
    snprintf(str, sizeof(str), "i%lde", num);
    str[sizeof(str) - 1] = '\0';

    *data = pushJstr(*data, str);
    if (!(*data)) return BEN_ERR;

    return BEN_OK;
}

int benEncodeJstr(jstr *data, jstr str)
{
    if (!(*data)) {
        *data = newEmptyJstr();
        if (!(*data)) return BEN_ERR;
    }

    char prefix[snprintf(NULL, 0, "%lu:", lenJstr(str)) + 1];
    snprintf(prefix, sizeof(prefix), "%lu:", lenJstr(str));
    prefix[sizeof(prefix) - 1] = '\0';

    *data = pushJstr(*data, prefix);
    if (!(*data)) return BEN_ERR;
    *data = pushJstr(*data, str);
    if (!(*data)) return BEN_ERR;

    return BEN_OK;
}

int benEncodeList(jstr *data, struct list *list)
{
    if (!(*data)) {
        *data = newEmptyJstr();
        if (!(*data)) return BEN_ERR;
    }

    *data = pushJstr(*data, "l");
    if (!(*data)) return BEN_ERR;

    for (u64 i = 0; i < list->len; i++) {
        if (!list->values[i]) continue;
        if (benEncode(data, list->values[i]) != BEN_OK)
            return BEN_ERR;
    }

    *data = pushJstr(*data, "e");
    if (!(*data)) return BEN_ERR;

    return BEN_OK;
}

int benEncodeMap(jstr *data, struct map *map)
{
    if (!(*data)) {
        *data = newEmptyJstr();
        if (!(*data)) return BEN_ERR;
    }

    *data = pushJstr(*data, "d");
    if (!(*data)) return BEN_ERR;

    struct mapIterator it; 
    mapIteratorInit(&it, map);

    struct mapEntry *e = mapIteratorNext(&it);
    while (e) {
        if (benEncodeJstr(data, e->key) != BEN_OK)
            return BEN_ERR;
        if (benEncode(data, e->val) != BEN_OK)
            return BEN_ERR;
        e = mapIteratorNext(&it);
    }

    *data = pushJstr(*data, "e");
    if (!(*data)) return BEN_ERR;

    return BEN_OK;
}

int benEncode(jstr *data, struct benNode *node)
{
    switch (node->type) {
    case BEN_TYPE_I64:
        return benEncodeI64(data, benAsI64(node));
    case BEN_TYPE_JSTR:
        return benEncodeJstr(data, benAsJstr(node));
    case BEN_TYPE_LIST:
        return benEncodeList(data, benAsList(node));
    case BEN_TYPE_MAP:
        return benEncodeMap(data, benAsMap(node));
    default:
        return BEN_ERR;
    }
}


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
        struct map *map = mapNew(8, 0.75, benDestroyBenNode);
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

