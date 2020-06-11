#ifndef TRACKER_H
#define TRACKER_H

#include "common.h"
#include "stack.h"
#include "util.h"
#include "jstr.h"
#include "list.h"
#include "map.h"
#include "ben.h"
#include "urlencode.h"

#define TRACKER_OK 0
#define TRACKER_ERR_ALLOC 1
#define TRACKER_ERR_INVALID_META 2

int calculateInfoHash(unsigned char *hash, unsigned char *metaRaw, u64 metaRawLen);

#endif
