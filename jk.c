#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "common.h"
#include "jstr.h"
#include "reap.h"

static void onSuccess(struct evLoop *loop, char *res) { 
    printf("%s\n",res);
}
static void onError(struct evLoop *loop, char *err) {}

int main(int argc, char **argv)
{
    char *headers[] = {
        "Host","skoobee.herokuapp.com",
        "Accept-Encoding","gzip",
        "Connection","close", NULL};
    struct evLoop loop;
    initEvLoop(&loop);
    reap(&loop,
        "skoobee.herokuapp.com",
        "/api/docs",
        headers,
        NULL,
        onSuccess,
        onError);
    runEvLoop(&loop);
    return 0;
}
