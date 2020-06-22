#ifndef INFO_H
#define INFO_H

#include "common.h"
#include "map.h"
#include "list.h"
#include "ben.h"

#define INFO_MULI_FILE 0
#define INFO_SINGLE_FILE 1

struct infoFile {
    i64 length; 
    char *name;
    char *path;
};

struct infoFileIter {
    u64 pos;
    int mode;
    char *name;
    i64 length;
    struct list *files;
};

void infoFileIterInit(struct infoFileIter *it, struct map *info);
int infoFileIterNext(struct infoFileIter *it, struct infoFile *file);
int infoVerify(struct map *info);
int infoGetFileMode(struct map *info);
i64 infoGetTotalBytes(struct map *info);

#endif
