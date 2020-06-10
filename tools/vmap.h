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
                printf("%s -> ", (JSTR_VALID(e->key)) ? "[jstr]" : e->key);
            else
                l = e;
            e = e->next;
        }
        if (l)
            printf("%s", (JSTR_VALID(e->key)) ? "[jstr]" : e->key);
        printf("}\n");
    }
}

#endif
