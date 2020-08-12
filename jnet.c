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
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <assert.h>

#include "jnet.h"

int jnetResolvePassive(struct addrinfo **info, const char *port)
{
    struct addrinfo hints; 

    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    return getaddrinfo(NULL, port, &hints, info);
}

int jnetResolveNumericHost(struct addrinfo **info,
                           const char *ip,
                           const char *port)
{
    struct addrinfo hints; 

    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICHOST;

    return getaddrinfo(ip, port, &hints, info);
}

int jnetGetSocketErrno(int fd)
{
    int err = 0;
    int len = 1;
    assert(getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) != -1);
    return err;
}

int jnetSocket(struct addrinfo *info)
{
    int fd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
    if (fd == -1) return fd;
    int r = fcntl(fd, F_SETFL, O_NONBLOCK);
    if (r == -1) return r;
    return fd;
}

int jnetBind(int fd, struct addrinfo *info)
{
    return bind(fd, info->ai_addr, info->ai_addrlen);
}

int jnetConnect(int fd, struct addrinfo *info)
{
    int r = connect(fd, info->ai_addr, info->ai_addrlen);
    if (r == -1 && errno != EINPROGRESS) return r;
}

