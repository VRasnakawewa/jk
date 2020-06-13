#ifndef VMAP_H
#define VMAP_H

#include <stdio.h>
#include <stdlib.h>

#include "../map.h"
#include "../jstr.h"

static void visualizeMap(struct map *map)
{
    for (u64 i = 0; i < map->cap; i++) {
        struct mapEntry *e = map->table[i];
        struct mapEntry *l = NULL; 
        printf("{");
        while (e) {
            if (e->next)
                printf("%s ->", JSTR(e->key));
            else
                printf("%s", JSTR(e->key));
            e = e->next;
        }
        printf("}\n");
    }
}

#endif
