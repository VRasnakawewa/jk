#ifndef INFO_H
#define INFO_H

#include "common.h"
#include "map.h"
#include "list.h"
#include "ben.h"

int infoVerify(struct map *info);
i64 infoGetTotalBytes(struct map *info);

#endif
