#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "jk.h"

void jkLog(int type, const char *fmt, ...)
{
    va_list ap;
    char msg[1024];
    
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    
    switch (type) {
    case LT_ERROR:
        fprintf(stderr, "ERROR: %s\n", msg); break;
    case LT_WARNING:
        printf("WARNING: %s\n", msg); break;
    default:
        printf("INFO: %s\n", msg);
    }
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
    mapDestroy(jk->clients);
    mapDestroy(jk->meta);
    free(jk);
}

static struct list *unmarshalPeers(jstr peers, int family)
{
    struct list *list;
    u64 peerSize = (AF_INET6) ? 18 : 6;

    list = listNew(lenJstr(peers)/peerSize, free);
    if (!list) return NULL;

    for (u64 i = 0; i < lenJstr(peers); i += peerSize) {
        unsigned char ipvX[peerSize-2];
        unsigned char port[2];
        memcpy(ipvX, peers + i, peerSize-2);
        memcpy(port, peers + i + peerSize-2, 2);

        char ipvXstr[INET6_ADDRSTRLEN];
        inet_ntop(family, ipvX, ipvXstr, sizeof(ipvXstr));
        /* todo: Verify ip address (check inet_ntop result) */
        char portstr[5+1];
        snprintf(portstr, sizeof(portstr),
            "%u", ntohs((port[0]<<8)+port[1]));
        /* todo: Verify port number (check range check) */

        u64 clientIdLen = 
            strlen(ipvXstr) +
            strlen(portstr) +
            1 + 
            1;
        char *clientId = malloc(clientIdLen);
        if (!clientId) {
            listDestroy(list);
            return NULL; 
        }
        snprintf(clientId, clientIdLen, "%s:%s", ipvXstr, portstr);
        listAdd(list, clientId);
        if (LIST_ALLOC_FAILED(list))
            return NULL;
    }

    return list;
}

static int verifyTrackerResponse(struct map *res)
{
    return 1;
}

#include "tools/vmap.h"
static void nbOnTrackerResponse(struct evLoop *loop,
                                void *callerData,
                                char *response,
                                size_t responseLen,
                                int error,
                                const char *errMsg)
{
    struct jk *jk = (struct jk *)callerData;

    if (error) {
        jkLog(LT_INFO, errMsg);
        if (response) free(response);
        stopEvLoop(loop);
        return;
    }
    char *body;
    if (!strstr(response, "HTTP/1.1 200 OK")) {
        jkLog(LT_ERROR, "Tracker request failed");
        goto error;
    }
    body = strstr(response, "\r\n\r\n");
    if (!body) {
        jkLog(LT_ERROR, "Tracker request failed");
        goto error;
    }
    body += 4;

    struct benNode res;
    if (benDecode(&res, body, responseLen - (body - response)) != BEN_OK) {
        jkLog(LT_ERROR, "Invalid tracker response");
        goto error;
    }
    if (!benIsMap(&res) || !verifyTrackerResponse(benAsMap(&res))) {
        jkLog(LT_ERROR, "Invalid tracker response");
        if (benIsMap(&res)) mapDestroy(benAsMap(&res));
        goto error;
    }
    jk->trackerResponse = benAsMap(&res);
    struct benNode *peersIpv4 = mapGet(benAsMap(&res), "peers");
    struct benNode *peersIpv6 = mapGet(benAsMap(&res), "peers6");

    /* todo: Either lists should be merged or maintain two lists */
    struct list *peers;
    if (peersIpv4)
        peers = unmarshalPeers(benAsJstr(peersIpv4), AF_INET);
    if (peersIpv6)
        peers = unmarshalPeers(benAsJstr(peersIpv6), AF_INET6);

    jk->clients = mapNew(peers->len, 1.0, clientDestroy);
    
    for (u64 i = 0; i < peers->len; i++) {
        unsigned char *id = peers->values[i];
        struct client *c = clientNew(id, jk);
        mapPut(jk->clients, id, c);
    }
    visualizeMap(jk->clients);

    /* Let the client map delete it's keys */
    peers->destroyValFn = NULL;
    listDestroy(peers);
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
        jkLog(LT_ERROR, strerror(errno));
        return 1;
    }
    if (benDecode(&meta, JSTR(metaRaw), lenJstr(metaRaw)) != BEN_OK) {
        jkLog(LT_ERROR, (errno) ? strerror(errno) : "Couldn't decode the meta file");
        destroyJstr(metaRaw);
        return 1;
    }
    if (!benIsMap(&meta) || !verifyMetaStructure(benAsMap(&meta))) {
        jkLog(LT_ERROR, "Invalid or corrupted meta file");
        destroyJstr(metaRaw);
        if (benIsMap(&meta)) mapDestroy(benAsMap(&meta));
        return 1;
    }
    announce = benAsJstr(mapGet(benAsMap(&meta), "announce"));
    calculateInfoHash(infoHash, JSTR(metaRaw), lenJstr(metaRaw));
    destroyJstr(metaRaw);
    jk = jkNew(infoHash, JSTR(announce), benAsMap(&meta));
    if (!jk) {
        jkLog(LT_ERROR, strerror(errno));
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

