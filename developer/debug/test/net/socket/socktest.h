/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Shared helpers for the bsdsocket.library cunit tests.  Each test program
    defines its own `SocketBase` and opens bsdsocket.library in main().
*/

#ifndef NET_SOCKTEST_H
#define NET_SOCKTEST_H

#include <proto/exec.h>
#include <proto/socket.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <stdio.h>
#include <string.h>
#include <strings.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>

extern struct Library *SocketBase;

/* Wait up to secs for fd to become readable (forwrite=0) or writable
 * (forwrite=1).  Returns the WaitSelect() result (>0 ready, 0 timeout). */
static int sock_wait(int fd, int forwrite, int secs)
{
    fd_set fs;
    struct timeval tv;

    FD_ZERO(&fs);
    FD_SET(fd, &fs);
    tv.tv_sec = secs;
    tv.tv_usec = 0;
    if (forwrite)
        return WaitSelect(fd + 1, NULL, &fs, NULL, &tv, NULL);
    return WaitSelect(fd + 1, &fs, NULL, NULL, &tv, NULL);
}

/* Fill a loopback sockaddr for the given family.  Returns the address len. */
static socklen_t st_loopback(int family, struct sockaddr_storage *ss,
    unsigned short nport)
{
    bzero(ss, sizeof(*ss));
    if (family == AF_INET6) {
        struct sockaddr_in6 *s6 = (struct sockaddr_in6 *)ss;
        struct in6_addr lo = IN6ADDR_LOOPBACK_INIT;
        s6->sin6_len = sizeof(*s6);
        s6->sin6_family = AF_INET6;
        s6->sin6_addr = lo;
        s6->sin6_port = nport;
        return sizeof(*s6);
    } else {
        struct sockaddr_in *s4 = (struct sockaddr_in *)ss;
        s4->sin_len = sizeof(*s4);
        s4->sin_family = AF_INET;
        s4->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        s4->sin_port = nport;
        return sizeof(*s4);
    }
}

/* Return the port (network order) stored in a sockaddr of either family. */
static unsigned short st_port(struct sockaddr_storage *ss)
{
    if (ss->ss_family == AF_INET6)
        return ((struct sockaddr_in6 *)ss)->sin6_port;
    return ((struct sockaddr_in *)ss)->sin_port;
}

/* Create a loopback listener of the given type (SOCK_STREAM/DGRAM) bound to an
 * ephemeral port.  On success returns the fd and stores the chosen port
 * (network order) in *nport_out.  Returns -1 on failure. */
static int st_bound(int family, int type, unsigned short *nport_out)
{
    struct sockaddr_storage ss;
    socklen_t len;
    int on = 1;
    int s = socket(family, type, 0);

    if (s < 0)
        return -1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    len = st_loopback(family, &ss, 0);
    if (bind(s, (struct sockaddr *)&ss, len) != 0) {
        CloseSocket(s);
        return -1;
    }
    len = sizeof(ss);
    if (getsockname(s, (struct sockaddr *)&ss, &len) != 0) {
        CloseSocket(s);
        return -1;
    }
    if (nport_out)
        *nport_out = st_port(&ss);
    return s;
}

/* Establish a connected loopback TCP pair (family AF_INET/AF_INET6).  Stores
 * the connected client fd in *cfd and the accepted server fd in *sfd, both in
 * blocking mode.  Returns 0 on success, -1 on failure. */
static int st_tcp_pair(int family, int *cfd, int *sfd)
{
    unsigned short nport;
    int lfd, c, a, on = 1, off = 0;
    struct sockaddr_storage ss;
    socklen_t len;

    *cfd = *sfd = -1;
    lfd = st_bound(family, SOCK_STREAM, &nport);
    if (lfd < 0)
        return -1;
    if (listen(lfd, 5) != 0) {
        CloseSocket(lfd);
        return -1;
    }
    c = socket(family, SOCK_STREAM, 0);
    if (c < 0) {
        CloseSocket(lfd);
        return -1;
    }
    IoctlSocket(c, FIONBIO, (char *)&on);
    len = st_loopback(family, &ss, nport);
    if (connect(c, (struct sockaddr *)&ss, len) != 0 &&
            Errno() != EINPROGRESS) {
        CloseSocket(c);
        CloseSocket(lfd);
        return -1;
    }
    if (sock_wait(lfd, 0, 3) <= 0) {
        CloseSocket(c);
        CloseSocket(lfd);
        return -1;
    }
    len = sizeof(ss);
    a = accept(lfd, (struct sockaddr *)&ss, &len);
    CloseSocket(lfd);
    if (a < 0) {
        CloseSocket(c);
        return -1;
    }
    if (sock_wait(c, 1, 3) <= 0) {		/* confirm client connected */
        CloseSocket(c);
        CloseSocket(a);
        return -1;
    }
    IoctlSocket(c, FIONBIO, (char *)&off);	/* back to blocking */
    *cfd = c;
    *sfd = a;
    return 0;
}

/* Transfer `total` bytes one way from cs to ss, verifying every byte against a
 * deterministic pattern.  Uses non-blocking I/O + WaitSelect to avoid a
 * single-task deadlock; exercises segmentation, window scaling and flow
 * control.  Returns 0 on success, -1 otherwise. */
static int st_oneway(int cs, int ss, int total)
{
    int on = 1, off = 0;
    int sent = 0, rcvd = 0, i, ok = 1;
    unsigned char buf[4096];

    IoctlSocket(cs, FIONBIO, (char *)&on);
    IoctlSocket(ss, FIONBIO, (char *)&on);

    while (rcvd < total && ok) {
        fd_set rf, wf;
        struct timeval tv;
        int mx = (cs > ss ? cs : ss);

        FD_ZERO(&rf);
        FD_ZERO(&wf);
        if (sent < total)
            FD_SET(cs, &wf);
        FD_SET(ss, &rf);
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        if (WaitSelect(mx + 1, &rf, &wf, NULL, &tv, NULL) <= 0) {
            ok = 0;
            break;
        }
        if (sent < total && FD_ISSET(cs, &wf)) {
            int chunk = total - sent, w;
            if (chunk > (int)sizeof(buf))
                chunk = sizeof(buf);
            for (i = 0; i < chunk; i++)
                buf[i] = (unsigned char)((sent + i) & 0xff);
            w = send(cs, buf, chunk, 0);
            if (w > 0)
                sent += w;
            else if (w < 0 && Errno() != EWOULDBLOCK && Errno() != EAGAIN)
                ok = 0;
        }
        if (FD_ISSET(ss, &rf)) {
            int r = recv(ss, buf, sizeof(buf), 0);
            if (r > 0) {
                for (i = 0; i < r; i++) {
                    if (buf[i] != (unsigned char)((rcvd + i) & 0xff)) {
                        ok = 0;
                        break;
                    }
                }
                rcvd += r;
            } else if (r == 0)
                break;
            else if (Errno() != EWOULDBLOCK && Errno() != EAGAIN)
                ok = 0;
        }
    }
    IoctlSocket(cs, FIONBIO, (char *)&off);
    IoctlSocket(ss, FIONBIO, (char *)&off);
    return (ok && rcvd == total && sent == total) ? 0 : -1;
}

#endif /* NET_SOCKTEST_H */
