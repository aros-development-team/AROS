/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Diagnostic: probe IPv6 bind/getsockname behaviour for TCP and UDP sockets.
*/

#include <proto/exec.h>
#include <proto/socket.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>

#include <stdio.h>
#include <string.h>
#include <strings.h>

struct Library *SocketBase = NULL;

static void probe(const char *label, int type)
{
    int s, br, gr, i;
    struct sockaddr_in6 sa;
    struct in6_addr lo = IN6ADDR_LOOPBACK_INIT;
    socklen_t len;

    s = socket(AF_INET6, type, 0);
    printf("%s: socket()=%d\n", label, s);
    if (s < 0)
        return;

    bzero(&sa, sizeof(sa));
    sa.sin6_len = sizeof(sa);
    sa.sin6_family = AF_INET6;
    sa.sin6_addr = lo;
    sa.sin6_port = 0;
    br = bind(s, (struct sockaddr *)&sa, sizeof(sa));
    printf("%s: bind(::1:0)=%d Errno=%ld\n", label, br, (long)Errno());

    bzero(&sa, sizeof(sa));
    len = sizeof(sa);
    gr = getsockname(s, (struct sockaddr *)&sa, &len);
    printf("%s: getsockname=%d len=%d family=%d port=%d addr=",
        label, gr, (int)len, sa.sin6_family, ntohs(sa.sin6_port));
    for (i = 0; i < 16; i++)
        printf("%02x", ((unsigned char *)&sa.sin6_addr)[i]);
    printf("\n");
    CloseSocket(s);
}

int main(void)
{
    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("v6diag: no bsdsocket.library\n");
        return 0;
    }
    probe("TCP6", SOCK_STREAM);
    probe("UDP6", SOCK_DGRAM);

    /* UDP6 loopback round-trip over ::1, step by step. */
    {
        int rfd, cfd, r;
        struct sockaddr_in6 sa;
        struct in6_addr lo = IN6ADDR_LOOPBACK_INIT;
        socklen_t len;
        fd_set rf;
        struct timeval tv;
        char buf[16];

        printf("UDP6-rt: creating receiver\n");
        rfd = socket(AF_INET6, SOCK_DGRAM, 0);
        bzero(&sa, sizeof(sa));
        sa.sin6_len = sizeof(sa); sa.sin6_family = AF_INET6;
        sa.sin6_addr = lo; sa.sin6_port = 0;
        printf("UDP6-rt: bind receiver = %d\n",
            bind(rfd, (struct sockaddr *)&sa, sizeof(sa)));
        len = sizeof(sa);
        getsockname(rfd, (struct sockaddr *)&sa, &len);
        printf("UDP6-rt: receiver port = %d\n", ntohs(sa.sin6_port));

        cfd = socket(AF_INET6, SOCK_DGRAM, 0);
        printf("UDP6-rt: sendto ::1 ...\n");
        r = sendto(cfd, "v6dg", 4, 0, (struct sockaddr *)&sa, sizeof(sa));
        printf("UDP6-rt: sendto = %d Errno=%ld\n", r, (long)Errno());

        FD_ZERO(&rf); FD_SET(rfd, &rf);
        tv.tv_sec = 3; tv.tv_usec = 0;
        printf("UDP6-rt: WaitSelect ...\n");
        r = WaitSelect(rfd + 1, &rf, NULL, NULL, &tv, NULL);
        printf("UDP6-rt: WaitSelect = %d\n", r);
        if (r > 0) {
            r = recv(rfd, buf, sizeof(buf), 0);
            printf("UDP6-rt: recv = %d (%.4s)\n", r, buf);
        }
        CloseSocket(cfd);
        CloseSocket(rfd);
    }

    /* TCP6 loopback handshake + echo over ::1, step by step. */
    {
        int lfd, cfd, afd, on = 1, r;
        struct sockaddr_in6 sa;
        struct in6_addr lo = IN6ADDR_LOOPBACK_INIT;
        socklen_t len;
        fd_set rf, wf;
        struct timeval tv;
        char buf[16];
        unsigned short port;

        printf("TCP6-rt: creating listener\n");
        lfd = socket(AF_INET6, SOCK_STREAM, 0);
        bzero(&sa, sizeof(sa));
        sa.sin6_len = sizeof(sa); sa.sin6_family = AF_INET6;
        sa.sin6_addr = lo; sa.sin6_port = 0;
        printf("TCP6-rt: bind listener = %d Errno=%ld\n",
            bind(lfd, (struct sockaddr *)&sa, sizeof(sa)), (long)Errno());
        printf("TCP6-rt: listen = %d\n", listen(lfd, 5));
        len = sizeof(sa);
        getsockname(lfd, (struct sockaddr *)&sa, &len);
        port = ntohs(sa.sin6_port);
        printf("TCP6-rt: listener port = %d\n", port);

        cfd = socket(AF_INET6, SOCK_STREAM, 0);
        IoctlSocket(cfd, FIONBIO, (char *)&on);
        bzero(&sa, sizeof(sa));
        sa.sin6_len = sizeof(sa); sa.sin6_family = AF_INET6;
        sa.sin6_addr = lo; sa.sin6_port = htons(port);
        r = connect(cfd, (struct sockaddr *)&sa, sizeof(sa));
        printf("TCP6-rt: connect = %d Errno=%ld (EINPROGRESS ok)\n",
            r, (long)Errno());

        FD_ZERO(&rf); FD_SET(lfd, &rf);
        tv.tv_sec = 3; tv.tv_usec = 0;
        printf("TCP6-rt: WaitSelect(listener readable) ...\n");
        r = WaitSelect(lfd + 1, &rf, NULL, NULL, &tv, NULL);
        printf("TCP6-rt: WaitSelect = %d\n", r);
        if (r > 0) {
            len = sizeof(sa);
            afd = accept(lfd, (struct sockaddr *)&sa, &len);
            printf("TCP6-rt: accept = %d port=%d\n", afd, ntohs(sa.sin6_port));
            if (afd >= 0) {
                FD_ZERO(&wf); FD_SET(cfd, &wf);
                tv.tv_sec = 3; tv.tv_usec = 0;
                r = WaitSelect(cfd + 1, NULL, &wf, NULL, &tv, NULL);
                printf("TCP6-rt: client writable = %d\n", r);
                r = send(afd, "v6tc", 4, 0);
                printf("TCP6-rt: server send = %d Errno=%ld\n",
                    r, (long)Errno());
                FD_ZERO(&rf); FD_SET(cfd, &rf);
                tv.tv_sec = 3; tv.tv_usec = 0;
                r = WaitSelect(cfd + 1, &rf, NULL, NULL, &tv, NULL);
                printf("TCP6-rt: client readable = %d\n", r);
                if (r > 0) {
                    r = recv(cfd, buf, sizeof(buf), 0);
                    printf("TCP6-rt: client recv = %d (%.4s)\n", r, buf);
                }
                CloseSocket(afd);
            }
        }
        CloseSocket(cfd);
        CloseSocket(lfd);
    }
    printf("v6diag: complete\n");
    CloseLibrary(SocketBase);
    return 0;
}
