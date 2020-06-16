#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "map.h"
#include "jstr.h"

static struct mapEntry *newMapEntry(char *key,
                                    void *val,
                                    struct mapEntry *next)
{
    struct mapEntry *e;

    e = malloc(sizeof(*e));
    if (!e) return NULL;
    e->key = key; 
    e->val = val;
    e->next = next;
    return e;
}

static void destroyKey(char *key)
{
    if (JSTR_VALID(key)) destroyJstr(key); else free(key);
}

struct map *mapNew(u64 cap,
                   float loadFactor,
                   void (*destroyValFn)(void *val))
{
    struct map *map;

    map = malloc(sizeof(*map));
    if (!map) return NULL;
    map->cap = cap;
    map->count = 0;
    map->loadFactor = loadFactor;
    map->threshold = (u64)(cap * loadFactor);
    map->destroyValFn = destroyValFn;
    map->table = malloc(sizeof(*map->table)*cap);
    if (!map->table) {
        free(map);
        return NULL;
    }
    memset(map->table, 0, sizeof(*map->table)*cap);
    return map;
}

void mapDestroy(struct map *map)
{
    if (!map) return;

    for (u64 i = 0; i < map->cap; i++) {
        struct mapEntry *e = map->table[i];
        while (e) {
            struct mapEntry *trash = e;
            destroyKey(trash->key);
            if (map->destroyValFn)
                map->destroyValFn(trash->val);
            e = e->next;
            free(trash);
        }
    }

    free(map->table);
    free(map);
}

static u64 djb2Hash(unsigned char *s, u64 len)
{
    u64 h = 5381;
    while (len--)
        h = ((h << 5) + h) ^ (*s++);
    return h;
}

static inline u64 genHash(char *s)
{
    return djb2Hash((unsigned char *)JSTR(s), lenJstr(s));
}

static void _rehashMap(struct map *map)
{
    if (map->cap == UINT64_MAX)
        return;
    u64 newcap = (map->cap > (UINT64_MAX >> 1)) ? UINT64_MAX : map->cap << 1;

    struct mapEntry **newtab;

    newtab = malloc(sizeof(*newtab)*newcap);
    if (!newtab) {
        mapDestroy(map);
        map->table = NULL;
        return;
    }
    memset(newtab, 0, sizeof(*newtab)*newcap);

    for (u64 i = 0; i < map->cap; i++) {
        struct mapEntry *e = map->table[i];

        while (e) {
            struct mapEntry *old = e;
            e = e->next;
            u64 index = genHash(old->key) % newcap;
            old->next = newtab[index];
            newtab[index] = old;
        }
    }

    free(map->table);
    map->cap = newcap;
    map->threshold = (u64)(newcap * map->loadFactor);
    map->table = newtab;
}

void *mapPut(struct map *map, char *key, void *val)
{
    u64 index = genHash(key) % map->cap;

    struct mapEntry *e = map->table[index];
    while (e) {
        if (!cmpJstr(key, e->key)) {
            void *oldval = e->val;
            e->val = val;
            destroyKey(key);
            return oldval;
        }
        e = e->next;
    }

    if (map->count >= map->threshold) {
        _rehashMap(map);
        if (MAP_ALLOC_FAILED(map)) {
            destroyKey(key);
            return NULL;
        }
    }

    struct mapEntry *new = newMapEntry(key, val, map->table[index]);
    if (!new) {
        mapDestroy(map);
        destroyKey(key);
        map->table = NULL;
        return NULL;
    }
    map->table[index] = new;
    map->count++;

    return NULL;
}

void *mapGet(struct map *map, char *key)
{
    struct mapEntry *e = map->table[genHash(key) % map->cap];

    while (e) {
        if (!cmpJstr(key, e->key))
            return e->val;
        e = e->next;
    }
    return NULL;
}

struct mapEntry *mapIteratorNext(struct mapIterator *it)
{
    while (!it->cur && it->pos > 0)
        it->cur = it->table[--it->pos];

    struct mapEntry *e = it->cur;
    if (it->cur)
        it->cur = it->cur->next;

    return e;
}

