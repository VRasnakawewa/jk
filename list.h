#ifndef LIST_H
#define LIST_H

#include "common.h"

struct list {
    u64 cap;
    u64 len;
    void **values;
    void (*destroyValFn)(void *val);
};

#define LIST_ALLOC_FAILED(list) (!(list)->values)

void initList(struct list *list, u64 cap, void (*destroyValFn)(void *val));
void destroyList(struct list *list);
void addValList(struct list *list, void *val);

#endif
