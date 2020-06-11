#ifndef BEN_H
#define BEN_H

#include "common.h"
#include "jstr.h"
#include "list.h"
#include "map.h"

#define BEN_OK 0
#define BEN_ERR 1

#define BEN_TYPE_INT 0 
#define BEN_TYPE_JSTR 1
#define BEN_TYPE_LIST 2
#define BEN_TYPE_MAP 3

#define benAsInt(node) \
    (((struct benNode *)(node))->value.i)
#define benAsJstr(node) \
    (((struct benNode *)(node))->value.s)
#define benAsList(node) \
    (((struct benNode *)(node))->value.l)
#define benAsMap(node) \
    (((struct benNode *)(node))->value.m)

struct benNode {
    union {
        i64 i;
        jstr s;
        struct list *l;
        struct map *m;
    } value;
    int type;
};

int benDecode(struct benNode **node, unsigned char *data, u64 len);
void benDestroyBenNode(void *node);

#endif
