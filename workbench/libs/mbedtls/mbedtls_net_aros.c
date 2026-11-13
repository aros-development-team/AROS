/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    mbedTLS network adapter for AROS bsdsocket.library.
    Implements mbedtls_net_* functions using Amiga socket API.
*/

#include "mbedtls/net_sockets.h"
#include "mbedtls/error.h"

#include <proto/exec.h>
#include <proto/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

struct Library *SocketBase;

void mbedtls_net_init(mbedtls_net_context *ctx)
{
    ctx->fd = -1;
    if (!SocketBase)
        SocketBase = OpenLibrary("bsdsocket.library", 4);
}

void mbedtls_net_free(mbedtls_net_context *ctx)
{
    if (ctx->fd >= 0)
    {
        CloseSocket(ctx->fd);
        ctx->fd = -1;
    }
}

int mbedtls_net_connect(mbedtls_net_context *ctx,
                        const char *host, const char *port, int proto)
{
    struct hostent *he;
    struct sockaddr_in addr;
    int fd, type;

    if (!SocketBase)
        return MBEDTLS_ERR_NET_SOCKET_FAILED;

    type = (proto == MBEDTLS_NET_PROTO_UDP) ? SOCK_DGRAM : SOCK_STREAM;
    fd = socket(AF_INET, type, 0);
    if (fd < 0)
        return MBEDTLS_ERR_NET_SOCKET_FAILED;

    he = gethostbyname((char *)host);
    if (!he)
    {
        CloseSocket(fd);
        return MBEDTLS_ERR_NET_UNKNOWN_HOST;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)atoi(port));
    memcpy(&addr.sin_addr, he->h_addr, he->h_length);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        CloseSocket(fd);
        return MBEDTLS_ERR_NET_CONNECT_FAILED;
    }

    ctx->fd = fd;
    return 0;
}

int mbedtls_net_send(void *ctx, const unsigned char *buf, size_t len)
{
    int fd = ((mbedtls_net_context *)ctx)->fd;
    int ret = send(fd, (void *)buf, len, 0);
    if (ret < 0)
    {
        if (Errno() == EAGAIN || Errno() == EWOULDBLOCK)
            return MBEDTLS_ERR_SSL_WANT_WRITE;
        return MBEDTLS_ERR_NET_SEND_FAILED;
    }
    return ret;
}

int mbedtls_net_recv(void *ctx, unsigned char *buf, size_t len)
{
    int fd = ((mbedtls_net_context *)ctx)->fd;
    int ret = recv(fd, (void *)buf, len, 0);
    if (ret < 0)
    {
        if (Errno() == EAGAIN || Errno() == EWOULDBLOCK)
            return MBEDTLS_ERR_SSL_WANT_READ;
        return MBEDTLS_ERR_NET_RECV_FAILED;
    }
    if (ret == 0)
        return MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY;
    return ret;
}

int mbedtls_net_recv_timeout(void *ctx, unsigned char *buf, size_t len,
                             uint32_t timeout)
{
    int fd = ((mbedtls_net_context *)ctx)->fd;
    struct timeval tv;
    fd_set fds;

    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;

    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    int ret = WaitSelect(fd + 1, &fds, NULL, NULL, &tv, NULL);
    if (ret == 0)
        return MBEDTLS_ERR_SSL_TIMEOUT;
    if (ret < 0)
        return MBEDTLS_ERR_NET_RECV_FAILED;

    return mbedtls_net_recv(ctx, buf, len);
}
