#ifndef LIST_H
#define LIST_H

#include "common.h"

struct list {
    u64 cap;
    u64 len;
    void **values;
    void (*destroyValFn)(void *val);
};

#define LIST_ALLOC_FAILED(list) (!(list) || !(list)->values)

struct list *listNew(u64 cap, void (*destroyValFn)(void *val));
void listDestroy(struct list *list);
void listAdd(struct list *list, void *val);

#endif
