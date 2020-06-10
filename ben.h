#ifndef BEN_H
#define BEN_H

#include "common.h"
#include "jstr.h"
#include "list.h"
#include "map.h"

#define BEN_OK 0
#define BEN_ERR 1

#define BEN_TYPE_INT 0 
#define BEN_TYPE_STR 1
#define BEN_TYPE_LIST 2
#define BEN_TYPE_MAP 3

#define BEN_AS_INT(node) \
    (((struct benNode *)(node))->value.i)
#define BEN_AS_STR(node) \
    (((struct benNode *)(node))->value.s)
#define BEN_AS_LIST(node) \
    (((struct benNode *)(node))->value.l)
#define BEN_AS_MAP(node) \
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

int benDecode(struct benNode **node, jstr data);
void benDestroy(void *node);

#endif
