#include <stdio.h>
#include <stdlib.h>

#include "ben.h"
#include "tools/vmap.h"

struct buffer {
    u64 len;
    u64 pos;
    jstr data;
};

static int decodeNextBuffer(struct buffer *buf, struct benNode *node);

static int toi64(i64 *v, unsigned char *s, u64 start, u64 end) 
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
            return BEN_ERR;
        }
    }
    return BEN_OK;
}

static int tou64(u64 *v, unsigned char *s, u64 start, u64 end) 
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
            return BEN_ERR;
        }
        else if (i == start && c == '+')
            ;
        else if (c == '.')
            break;
        else {
            *v = 0;
            return BEN_ERR;
        }
    }
    return BEN_OK;
}

static int findBuffer(struct buffer *buf, unsigned char c, u64 *index)
{
    unsigned char *data = JSTR(buf->data);

    for (*index = buf->pos; *index < buf->len; (*index)++) {
        if (data[*index] == c)
            return BEN_OK;
    }
    *index = 0;
    return BEN_ERR;
}

static int decodeNextI64Buffer(struct buffer *buf, i64 *num)
{
    int r;
    unsigned char *data = JSTR(buf->data);

    u64 end;
    r = findBuffer(buf, 'e', &end);
    if (r != BEN_OK) return r;
    r = toi64(num, data, buf->pos+1, end);
    if (r != BEN_OK) return r;
    buf->pos += end + 1 - buf->pos;
    return BEN_OK;
}

static int decodeNextJstrBuffer(struct buffer *buf, jstr *str)
{
    int r;
    unsigned char *data = JSTR(buf->data);

    u64 sep;
    r = findBuffer(buf, ':', &sep);
    if (r != BEN_OK) return r;
    u64 len;
    r = tou64(&len, data, buf->pos, sep);
    if (r != BEN_OK) return r;
    u64 end = ++sep + len;
    buf->pos = end;
    *str = newJstr(data+sep, end-sep);
    return (*str) ? BEN_OK : BEN_ERR;
}

static int decodeNextListBuffer(struct buffer *buf, struct list *list)
{
    int r;
    unsigned char *data = JSTR(buf->data);

    buf->pos++;
    while (data[buf->pos] != 'e') {
        struct benNode *node;

        node = malloc(sizeof(*node));
        if (!node) return BEN_ERR;
        r = decodeNextBuffer(buf, node);
        if (r != BEN_OK) {
            free(node);
            return r;
        }
        addValList(list, node);
        if (LIST_ALLOC_FAILED(list)) {
            free(node);
            return BEN_ERR;
        }
    }
    buf->pos++;

    return BEN_OK;
}

static int decodeNextMapBuffer(struct buffer *buf, struct map *map)
{
    int r;
    unsigned char *data = JSTR(buf->data);

    buf->pos++;
    while (data[buf->pos] != 'e') {
        struct benNode *node; 
        jstr key;

        node = malloc(sizeof(*node));
        if (!node) return BEN_ERR;
        r = decodeNextJstrBuffer(buf, &key);
        if (r != BEN_OK) {
            free(node);
            return r;
        }
        r = decodeNextBuffer(buf, node);
        if (r != BEN_OK) {
            free(node);
            return r;
        }
        putValMap(map, key, node);
        if (MAP_ALLOC_FAILED(map)) {
            free(node);
            destroyJstr(key);
            return BEN_ERR;
        }
    }
    buf->pos++;

    return BEN_OK;
}

static int decodeNextBuffer(struct buffer *buf, struct benNode *node)
{
    int r;
    unsigned char *data = JSTR(buf->data);

    switch (data[buf->pos]) {
    case 'l': {
        node->type = BEN_TYPE_LIST;
        struct list *list = newList(16, benDestroy);
        if (!list) return BEN_ERR;
        node->value.l = list;
        r = decodeNextListBuffer(buf, node->value.l);
        if (r != BEN_OK) {
            destroyList(list);
            return r;
        }
        return BEN_OK;
    }
    case 'd': {
        node->type = BEN_TYPE_MAP;
        struct map *map = newMap(16, 0.75, benDestroy);
        if (!map) return BEN_ERR;
        node->value.m = map;
        r = decodeNextMapBuffer(buf, node->value.m);
        if (r != BEN_OK) {
            destroyMap(map);
            return r;
        }
        return BEN_OK;
    }
    case 'i':
        node->type = BEN_TYPE_INT;
        return decodeNextI64Buffer(buf, &node->value.i);
    default:
        node->type = BEN_TYPE_STR;
        r = decodeNextJstrBuffer(buf, &node->value.s);
        if (r != BEN_OK) {
            destroyJstr(node->value.s);
            return r;
        }
    }
}

int benDecode(struct benNode **node, jstr data)
{
    int r;
    struct buffer buf;

    buf.pos = 0;
    buf.len = lenJstr(data);
    buf.data = data;

    *node = malloc(sizeof(**node));
    if (!(*node)) return BEN_ERR;
    r = decodeNextBuffer(&buf, *node);
    if (r != BEN_OK) benDestroy(*node);
    return r;
}

void benDestroy(void *node)
{
    if (!node) return;

    struct benNode *n = (struct benNode *)node;
    switch (n->type) {
    case BEN_TYPE_INT:
        break;
    case BEN_TYPE_STR:
        if (n->value.s) {
            destroyJstr(n->value.s);
            n->value.s = NULL;
        }
        break;
    case BEN_TYPE_LIST:
        if (n->value.l) {
            destroyList(n->value.l);
            n->value.l = NULL;
        }
        break;
    case BEN_TYPE_MAP:
        if (n->value.m) {
            destroyMap(n->value.m);
            n->value.m = NULL;
        }
        break;
    default:
        break;
    }
    free(n);
}

