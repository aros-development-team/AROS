/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    ARDA - bsdsocket.library transport.

    One listening socket, at most one client.  The listening socket is
    non-blocking so accept() can never stall; accepted sockets inherit that,
    so every recv()/send() is paired with a bounded WaitSelect().
*/

#include "arda.h"

#include <proto/exec.h>
#include <proto/bsdsocket.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

extern struct Library *SocketBase;

static BOOL err_would_block(void)
{
    LONG e = Errno();
    return (e == EWOULDBLOCK || e == EAGAIN || e == EINTR);
}

BOOL arda_net_listen(struct Arda *a, const char *addr, ULONG port)
{
    struct sockaddr_in sa;
    ULONG ip;
    int on = 1;

    ip = inet_addr(addr);
    if (ip == INADDR_NONE && strcmp(addr, "255.255.255.255") != 0)
    {
        printf("ARDA: invalid ADDRESS '%s'\n", addr);
        return FALSE;
    }

    a->ls = socket(AF_INET, SOCK_STREAM, 0);
    if (a->ls < 0)
    {
        printf("ARDA: socket() failed (errno %ld)\n", (long)Errno());
        return FALSE;
    }

    setsockopt(a->ls, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    memset(&sa, 0, sizeof(sa));
    sa.sin_len         = sizeof(sa);
    sa.sin_family      = AF_INET;
    sa.sin_addr.s_addr = ip;                /* inet_addr() is network order */
    sa.sin_port        = htons((UWORD)port);

    if (bind(a->ls, (struct sockaddr *)&sa, sizeof(sa)) != 0)
    {
        printf("ARDA: bind(%s:%lu) failed (errno %ld)\n", addr,
               (unsigned long)port, (long)Errno());
        return FALSE;
    }

    if (listen(a->ls, 2) != 0)
    {
        printf("ARDA: listen() failed (errno %ld)\n", (long)Errno());
        return FALSE;
    }

    if (IoctlSocket(a->ls, FIONBIO, (char *)&on) != 0)
        arda_log(a, "ARDA: FIONBIO failed (errno %ld), continuing blocking", (long)Errno());

    a->port = port;
    return TRUE;
}

void arda_net_close_client(struct Arda *a, const char *why)
{
    if (a->client.fd >= 0)
    {
        CloseSocket(a->client.fd);
        arda_log(a, "ARDA: client %s disconnected (%s)", a->client.peer, why);
    }
    memset(&a->client, 0, sizeof(a->client));
    a->client.fd = -1;
    strcpy(a->client.peer, "-");
}

void arda_net_accept(struct Arda *a)
{
    struct sockaddr_in sa;
    socklen_t len = sizeof(sa);
    int fd, on = 1;

    fd = accept(a->ls, (struct sockaddr *)&sa, &len);
    if (fd < 0)
    {
        if (!err_would_block())
            arda_log(a, "ARDA: accept() failed (errno %ld)", (long)Errno());
        return;
    }

    IoctlSocket(fd, FIONBIO, (char *)&on);

    if (a->client.fd >= 0)
    {
        static const char busy[] = "ERR 503 busy, one client at a time\n";
        arda_net_send(a, fd, busy, sizeof(busy) - 1);
        CloseSocket(fd);
        a->rejected++;
        arda_log(a, "ARDA: refused second client while %s is connected", a->client.peer);
        return;
    }

    memset(&a->client, 0, sizeof(a->client));
    a->client.fd = fd;
    snprintf(a->client.peer, sizeof(a->client.peer), "%s:%u",
             Inet_NtoA(sa.sin_addr.s_addr), (unsigned)ntohs(sa.sin_port));
    a->conns++;
    arda_log(a, "ARDA: client %s connected", a->client.peer);
}

/* Write all of data, waiting (bounded) for the socket to drain. */
BOOL arda_net_send(struct Arda *a, int fd, const char *data, ULONG len)
{
    ULONG off = 0;
    int stalled = 0;

    while (off < len)
    {
        int n = send(fd, data + off, len - off, 0);

        if (n > 0)
        {
            off += n;
            stalled = 0;
            continue;
        }
        if (n < 0 && !err_would_block())
            return FALSE;

        if (stalled++ >= ARDA_SEND_TIMEOUT)
            return FALSE;

        {
            fd_set wf;
            struct timeval tv;
            ULONG sigs = SIGBREAKF_CTRL_C;
            int r;

            FD_ZERO(&wf);
            FD_SET(fd, &wf);
            tv.tv_sec  = 1;
            tv.tv_usec = 0;
            r = WaitSelect(fd + 1, NULL, &wf, NULL, &tv, &sigs);
            if (sigs & SIGBREAKF_CTRL_C)
            {
                a->running = FALSE;
                return FALSE;
            }
            if (r < 0 && Errno() != EINTR)
                return FALSE;
        }
    }
    return TRUE;
}

void arda_net_reply(struct Arda *a)
{
    if (a->client.fd < 0)
        return;
    if (!arda_net_send(a, a->client.fd, a->resp.buf, a->resp.len))
    {
        arda_net_close_client(a, "send failed");
        return;
    }
    if (a->client.closing)
        arda_net_close_client(a, "QUIT");
}

static BOOL line_is_clean(const char *s)
{
    for (; *s; s++)
    {
        unsigned char c = (unsigned char)*s;
        if (c != '\t' && (c < 0x20 || c > 0x7e))
            return FALSE;
    }
    return TRUE;
}

/* The client socket is readable: pull bytes, split into lines, dispatch. */
void arda_net_input(struct Arda *a)
{
    struct ArdaClient *c = &a->client;
    char buf[ARDA_RECV_CHUNK];
    int n, i;

    n = recv(c->fd, buf, sizeof(buf), 0);
    if (n == 0)
    {
        arda_net_close_client(a, "peer closed");
        return;
    }
    if (n < 0)
    {
        if (!err_would_block())
            arda_net_close_client(a, "recv failed");
        return;
    }

    c->idle = 0;

    for (i = 0; i < n && a->client.fd >= 0; i++)
    {
        char ch = buf[i];

        if (ch == '\n')
        {
            if (c->discard)
            {
                c->discard = FALSE;
                c->linelen = 0;
                continue;
            }
            c->line[c->linelen] = '\0';
            if (c->linelen && c->line[c->linelen - 1] == '\r')
                c->line[--c->linelen] = '\0';
            c->linelen = 0;

            if (!line_is_clean(c->line))
            {
                a->errors++;
                arda_resp_err(&a->resp, 400, "request contains non-printable characters");
                arda_net_reply(a);
                continue;
            }
            arda_dispatch(a, c->line);
            continue;
        }

        if (c->discard)
            continue;

        if (c->linelen >= ARDA_MAX_LINE)
        {
            c->discard = TRUE;
            c->linelen = 0;
            a->errors++;
            arda_resp_err(&a->resp, 400, "request line too long (max " 
                          "256 bytes)");
            arda_net_reply(a);
            continue;
        }
        c->line[c->linelen++] = ch;
    }
}
