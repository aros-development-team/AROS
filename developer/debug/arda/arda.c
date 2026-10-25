/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    ARDA - AROS Remote Debug Agent.

    A small read-only diagnostic server for investigating a running AROS
    system from a development host.  It opens bsdsocket.library, listens on
    one TCP port (loopback by default) and answers line-oriented commands;
    see host/PROTOCOL.md and host/README.md.

    Usage:  ARDA [PORT=<n>] [ADDRESS=<dotted quad>] [IDLE=<secs>] [QUIET]

    Remote access requires an explicit ADDRESS (e.g. the interface address or
    0.0.0.0) and should only be enabled on a trusted development network.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/bsdsocket.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "arda.h"

struct Library *SocketBase = NULL;

const char version[] = "$VER: ARDA " ARDA_AGENT_VERSION " (25.10.2026) AROS Remote Debug Agent";

static const char arg_template[] = "PORT/N,ADDRESS/K,IDLE/N,QUIET/S";
enum { ARG_PORT, ARG_ADDRESS, ARG_IDLE, ARG_QUIET, ARG_COUNT };

/* Kept static: the response buffer is 64 KiB and must not live on the stack. */
static struct Arda arda;

void arda_log(struct Arda *a, const char *fmt, ...)
{
    char msg[256];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    arda_log_event(msg);
    if (a->quiet)
        return;
    puts(msg);
    fflush(stdout);
}

int main(void)
{
    IPTR args[ARG_COUNT] = { 0 };
    struct RDArgs *rda;
    const char *address = ARDA_DEFAULT_ADDRESS;
    LONG port = ARDA_DEFAULT_PORT;
    LONG idle = ARDA_DEFAULT_IDLE;
    int rc = RETURN_OK;

    rda = ReadArgs(arg_template, args, NULL);
    if (!rda)
    {
        PrintFault(IoErr(), "ARDA");
        return RETURN_ERROR;
    }
    if (args[ARG_PORT])
        port = *(LONG *)args[ARG_PORT];
    if (args[ARG_ADDRESS])
        address = (const char *)args[ARG_ADDRESS];
    if (args[ARG_IDLE])
        idle = *(LONG *)args[ARG_IDLE];

    if (port < 1 || port > 65535 || idle < 0)
    {
        printf("ARDA: PORT must be 1..65535 and IDLE >= 0\n");
        FreeArgs(rda);
        return RETURN_ERROR;
    }

    memset(&arda, 0, sizeof(arda));
    arda.ls         = -1;
    arda.client.fd  = -1;
    arda.quiet      = args[ARG_QUIET] ? TRUE : FALSE;
    arda.idle_limit = idle;
    arda.resp.buf   = arda.respbuf;
    arda.resp.cap   = ARDA_MAX_RESP;
    strcpy(arda.client.peer, "-");

    SocketBase = OpenLibrary("bsdsocket.library", 4);
    if (!SocketBase)
    {
        printf("ARDA: bsdsocket.library v4 not available (is the TCP/IP stack running?)\n");
        FreeArgs(rda);
        return RETURN_FAIL;
    }

    arda_logsrc_init(&arda);
    arda_sys_init(&arda);

    if (!arda_net_listen(&arda, address, port))
    {
        rc = RETURN_FAIL;
        goto out;
    }

    arda_log(&arda, "ARDA %s listening on %s:%ld (protocol %s), Ctrl-C to stop",
             ARDA_AGENT_VERSION, address, (long)port, ARDA_PROTOCOL);

    arda.running = TRUE;
    while (arda.running)
    {
        fd_set rf;
        struct timeval tv;
        ULONG sigs = SIGBREAKF_CTRL_C | arda_logsrc_sigmask();
        int nfds = arda.ls + 1;
        int n;

        FD_ZERO(&rf);
        FD_SET(arda.ls, &rf);
        if (arda.client.fd >= 0)
        {
            FD_SET(arda.client.fd, &rf);
            if (arda.client.fd + 1 > nfds)
                nfds = arda.client.fd + 1;
        }
        tv.tv_sec  = 1;
        tv.tv_usec = 0;

        n = WaitSelect(nfds, &rf, NULL, NULL, &tv, &sigs);

        arda_logsrc_poll(&arda);

        if (sigs & SIGBREAKF_CTRL_C)
        {
            arda_log(&arda, "ARDA: break received, shutting down");
            break;
        }
        if (n < 0)
        {
            if (Errno() == EINTR)
                continue;
            arda_log(&arda, "ARDA: WaitSelect() failed (errno %ld), shutting down", (long)Errno());
            rc = RETURN_FAIL;
            break;
        }

        arda.uptime++;              /* approximate: WaitSelect returns at least once a second */

        if (n == 0)
        {
            if (arda.client.fd >= 0 && arda.idle_limit && ++arda.client.idle >= arda.idle_limit)
            {
                static const char msg[] = "ERR 408 idle timeout, closing\n";
                arda_net_send(&arda, arda.client.fd, msg, sizeof(msg) - 1);
                arda_net_close_client(&arda, "idle timeout");
            }
            continue;
        }

        if (arda.client.fd >= 0 && FD_ISSET(arda.client.fd, &rf))
            arda_net_input(&arda);
        if (FD_ISSET(arda.ls, &rf))
            arda_net_accept(&arda);
    }

out:
    arda_net_close_client(&arda, "agent shutdown");
    if (arda.ls >= 0)
        CloseSocket(arda.ls);
    CloseLibrary(SocketBase);
    SocketBase = NULL;
    arda_sys_exit();
    arda_log(&arda, "ARDA: stopped");
    arda_logsrc_exit();
    FreeArgs(rda);
    return rc;
}
