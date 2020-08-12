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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>

#include "reap.h"

static char *defualtHttpHeaders[] = {NULL};
static char *defaultQueryParams[] = {NULL};

struct reapOpt {
    char *reqStr;
    size_t reqStrLen;
    size_t reqStrPos;
    char *resStr;
    size_t resStrLen;
    size_t resStrCap;
    void *callerData;
    onResponseFn *onResponse;
};

static size_t lenKeyValPairs(char **s)
{
    size_t len;
    for (len = 0; s[len]; len++);
    return len;
}

static inline size_t lenHeaders(char **h)
{
    return lenKeyValPairs(h);
}

static inline size_t lenParams(char **p)
{
    return lenKeyValPairs(p);
}

static char *buildHttpHeaders(char **s)
{
    size_t i = 0, cap = 1;   /* '\0' */
    while (s[i]) {
        cap += strlen(s[i++]); /* key */
        cap += 1;              /* : */
        cap += 1;              /* SP */
        cap += strlen(s[i++]); /* val */
        cap += 2;              /* \r\n */
    }
    char *h;

    h = malloc(cap);
    if (!h) return NULL;

    i = 0;
    size_t len = 0;
    while (s[i]) {
        char *key = s[i++];
        char *val = s[i++];
        len += snprintf(h+len, cap-len,
                "%s: %s\r\n", key, val);
    }
    h[len] = '\0';
    return h;
}

static char *buildQueryParams(char **s)
{
    size_t i = 0, cap = 1;   /* '\0' */
    while (s[i]) {
        cap += 1;              /* ? or & */
        cap += strlen(s[i++]); /* key */
        cap += 1;              /* = */
        cap += strlen(s[i++]); /* val */
    }
    char *q;

    q = malloc(cap);
    if (!q) return NULL;

    i = 0;
    size_t len = 0;
    while (s[i]) {
        char *key = s[i++];
        char *val = s[i++];
        char *fmt = (i == 2)
            ? "?%s=%s"  /* ?key=val */
            : "&%s=%s"; /* &key=val */
        len += snprintf(q+len,
                cap-len, fmt, key, val);
    }
    q[len] = '\0';
    return q;
}

#define HTTP_GET_REQUEST_FMT "GET %s%s HTTP/1.1\r\n%s\r\n"
struct reapOpt *newReapOpt(void *callerData,
                           char *host,
                           char *path,
                           char **headers,
                           char **params,
                           onResponseFn *onResponse)
{
    if (!path)
        path = "/";
    if (!headers)
        headers = defualtHttpHeaders;
    if (!params)
        params = defaultQueryParams;

    /* Add missing '/' to the path */
    char pathCopy[1 + strlen(path) + 1];
    if (*path != '/') {
        snprintf(pathCopy, sizeof(pathCopy), "/%s", path);
        path = pathCopy;
    }

    /* Add missing 'Host' header to the headers */
    size_t headersLen = lenHeaders(headers);
    char *headersCopy[2 + headersLen + 1];
    headersCopy[0] = "Host";
    headersCopy[1] = host;
    for (size_t i = 0; i < headersLen; i++)
        headersCopy[i+2] = headers[i];
    headersCopy[2 + headersLen] = NULL;
    headers = headersCopy;
    
    struct reapOpt *opt; 

    opt = malloc(sizeof(*opt));
    if (!opt) return NULL;
    opt->callerData = callerData;
    opt->onResponse = onResponse;
    opt->resStrLen = 0;
    opt->resStrCap = 8192;
    opt->resStr = malloc(opt->resStrCap);
    if (!opt->resStr) {
        free(opt);
        return NULL;
    }

    char *h, *q;

    h = buildHttpHeaders(headers);
    q = buildQueryParams(params);
    if (!q || !h) goto error;

    opt->reqStrPos = 0;
    opt->reqStrLen = 
        strlen(path) +
        strlen(HTTP_GET_REQUEST_FMT)-2-2-2 +
        strlen(q) + 
        strlen(h) + 1;
    opt->reqStr = malloc(opt->reqStrLen);

    if (!opt->reqStr)
        goto error;

    snprintf(opt->reqStr, opt->reqStrLen,
        HTTP_GET_REQUEST_FMT, path, q, h);

    goto clear;

error:
    free(opt);
    opt = NULL;
clear:
    free(q);
    free(h);
    return opt;
}

void destroyReapOpt(struct reapOpt *opt)
{
    if (opt) {
        free(opt->resStr);
        free(opt->reqStr);
        free(opt);
    }
}

