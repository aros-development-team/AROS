/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    ARDA - AROS Remote Debug Agent.

    A small read-only diagnostic server for investigating a running AROS
    system from a development host.  It opens bsdsocket.library, listens on
    one TCP port (loopback by default) and answers line-oriented commands;
    see host/PROTOCOL.md and host/README.md.

    Where the stack lets the tasks of a program share a library base
    (SBTC_CAN_SHARE_LIBRARY_BASES), each connection is served by a thread of
    its own on the same base; otherwise one connection at a time.

    Usage:  ARDA [PORT=<n>] [ADDRESS=<dotted quad>] [IDLE=<secs>] [QUIET] [CONTROL]

    Remote access requires an explicit ADDRESS (e.g. the interface address or
    0.0.0.0) and should only be enabled on a trusted development network.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/bsdsocket.h>
#include <libraries/bsdsocket.h>
#include <utility/hooks.h>
#include <aros/asmcall.h>

#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "arda.h"

struct Library *SocketBase = NULL;

const char version[] = "$VER: ARDA " ARDA_AGENT_VERSION " (25.10.2026) AROS Remote Debug Agent";

static const char arg_template[] = "PORT/N,ADDRESS/K,IDLE/N,QUIET/S,CONTROL/S";
enum { ARG_PORT, ARG_ADDRESS, ARG_IDLE, ARG_QUIET, ARG_CONTROL, ARG_COUNT };

/* Kept static: the response buffer is 64 KiB and must not live on the stack. */
static struct Arda arda;
static struct ArdaShared shared;

/* The context of each thread that makes socket calls, for the error hook */
static struct
{
    struct Task *task;
    struct Arda *ctx;
} threads[ARDA_MAX_CLIENTS + 1];

static void thread_register(struct Arda *a)
{
    ULONG i;

    Forbid();
    for (i = 0; i <= ARDA_MAX_CLIENTS; i++)
        if (!threads[i].task)
        {
            threads[i].ctx = a;
            threads[i].task = FindTask(NULL);
            break;
        }
    Permit();
}

static void thread_unregister(void)
{
    struct Task *me = FindTask(NULL);
    ULONG i;

    Forbid();
    for (i = 0; i <= ARDA_MAX_CLIENTS; i++)
        if (threads[i].task == me)
            threads[i].task = NULL;
    Permit();
}

static struct Arda *thread_ctx(void)
{
    struct Task *me = FindTask(NULL);
    ULONG i;

    for (i = 0; i <= ARDA_MAX_CLIENTS; i++)
        if (threads[i].task == me)
            return threads[i].ctx;
    return NULL;
}

/* The stack reports each error to the calling thread through this hook */
AROS_UFH3(ULONG, error_hook_func,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(APTR, obj, A2),
    AROS_UFHA(struct ErrorHookMsg *, msg, A1))
{
    AROS_USERFUNC_INIT

    struct Arda *a = thread_ctx();

    if (a && msg->ehm_Action == EHMA_Set_errno)
        a->sockerr = msg->ehm_Code;
    return 0;

    AROS_USERFUNC_EXIT
}

static struct Hook error_hook = { { NULL, NULL }, (HOOKFUNC)error_hook_func, NULL, NULL };

LONG arda_errno(void)
{
    struct Arda *a;

    if (shared.shared_base && (a = thread_ctx()))
        return a->sockerr;
    return Errno();
}

void arda_lock(struct Arda *a)
{
    ObtainSemaphore(&a->sh->lock);
}

void arda_unlock(struct Arda *a)
{
    ReleaseSemaphore(&a->sh->lock);
}

void arda_log(struct Arda *a, const char *fmt, ...)
{
    char msg[256];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    arda_lock(a);
    arda_log_event(msg);
    if (!a->quiet)
    {
        puts(msg);
        fflush(stdout);
    }
    arda_unlock(a);
}

/* Share the base between the agent's threads, errors reported per thread */
static BOOL share_base(void)
{
    struct TagItem tags[] =
    {
        { SBTM_SETVAL(SBTC_ERROR_HOOK),              (IPTR)&error_hook },
        { SBTM_SETVAL(SBTC_CAN_SHARE_LIBRARY_BASES), TRUE },
        { TAG_DONE,                                  0 }
    };

    return SocketBaseTagList(tags) == 0;
}

/* ---- one connection, served by a thread of its own --------------------- */

static void *client_thread(void *arg)
{
    struct Arda *a = arg;

    thread_register(a);
    arda_log(a, "ARDA: client %s connected", a->client.peer);

    while (a->client.fd >= 0 && a->running && !a->sh->reboot)
    {
        fd_set rf;
        struct timeval tv;
        ULONG sigs = SIGBREAKF_CTRL_C;
        int n;

        FD_ZERO(&rf);
        FD_SET(a->client.fd, &rf);
        tv.tv_sec  = 1;
        tv.tv_usec = 0;
        n = WaitSelect(a->client.fd + 1, &rf, NULL, NULL, &tv, &sigs);
        if (sigs & SIGBREAKF_CTRL_C)
            break;
        if (n < 0)
        {
            if (arda_errno() == EINTR)
                continue;
            break;
        }
        if (n == 0)
        {
            if (a->idle_limit && ++a->client.idle >= a->idle_limit)
            {
                static const char msg[] = "ERR 408 idle timeout, closing\n";
                arda_net_send(a, a->client.fd, msg, sizeof(msg) - 1);
                arda_net_close_client(a, "idle timeout");
            }
            continue;
        }
        arda_net_input(a);
    }
    arda_net_close_client(a, a->sh->reboot ? "reboot" : "agent shutdown");
    thread_unregister();

    arda_lock(a);
    a->sh->clients--;
    arda_unlock(a);
    if (a->ctlbuf)
        FreeMem(a->ctlbuf, ARDA_CTL_CHUNK);
    FreeVec(a);
    return NULL;
}

