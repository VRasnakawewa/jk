#ifndef MAP_H
#define MAP_H

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

#define MAP_ALLOC_FAILED(map) (!(map) || (!(map)->table))

struct map *newMap(u64 cap,
                   float loadFactor,
                   void (*destroyValFn)(void *val));
void destroyMap(struct map *map);
void *putValMap(struct map *map, char *key, void *val);
void *getValMap(struct map *map, char *key);

#endif
