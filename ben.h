#ifndef BEN_H
#define BEN_H

#include "common.h"
#include "jstr.h"
#include "list.h"
#include "map.h"

#define BEN_OK 1
#define BEN_ERR 0

#define BEN_TYPE_I64 0 
#define BEN_TYPE_JSTR 1
#define BEN_TYPE_LIST 2
#define BEN_TYPE_MAP 3

#define benIsInt(node) \
    (((struct benNode *)(node))->type == BEN_TYPE_I64)
#define benIsJstr(node) \
    (((struct benNode *)(node))->type == BEN_TYPE_JSTR)
#define benIsList(node) \
    (((struct benNode *)(node))->type == BEN_TYPE_LIST)
#define benIsMap(node) \
    (((struct benNode *)(node))->type == BEN_TYPE_MAP)

#define benAsI64(node) \
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

int benEncodeI64(jstr *data, i64 num);
int benEncodeJstr(jstr *data, jstr str);
int benEncodeList(jstr *data, struct list *list);
int benEncodeMap(jstr *data, struct map *map);
int benDecode(struct benNode *node, unsigned char *data, u64 dataLen);
int benEncode(jstr *data, struct benNode *node);

#endif
