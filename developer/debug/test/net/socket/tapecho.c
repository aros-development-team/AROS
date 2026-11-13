/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    A minimal bsdsocket.library TCP echo server, intended for testing the
    socket library over a real network interface (e.g. the hosted tap.device).

    It listens on TCP <port> (default 7777) on all interfaces, accepts one
    connection (waiting up to <timeout> seconds), echoes everything back until
    the peer closes, then exits.  This exercises the inbound listen/accept path
    (and hence the SYN cache) against an external peer.

    Usage:  tapecho [PORT] [TIMEOUT]
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/socket.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Library *SocketBase = NULL;

int main(int argc, char **argv)
{
    int ls, cs;
    int port = 7777, timeout = 30, on = 1;
    struct sockaddr_in sa;
    socklen_t len;
    fd_set rf;
    struct timeval tv;
    char buf[512];
    int r, total = 0;

    if (argc > 1)
        port = atoi(argv[1]);
    if (argc > 2)
        timeout = atoi(argv[2]);

    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("tapecho: bsdsocket.library not available (is AROSTCP running?)\n");
        return 20;
    }

    ls = socket(AF_INET, SOCK_STREAM, 0);
    if (ls < 0) {
        printf("tapecho: socket() failed\n");
        CloseLibrary(SocketBase);
        return 20;
    }
    setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    bzero(&sa, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_len = sizeof(sa);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    sa.sin_port = htons((unsigned short)port);
    if (bind(ls, (struct sockaddr *)&sa, sizeof(sa)) != 0) {
        printf("tapecho: bind() to port %d failed\n", port);
        CloseSocket(ls);
        CloseLibrary(SocketBase);
        return 20;
    }
    listen(ls, 5);
    printf("tapecho: listening on port %d (waiting %ds for a connection)\n",
        port, timeout);

    FD_ZERO(&rf);
    FD_SET(ls, &rf);
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    if (WaitSelect(ls + 1, &rf, NULL, NULL, &tv, NULL) <= 0) {
        printf("tapecho: no connection within %ds, exiting\n", timeout);
        CloseSocket(ls);
        CloseLibrary(SocketBase);
        return 0;
    }

    len = sizeof(sa);
    cs = accept(ls, (struct sockaddr *)&sa, &len);
    if (cs < 0) {
        printf("tapecho: accept() failed\n");
        CloseSocket(ls);
        CloseLibrary(SocketBase);
        return 20;
    }
    printf("tapecho: connection from %s:%d\n",
        Inet_NtoA(sa.sin_addr.s_addr), ntohs(sa.sin_port));

    /* Echo until the peer closes. */
    while ((r = recv(cs, buf, sizeof(buf), 0)) > 0) {
        total += r;
        if (send(cs, buf, r, 0) != r)
            break;
    }
    printf("tapecho: echoed %d bytes, connection closed\n", total);

    CloseSocket(cs);
    CloseSocket(ls);
    CloseLibrary(SocketBase);
    return 0;
}
