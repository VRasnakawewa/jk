/*
 * MIT License
 * 
 * Copyright (c) 2020 Sujanan Bhathiya
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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

int listIsEmpty(struct list *list)
{
    return list->len == 0;
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
        if (LIST_ALLOC_FAILED(list)) {
            listDestroy(list);
            return;
        }
    }
    list->values[list->len++] = val;
}

void *listPop(struct list *list)
{
    if (list->len == 0) return NULL;
    return list->values[--list->len];
}

