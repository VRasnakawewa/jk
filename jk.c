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

#define JK_CONFIG_COMPACT 0
#define JK_CONFIG_CLIENT_PORT 6882

#define JK_EVENT_STARTED 0
#define JK_EVENT_STOPED 1
#define JK_EVENT_COMPLETED 2

struct jk {
    unsigned char id[20];
    unsigned char infoHash[20];
    int event;
    u64 uploaded;
    u64 downloaded;
    u64 seeders;
    u64 leechers;
    char *trackerUrl;
    char *trackerId;
    u64 trackerIntervalSec;
    struct list *peers;
    struct map *clients;
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

struct jk *jkNew(unsigned char *id, char *trackerUrl, struct map *meta)
{
    struct jk *jk;

    jk = malloc(sizeof(*jk));
    if (!jk) return NULL;
    generatePeerId(jk->id);
    strncpy(jk->id, id, sizeof(jk->id));
    jk->event = JK_EVENT_STARTED;
    jk->uploaded = 0;
    jk->downloaded = 0;
    jk->seeders = 0; 
    jk->leechers = 0;
    jk->trackerUrl = trackerUrl;
    jk->trackerId = NULL;
    jk->trackerIntervalSec = UINT64_MAX;
    jk->peers = NULL;
    jk->clients = NULL;
    return jk;
}

void jkDestroy(struct jk *jk) { free(jk); }

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: jk <torrent>\n");
        return 1;
    }
    int exitcode = 0;
    char *filename = argv[1];
    jstr metaRaw;  
    struct benNode *meta;
    struct benNode *announce;
    unsigned char infoHash[20];
    struct jk *jk = NULL;

    metaRaw = readFile(filename);
    if (!metaRaw) {
        jkLogError(strerror(errno));
        return 1;
    }
    if (benDecode(&meta, metaRaw) != BEN_OK) {
        jkLogError((errno) ? strerror(errno) : "Couldn't decode the meta file");
        exitcode = 1;
        goto clear;
    }
    if (!benIsMap(meta)) {
        jkLogError("Invalid or corrupted meta file");
        exitcode = 1;
        goto clear;
    }
    announce = mapGet(benAsMap(meta), "announce");
    if (!announce || !benIsJstr(announce)) {
        jkLogError("Invalid or corrupted meta file");
        exitcode = 1;
        goto clear;
    }
    if (!calculateInfoHash(infoHash, JSTR(metaRaw), lenJstr(metaRaw))) {
        jkLogError((errno) ? strerror(errno) : "Corrupted info file structure in meta file");
        exitcode = 1;
        goto clear;
    }

    jk = jkNew(infoHash, JSTR(benAsJstr(announce)), benAsMap(meta));
    if (!jk) {
        jkLogError(strerror(errno));
        exitcode = 1;
        goto clear;
    }

clear:
    benDestroyBenNode(meta);
    destroyJstr(metaRaw);
    jkDestroy(jk);
    return exitcode;
}

