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
#include "piece.h"

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
    unsigned char id[20];         /* jk peer id */
    unsigned char infoHash[20];   /* info hash */
    int event;                    /* event (started|stoped|completed) */
    u64 uploaded;                 /* total uploaded bytes */
    u64 downloaded;               /* total downloaded bytes */
    u64 total;                    /* total bytes */
    u64 seeders;                  /* number of seeders according to tracker */
    u64 leechers;                 /* number of leechers according to tracker */
    char *trackerProto;           /* tracker protocol (http|udp) */
    char *trackerUrl;             /* tracker url */
    char *trackerId;              /* tracker id for next announcements */
    u64 trackerIntervalSec;       /* seconds jk should wait for the next tracker request */
    struct map *trackerResponse;  /* decoded tracker response */
    struct map *workersInactive;  /* set of inactive workers (no memory allocated) */
    struct map *workers;          /* active workers */
    struct map *meta;             /* decoded torrent file */
};

/* Log types */
#define LT_ERROR 0
#define LT_WARNING 1
#define LT_INFO 2
#define LT_DEBUG 3

#define jkLog(...) jkLogFunc(__FILE__, __LINE__, __VA_ARGS__)
void jkLogFunc(char *file, int line, int type, const char *fmt, ...);

struct jk *jkNew(unsigned char *hash, char *trackerUrl, struct map *meta);
void jkDestroy(struct jk *jk);
/* jk non blocking */
void nbJkSendTrackerRequest(struct evLoop *loop, struct jk *jk);

#endif