static void onRecvReady(struct evLoop *loop,
        int sockfd, void *data, int mask)
{
    int errcode = 0;
    struct reapOpt *opt = (struct reapOpt *)data;

    int n = read(sockfd,
        opt->resStr + opt->resStrLen,
        opt->resStrCap - opt->resStrLen);
    if (n == -1) {
        errcode = errno; 
        goto error;
    }
    if (n == 0) {
        close(sockfd);
        removeFileEventEvLoop(loop, sockfd, mask);

        opt->resStr[opt->resStrLen] = '\0';
        opt->onResponse(loop, opt->callerData,
            opt->resStr, opt->resStrLen, 0, NULL);
        destroyReapOpt(opt);
        return;
    }

    opt->resStrLen += n;
    if (opt->resStrLen >= opt->resStrCap) {
        opt->resStr = realloc(opt->resStr,
            opt->resStrCap + (opt->resStrCap >> 1));
        if (!opt->resStr) {
            errcode = errno; 
            goto error;
        }
        opt->resStrCap += opt->resStrCap >> 1;
    }
    return;

error:
    removeFileEventEvLoop(loop, sockfd, mask);
    close(sockfd);
    opt->onResponse(loop, opt->callerData, NULL, 0, errcode, strerror);
    destroyReapOpt(opt);
}

static void onSendReady(struct evLoop *loop,
        int sockfd, void *data, int mask)
{
    int errcode = 0;
    struct reapOpt *opt = (struct reapOpt *)data;

    if (opt->reqStrPos == opt->reqStrLen) {
        removeFileEventEvLoop(loop, sockfd, mask);
        if (!addFileEventEvLoop(loop,
                sockfd,
                EV_READABLE,
                onRecvReady,
                opt)) {
            errcode = errno;
            goto error;
        }
        return;
    }

    int n = write(sockfd,
        opt->reqStr + opt->reqStrPos,
        opt->reqStrLen - opt->reqStrPos);
    if (n == -1) {
        errcode = errno;
        goto error;
    }
    opt->reqStrPos += n;
    return;

error:
    removeFileEventEvLoop(loop, sockfd, mask);
    close(sockfd);
    opt->onResponse(loop, opt->callerData, NULL, 0, errcode, strerror);
    destroyReapOpt(opt);
}

static void onConnect(struct evLoop *loop,
        int sockfd, void *data, int mask)
{
    int errcode = 0;
    struct reapOpt *opt = (struct reapOpt *)data;

    removeFileEventEvLoop(loop, sockfd, mask);
    if (!addFileEventEvLoop(loop,
            sockfd,
            EV_WRITABLE,
            onSendReady,
            opt)) {
        errcode = errno;
        goto error;
    }
    return;

error:
    removeFileEventEvLoop(loop, sockfd, mask);
    close(sockfd);
    opt->onResponse(loop, opt->callerData, NULL, 0, errcode, strerror);
    destroyReapOpt(opt);
}

void reap(struct evLoop *loop,
          void *callerData,
          char *hostname,
          char *port,
          char *path,
          char **headers,
          char **params,
          onResponseFn *onResponse)
{
    if (loop->stop) return;

    int r;
    int errcode = 0;
    struct addrinfo hints;
    struct addrinfo *info = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    r = getaddrinfo(hostname, (port) ? port : "80", &hints, &info);
    if (r) {
        onResponse(loop, callerData, NULL, 0,
                r, (reapStrerrorFn *)gai_strerror);
        return;
    }

    char host[strlen(hostname) + strlen(port) + 1 + 1];
    if (port)
        snprintf(host, sizeof(host), "%s:%s", hostname, port);
    else
        snprintf(host, sizeof(host), "%s", hostname);

    struct reapOpt *opt;

    opt = newReapOpt(
            callerData,
            host,
            path,
            headers,
            params,
            onResponse);
    if (!opt) {
        errcode = errno;
        goto error;
    }

    struct addrinfo *p;
    for (p = info; p; p = p->ai_next) {
        int sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) {
            errcode = errno;
            continue;
        }
        r = fcntl(sockfd, F_SETFL, O_NONBLOCK);
        if (r == -1) {
            errcode = errno;
            close(sockfd);
            continue;
        }
        r = connect(sockfd, p->ai_addr, p->ai_addrlen);
        if (r == -1 && errno != EINPROGRESS) {
            errcode = errno;
            close(sockfd);
            continue;
        }
        if (!addFileEventEvLoop(loop,
                sockfd,
                EV_WRITABLE,
                onConnect,
                opt)) {
            errcode = errno;
            close(sockfd);
            continue;
        }
        freeaddrinfo(info);
        return;
    }

error:
    freeaddrinfo(info);
    onResponse(loop, callerData, NULL, 0, errcode, strerror);
    destroyReapOpt(opt);
    return;
}

