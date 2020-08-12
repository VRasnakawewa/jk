/*
 * MIT License
 * 
 * Copyright (c) 2020 Sujanan Bhathiya
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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
#include <fcntl.h>

#include "jk.h"

void jkLogFunc(char *file, int line, int type, const char *fmt, ...)
{
    if (type == LT_DEBUG && !JK_CONFIG_DEBUG) return;

    va_list ap;
    char msg[1024];
    
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    
    switch (type) {
    case LT_ERROR:
        fprintf(stderr, "ERROR (%s:%d): %s\n", file, line, msg);
        break;
    case LT_WARNING:
        printf("WARNING: %s\n", msg);
        break;
    case LT_INFO:
        printf("INFO: %s\n", msg);
        break;
    default:
        printf("DEBUG (%s:%d): %s\n", file, line, msg);
        break;
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
    jk->workersInactive = NULL;
    jk->workers = NULL;
    jk->meta = meta;
    return jk;
}

void jkDestroy(struct jk *jk)
{
    mapDestroy(jk->trackerResponse);
    mapDestroy(jk->workersInactive);
    mapDestroy(jk->workers);
    mapDestroy(jk->meta);
    free(jk);
}

static struct list *unmarshalPeers(jstr peers, int family)
{
    u64 peersize = (family == AF_INET6) ? 18 : 6;

    if (lenJstr(peers) % peersize != 0) {
        jkLog(LT_WARNING, "Received malformed peers");
        return NULL;
    }

    struct list *list = listNew(lenJstr(peers) / peersize, free);
    if (!list) return NULL;

    for (u64 i = 0; i < lenJstr(peers); i += peersize) {
        unsigned char ipvx[peersize-2];
        unsigned char port[2];
        memcpy(ipvx, peers + i, peersize-2);
        memcpy(port, peers + i + peersize-2, 2);

        char ipvxstr[INET6_ADDRSTRLEN];
        inet_ntop(family, ipvx, ipvxstr, sizeof(ipvxstr));
        char portstr[5+1];
        snprintf(portstr, sizeof(portstr), "%d", ntohs((*(int *)port)));

        u64 workerIdLen = 
            strlen(ipvxstr) +
            strlen(portstr) +
            1 + 
            1;
        char *workerId = malloc(workerIdLen);
        if (!workerId) {
            listDestroy(list);
            return NULL; 
        }
        snprintf(workerId, workerIdLen, "%s:%s", ipvxstr, portstr);
        listAdd(list, workerId);
        if (LIST_ALLOC_FAILED(list))
            return NULL;
    }

    return list;
}

static int verifyTrackerResponse(struct map *res)
{
    return 1;
}

static int createWorkers(struct jk *jk, struct list *peers)
{
    for (u64 i = mapSize(jk->workers);
        i < JK_CONFIG_MAX_ACTIVE_PEERS && !listIsEmpty(peers) ; i++)
    {
        unsigned char *id = listPop(peers);
        if (!mapHas(jk->workers, id)) { 
            struct worker *w = workerNew(id, jk);
            if (!w) return 0;
            mapPut(jk->workers, id, w);
            if (MAP_ALLOC_FAILED(jk->workers)) {
                workerDestroy(w);
                return 0;
            }
        }
    }
    return 1;
}

static int createInactiveWorkers(struct jk *jk, struct list *peers)
{
    for (u64 i = mapSize(jk->workersInactive); 
        i < JK_CONFIG_MAX_INACTIVE_PEERS && !listIsEmpty(peers); i++)
    {
        unsigned char *id = listPop(peers);
        if (!mapHas(jk->workersInactive, id)) {
            mapPut(jk->workersInactive, id, NULL);
            if (MAP_ALLOC_FAILED(jk->workersInactive))
                return 0;
        }
    }
    return 1;
}

static void nbOnTrackerResponse(struct evLoop *loop,
                                void *callerData,
                                char *response,
                                size_t responseLen,
                                int errcode,
                                reapStrerrorFn *reapStrerror)
{
    struct jk *jk = (struct jk *)callerData;

    if (errcode == EAI_AGAIN ||
        errcode == EAGAIN ||
        errcode == ENETUNREACH ||
        errcode == ETIMEDOUT) {
        jkLog(LT_DEBUG, "Tracker request failed. Trying again");
        return; /* retry */
    }
    if (errcode) {
        jkLog(LT_ERROR, "Tracker request failed: %s", reapStrerror(errcode));
        stopEvLoop(loop); /* cannot recover */
        return;
    }
        
    if (!strstr(response, "HTTP/1.1 200 OK")) {
        jkLog(LT_DEBUG, "Tracker request failed. Trying again");
        return; /* retry */
    }

    char *body = strstr(response, "\r\n\r\n");
    if (!body) {
        jkLog(LT_DEBUG, "Tracker request failed. Trying again");
        return; /* retry */
    }
    body += 4;

    struct benNode res;
    if (benDecode(&res, body, responseLen - (body - response)) != BEN_OK) {
        jkLog(LT_DEBUG, "Invalid bencode format in tracker response. Trying again");
        return; /* retry */
    }
    if (!benIsMap(&res) || !verifyTrackerResponse(benAsMap(&res))) {
        jkLog(LT_DEBUG, "Invalid bencode format in tracker response. Trying again");
        if (benIsMap(&res)) mapDestroy(benAsMap(&res));
        return; /* retry */
    }
    mapDestroy(jk->trackerResponse); /* destroy existing tracker response */
    jk->trackerResponse = benAsMap(&res);

    struct benNode *peers4ben = mapGet(benAsMap(&res), "peers");
    struct benNode *peers6ben = mapGet(benAsMap(&res), "peers6");
    struct list *peers4 = NULL;
    struct list *peers6 = NULL;
    if (peers4ben)
        peers4 = unmarshalPeers(benAsJstr(peers4ben), AF_INET);
    if (peers6ben)
        peers6 = unmarshalPeers(benAsJstr(peers6ben), AF_INET6);
    if ((peers4ben && !peers4) || (peers6ben && !peers6)) {
        jkLog(LT_ERROR, strerror(errno));
        stopEvLoop(loop);
        return;
    }

    if (!jk->workersInactive) {
        jk->workersInactive = mapNew(JK_CONFIG_MAX_INACTIVE_PEERS, 1.0, free, NULL);
        if (!jk->workersInactive) goto enomem;
    }

    if (!jk->workers) {
        jk->workers = mapNew(JK_CONFIG_MAX_ACTIVE_PEERS, 1.0, free, workerDestroy);
        if (!jk->workers) goto enomem;
    }

    if (peers4 && !createWorkers(jk, peers4))
        goto enomem;
    if (peers6 && !createWorkers(jk, peers6))
        goto enomem;
    if (peers4 && !createInactiveWorkers(jk, peers4))
        goto enomem;
    if (peers6 && !createInactiveWorkers(jk, peers6))
        goto enomem;

    struct mapIterator it; 
    mapIteratorInit(&it, jk->workers);
    struct mapEntry *e = mapIteratorNext(&it);
    while (e) {
        nbWorkerConnect(loop, (struct worker *)e->val);
        e = mapIteratorNext(&it);
    }
 
    listDestroy(peers4);
    listDestroy(peers6);
    return;

enomem:
    listDestroy(peers4);
    listDestroy(peers6);
    jkLog(LT_ERROR, strerror(errno));
    stopEvLoop(loop);
    return;
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
    if (benDecode(&meta, metaRaw, lenJstr(metaRaw)) != BEN_OK) {
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
    calculateInfoHash(infoHash, metaRaw, lenJstr(metaRaw));
    destroyJstr(metaRaw);
    jk = jkNew(infoHash, announce, benAsMap(&meta));
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

