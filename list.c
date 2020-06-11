#include <stdlib.h>

#include "common.h"
#include "list.h"

struct list *listNew(u64 cap, void (*destroyValFn)(void *val))
{
    struct list *list;
    void **values;

    list = malloc(sizeof(*list));
    if (!list) return NULL;
    values = malloc(sizeof(*values)*cap);
    if (!values) {
        free(list);
        return NULL;
    }
    list->values = values;
    list->len = 0;
    list->cap = cap;
    list->destroyValFn = destroyValFn;
    return list;
}

void listDestroy(struct list *list)
{
    if (!list) return;
    if (list->destroyValFn) {
        while (list->len-- > 0)
            list->destroyValFn(list->values[list->len]);
    }
    free(list->values);
    free(list);
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

void listAdd(struct list *list, void *val)
{
    if (list->len >= list->cap) {
        _growList(list);
        if (LIST_ALLOC_FAILED(list))
            return;
    }
    list->values[list->len++] = val;
}

