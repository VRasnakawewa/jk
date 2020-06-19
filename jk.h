#ifndef JK_H
#define JK_H

#include "common.h"
#include "util.h"
#include "ben.h"
#include "list.h"
#include "map.h"
#include "urlencode.h"
#include "info.h"
#include "ev.h"
#include "reap.h"
#include "worker.h"

#define JK_CONFIG_DEBUG 1
#define JK_CONFIG_COMPACT "1"
#define JK_CONFIG_CLIENT_PORT "10001"
#define JK_CONFIG_MAX_ACTIVE_PEERS 30
#define JK_CONFIG_MAX_INACTIVE_PEERS 50

#define JK_EVENT_STARTED 0
#define JK_EVENT_STOPED 1
#define JK_EVENT_COMPLETED 2
static char *jk_events[] = {
    "started",
    "stoped",
    "completed"};
#define JK_EVENT_STR(event) jk_events[event]

struct jk {
    unsigned char id[20];
    unsigned char infoHash[20];
    int event;
    u64 uploaded;
    u64 downloaded;
    u64 total;
    u64 seeders;
    u64 leechers;
    char *trackerProto;
    char *trackerUrl;
    char *trackerId;
    u64 trackerIntervalSec;
    struct map *trackerResponse;
    struct map *workersInactive;
    struct map *workers;
    struct list *workQueue;
    struct map *meta;
};

/* Log types */
#define LT_ERROR 0
#define LT_WARNING 1
#define LT_INFO 2
#define LT_DEBUG 3
void jkLog(int type, const char *fmt, ...);

struct jk *jkNew(unsigned char *hash, char *trackerUrl, struct map *meta);
void jkDestroy(struct jk *jk);
/* jk non blocking */
void nbJkSendTrackerRequest(struct evLoop *loop, struct jk *jk);

#endif
