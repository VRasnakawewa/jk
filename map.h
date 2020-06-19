#ifndef MAP_H
#define MAP_H

#include <stdio.h>
#include "common.h"

struct mapEntry {
    char *key;
    void *val;
    struct mapEntry *next;
};

struct map {
    u64 cap;    
    u64 count;
    u64 threshold;
    float loadFactor;
    void (*destroyValFn)(void *val);
    struct mapEntry **table;
};

struct mapIterator {
    u64 pos;
    struct mapEntry *cur;
    struct mapEntry **table;
};

#define MAP_ALLOC_FAILED(map) (!(map) || (!(map)->table))

struct map *mapNew(u64 cap,
                   float loadFactor,
                   void (*destroyValFn)(void *val));
void mapDestroy(struct map *map);
void *mapPut(struct map *map, char *key, void *val);
void *mapGet(struct map *map, char *key);
int mapRemove(struct map *map, char *key);
int mapHas(struct map *map, char *key);
u64 mapSize(struct map *map);

static inline void mapIteratorInit(struct mapIterator *iter, struct map *map)
{
    iter->cur = NULL;
    iter->pos = map->cap;
    iter->table = map->table;
}
struct mapEntry *mapIteratorNext(struct mapIterator *iter);


#endif
