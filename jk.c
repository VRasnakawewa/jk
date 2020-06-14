#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include "common.h"
#include "util.h"
#include "ben.h"
#include "list.h"
#include "map.h"
#include "urlencode.h"
#include "info.h"
#include "reap.h"

#define JK_CONFIG_COMPACT "1"
#define JK_CONFIG_CLIENT_PORT "6882"

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
    struct list *peers;
    struct map *clients;
    struct map *meta;
};

void jkLogError(const char *fmt, ...)
{
    va_list ap;
    char msg[1024];
    
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    fprintf(stderr, "error: %s\n", msg);
}

struct jk *jkNew(unsigned char *hash, char *trackerUrl, struct map *meta)
{
    struct jk *jk;

    jk = malloc(sizeof(*jk));
    if (!jk) return NULL;
    generatePeerId(jk->id);
    strncpy(jk->infoHash, hash, sizeof(jk->infoHash));
    jk->event = JK_EVENT_STARTED;
    jk->uploaded = 0;
    jk->downloaded = 0;
    jk->total = infoGetTotalBytes(benAsMap(mapGet(meta, "info")));
    jk->seeders = 0; 
    jk->leechers = 0;
    jk->trackerProto = trackerUrl;
    strtok_r(jk->trackerProto, "://", &trackerUrl);
    trackerUrl += 2;
    jk->trackerUrl = trackerUrl;
    jk->trackerId = NULL;
    jk->trackerIntervalSec = UINT64_MAX;
    jk->peers = NULL;
    jk->clients = NULL;
    jk->meta = meta;
    return jk;
}

void jkDestroy(struct jk *jk) { free(jk); }

void jkOnTrackerResponse(struct evLoop *loop,
                         void *callerData,
                         char *response,
                         int error,
                         const char *errMsg)
{
    struct jk *jk = (struct jk *)callerData;

    if (error) {
        jkLogError(errMsg);
        if (response) free(response);
        stopEvLoop(loop);
        return;
    }
    printf("%s\n", response);
}

void jkSendTrackerRequest(struct evLoop *loop, struct jk *jk)
{
    char infoHash[URLENCODE_DEST_LEN(sizeof(jk->infoHash))];
    urlencode(infoHash, jk->infoHash, sizeof(jk->infoHash));

    char peerId[URLENCODE_DEST_LEN(sizeof(jk->id))];
    urlencode(peerId, jk->id, sizeof(jk->id));

    char *port = JK_CONFIG_CLIENT_PORT;

    char uploaded[snprintf(NULL,0,"%lu",jk->uploaded) + 1];
    snprintf(uploaded, sizeof(uploaded), "%lu", jk->uploaded);

    char downloaded[snprintf(NULL,0,"%lu",jk->downloaded) + 1];
    snprintf(downloaded, sizeof(downloaded), "%lu", jk->downloaded);

    char left[snprintf(NULL,0,"%lu",jk->total-jk->downloaded) + 1];
    snprintf(left, sizeof(left), "%lu", jk->total-jk->downloaded);

    char *compact = JK_CONFIG_COMPACT;

    char *event = JK_EVENT_STR(jk->event);

    char trackerHostname[strlen(jk->trackerUrl) + 1];
    strncpy(trackerHostname, jk->trackerUrl, sizeof(trackerHostname));

    char *p;
    strtok_r(trackerHostname, "/", &p);
    char trackerPath[strlen(p)+1+1];
    snprintf(trackerPath, sizeof(trackerPath), "/%s", p);

    char *trackerPort;
    strtok_r(trackerHostname, ":", &trackerPort);

    char trackerHost[strlen(trackerHostname) + strlen(trackerPort) + 1 + 1];
    snprintf(trackerHost, sizeof(trackerHost),
        "%s:%s", trackerHostname, trackerPort);

    char *headers[] = {
        "Host",trackerHost,
        "Connection","close",
        NULL
    };
    char *params[] = {
        "info_hash",infoHash,
        "peer_id",peerId,
        "port",port,
        "uploaded",uploaded,
        "downloaded",downloaded,
        "left",left,
        "compact",compact,
        "event",event,
        NULL
    }; 
    reap(loop,
             jk, 
             trackerHostname,
             trackerPort,
             trackerPath,
             headers,
             params,
             jkOnTrackerResponse);
}

int jkVerifyMetaStructure(struct map *meta)
{
    return 1;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: jk <torrent>\n");
        return 1;
    }
    char *filename = argv[1];
    jstr metaRaw;  
    struct benNode *meta;
    jstr announce;
    unsigned char infoHash[20];
    struct jk *jk = NULL;

    metaRaw = readFile(filename);
    if (!metaRaw) {
        jkLogError(strerror(errno));
        return 1;
    }
    if (benDecode(&meta, metaRaw) != BEN_OK) {
        jkLogError((errno) ? strerror(errno) : "Couldn't decode the meta file");
        destroyJstr(metaRaw);
        return 1;
    }
    if (!benIsMap(meta) || !jkVerifyMetaStructure(benAsMap(meta))) {
        jkLogError("Invalid or corrupted meta file");
        destroyJstr(metaRaw);
        benDestroyBenNode(meta);
        return 1;
    }
    announce = benAsJstr(mapGet(benAsMap(meta), "announce"));
    calculateInfoHash(infoHash, JSTR(metaRaw), lenJstr(metaRaw));
    destroyJstr(metaRaw);
    jk = jkNew(infoHash, JSTR(announce), benAsMap(meta));
    if (!jk) {
        jkLogError(strerror(errno));
        benDestroyBenNode(meta);
        return 1;
    }
    struct evLoop loop;
    initEvLoop(&loop);

    jkSendTrackerRequest(&loop, jk);

    runEvLoop(&loop);

    benDestroyBenNode(meta);
    jkDestroy(jk);
    return 0;
}

