#include <stdlib.h>

#include "common.h"
#include "list.h"

void initList(struct list *list, u64 cap, void (*destroyValFn)(void *val))
{
    void **values;

    values = malloc(sizeof(*values));
    if (!values) return;
    list->values = values;
    list->len = 0;
    list->cap = cap;
    list->destroyValFn = destroyValFn;
}

void destroyList(struct list *list)
{
    if (list->destroyValFn) {
        while (list->len-- > 0)
            list->destroyValFn(list->values[list->len]);
    }
    free(list->values);
}

static void _resizeList(struct list *list, u64 newcap)
{
    list->values = realloc(list->values, sizeof(*list->values)*newcap);
    if (!list->values)
        return;
    list->cap = newcap;
}

static void _growList(struct list *list)
{
    _resizeList(list,
        list->cap + (list->cap >> 1));
}

void addValList(struct list *list, void *val)
{
    if (list->len >= list->cap) {
        _growList(list);
        if (LIST_ALLOC_FAILED(list))
            return;
    }
    list->values[list->len++] = val;
}

