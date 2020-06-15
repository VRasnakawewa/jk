#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include "jk.h"

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
    jk->trackerResponse = NULL;
    jk->peers = NULL;
    jk->clients = NULL;
    jk->meta = meta;
    return jk;
}

void jkDestroy(struct jk *jk)
{
    listDestroy(jk->peers);
    mapDestroy(jk->trackerResponse);
    mapDestroy(jk->meta);
    free(jk);
}

void createClients(struct jk *jk)
{
}

void unmarshalTrackerResponse(struct jk *jk)
{
}

static int verifyTrackerResponse(struct map *res)
{
    return 1;
}

static void nbOnTrackerResponse(struct evLoop *loop,
                                void *callerData,
                                char *response,
                                size_t responseLen,
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
    char *body;
    if (!strstr(response, "HTTP/1.1 200 OK"))
        goto error;
    body = strstr(response, "\r\n\r\n");
    if (!body)
        goto error;
    body += 4;

    struct benNode res;
    if (benDecode(&res, body, responseLen - (body - response)) != BEN_OK)
        goto error;

    
error:
    free(response);
    stopEvLoop(loop);
}

void nbJkSendTrackerRequest(struct evLoop *loop, struct jk *jk)
{
    char infoHash[URLENCODE_DEST_LEN(sizeof(jk->infoHash))];
    urlencode(infoHash, jk->infoHash, sizeof(jk->infoHash));

    char peerId[URLENCODE_DEST_LEN(sizeof(jk->id))];
    urlencode(peerId, jk->id, sizeof(jk->id));

    char *port = JK_CONFIG_CLIENT_PORT;

    char uploaded[u64strlen(jk->uploaded)+1];
    u64str(uploaded, jk->uploaded);

    char downloaded[u64strlen(jk->downloaded)+1];
    u64str(downloaded, jk->downloaded);

    char left[u64strlen(jk->total - jk->downloaded)+1];
    u64str(left, jk->total - jk->downloaded);

    char *compact = JK_CONFIG_COMPACT;

    char *event = JK_EVENT_STR(jk->event);

    char trackerHostname[strlen(jk->trackerUrl) + 1];
    strncpy(trackerHostname, jk->trackerUrl, sizeof(trackerHostname));

    char *trackerPath;
    strtok_r(trackerHostname, "/", &trackerPath);

    char *trackerPort;
    strtok_r(trackerHostname, ":", &trackerPort);

    char *headers[] = {
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
             nbOnTrackerResponse);
}

static int verifyMetaStructure(struct map *meta)
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
    struct benNode meta;
    jstr announce;
    unsigned char infoHash[20];
    struct jk *jk = NULL;

    metaRaw = readFile(filename);
    if (!metaRaw) {
        jkLogError(strerror(errno));
        return 1;
    }
    if (benDecode(&meta, JSTR(metaRaw), lenJstr(metaRaw)) != BEN_OK) {
        jkLogError((errno) ? strerror(errno) : "Couldn't decode the meta file");
        destroyJstr(metaRaw);
        return 1;
    }
    if (!benIsMap(&meta) || !verifyMetaStructure(benAsMap(&meta))) {
        jkLogError("Invalid or corrupted meta file");
        destroyJstr(metaRaw);
        if (benIsMap(&meta)) mapDestroy(benAsMap(&meta));
        return 1;
    }
    announce = benAsJstr(mapGet(benAsMap(&meta), "announce"));
    calculateInfoHash(infoHash, JSTR(metaRaw), lenJstr(metaRaw));
    destroyJstr(metaRaw);
    jk = jkNew(infoHash, JSTR(announce), benAsMap(&meta));
    if (!jk) {
        jkLogError(strerror(errno));
        mapDestroy(benAsMap(&meta));
        return 1;
    }
    struct evLoop loop;
    initEvLoop(&loop);

    nbJkSendTrackerRequest(&loop, jk);

    runEvLoop(&loop);

    jkDestroy(jk);
    return 0;
}