BOOL arda_client_start(struct Arda *m, int fd, const char *peer)
{
    struct Arda *a = AllocVec(sizeof(*a), MEMF_PUBLIC | MEMF_CLEAR);
    pthread_attr_t attr;
    pthread_t t;
    int err;

    if (!a)
        return FALSE;
    a->sh         = m->sh;
    a->ls         = -1;
    a->port       = m->port;
    a->idle_limit = m->idle_limit;
    a->quiet      = m->quiet;
    a->control    = m->control;
    a->running    = TRUE;
    a->resp.buf   = a->respbuf;
    a->resp.cap   = ARDA_MAX_RESP;
    a->client.fd  = fd;
    strncpy(a->client.peer, peer, sizeof(a->client.peer) - 1);
    if (a->control && !(a->ctlbuf = AllocMem(ARDA_CTL_CHUNK, MEMF_PUBLIC)))
    {
        FreeVec(a);
        return FALSE;
    }

    arda_lock(m);
    a->sh->clients++;
    arda_unlock(m);

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, 128 * 1024);
    err = pthread_create(&t, &attr, client_thread, a);
    pthread_attr_destroy(&attr);
    if (err)
    {
        arda_lock(m);
        a->sh->clients--;
        arda_unlock(m);
        if (a->ctlbuf)
            FreeMem(a->ctlbuf, ARDA_CTL_CHUNK);
        FreeVec(a);
        return FALSE;
    }
    return TRUE;
}

/* Shutdown: every client thread gets a break and closes its connection */
static void stop_clients(void)
{
    ULONG i, wait;

    for (wait = 0; shared.clients && wait < 50; wait++)
    {
        Forbid();
        for (i = 0; i <= ARDA_MAX_CLIENTS; i++)
            if (threads[i].task && threads[i].ctx != &arda)
                Signal(threads[i].task, SIGBREAKF_CTRL_C);
        Permit();
        Delay(5);
    }
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
    memset(&shared, 0, sizeof(shared));
    InitSemaphore(&shared.lock);
    shared.owner    = FindTask(NULL);
    arda.sh         = &shared;
    arda.ls         = -1;
    arda.client.fd  = -1;
    arda.quiet      = args[ARG_QUIET] ? TRUE : FALSE;
    arda.control    = args[ARG_CONTROL] ? TRUE : FALSE;
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

    thread_register(&arda);
    shared.shared_base = share_base();

    arda_logsrc_init(&arda);
    arda_sys_init(&arda);
    arda_ctl_init(&arda);

    if (!arda_net_listen(&arda, address, port))
    {
        rc = RETURN_FAIL;
        goto out;
    }

    arda_log(&arda, "ARDA %s listening on %s:%ld (protocol %s)%s, %s %lu, Ctrl-C to stop",
             ARDA_AGENT_VERSION, address, (long)port, ARDA_PROTOCOL,
             arda.control ? ", control mode (files, env, jobs, reboot)" : "",
             shared.shared_base ? "clients at a time" : "client at a time",
             (unsigned long)(shared.shared_base ? ARDA_MAX_CLIENTS : 1));

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
        arda_lock(&arda);
        arda_ctl_poll(&arda);
        arda_unlock(&arda);
        if (shared.reboot)
        {
            ULONG wait;

            arda_log(&arda, "ARDA: rebooting on request");
            arda_net_close_client(&arda, "reboot");
            /* the client threads see the flag, send nothing more and close */
            for (wait = 0; shared.clients && wait < 30; wait++)
                Delay(5);
            Delay(25);
            ColdReboot();
        }

        if (sigs & SIGBREAKF_CTRL_C)
        {
            arda_log(&arda, "ARDA: break received, shutting down");
            break;
        }
        if (n < 0)
        {
            if (arda_errno() == EINTR)
                continue;
            arda_log(&arda, "ARDA: WaitSelect() failed (errno %ld), shutting down", (long)arda_errno());
            rc = RETURN_FAIL;
            break;
        }

        shared.uptime++;            /* approximate: WaitSelect returns at least once a second */

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
    stop_clients();
    if (arda.ls >= 0)
        CloseSocket(arda.ls);
    CloseLibrary(SocketBase);
    SocketBase = NULL;
    arda_ctl_exit(&arda);
    arda_sys_exit();
    arda_log(&arda, "ARDA: stopped");
    arda_logsrc_exit();
    FreeArgs(rda);
    return rc;
}
