/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: SMP coverage for networking from pthreads (SMP-Net covers it
          with native tasks): posixc read()/write()/close() on a socket from
          threads on several CPUs that never opened bsdsocket.library.
          Needs the TCP/IP stack running.
*/

#include <exec/tasks.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <dos/dostags.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/kernel.h>
#include <proto/socket.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <libraries/bsdsocket.h>

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#if defined(__AROSEXEC_SMP__)
#define DEBUG 1
#include <aros/debug.h>

APTR KernelBase;
struct Library *SocketBase;     /* unused: every task opens its own base */
static int g_NumCPUs = 1;
static int pass, fail, invalid;

#define MAXCPUS         8

#define DONE_TICKS      3000    /* 60s for a part to finish */

static void check(BOOL ok, const char *what)
{
    if (ok)
    {
        bug("[smppthreadsnet] %s OK\n", what);
        pass++;
    }
    else
    {
        bug("[smppthreadsnet] *** FAIL *** %s\n", what);
        fail++;
    }
}

static UWORD master_opencnt(void)
{
    struct Library *lib;
    UWORD cnt = 0;

    Forbid();
    lib = (struct Library *)FindName(&SysBase->LibList, "bsdsocket.library");
    if (lib)
        cnt = lib->lib_OpenCnt;
    Permit();
    return cnt;
}

static void loopback(struct sockaddr_in *sin, UWORD port)
{
    memset(sin, 0, sizeof(*sin));
    sin->sin_len = sizeof(*sin);
    sin->sin_family = AF_INET;
    sin->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    sin->sin_port = port;
}

/* ---- posixc socket I/O from threads ---- */

#define PX_ROUNDS   100
#define PX_CHUNK    64

static LONG g_PxWr, g_PxRd;
static ULONG g_PxWriters;
static volatile ULONG g_PxBad, g_PxRead, g_PxReadDone;
static volatile LONG g_PxCloseRet;

static void *PxWriter(void *arg)
{
    UBYTE buf[PX_CHUNK];
    int r;

    for (r = 0; r < PX_ROUNDS; r++)
    {
        /* One value per chunk, so a split write shows up as a mixed chunk */
        memset(buf, (int)(((IPTR)arg << 4) | (r & 15)), PX_CHUNK);
        if (write(g_PxWr, buf, PX_CHUNK) != PX_CHUNK)
            __atomic_add_fetch(&g_PxBad, 1, __ATOMIC_RELAXED);
    }
    return NULL;
}

static void *PxReader(void *arg)
{
    ULONG want = g_PxWriters * PX_ROUNDS * PX_CHUNK;
    UBYTE buf[PX_CHUNK];
    ULONG have = 0;

    while (g_PxRead < want)
    {
        LONG n = read(g_PxRd, buf + have, PX_CHUNK - have);

        if (n <= 0)
            break;
        have += n;
        g_PxRead += n;
        if (have == PX_CHUNK)
        {
            int i;

            for (i = 1; i < PX_CHUNK; i++)
                if (buf[i] != buf[0])
                {
                    __atomic_add_fetch(&g_PxBad, 1, __ATOMIC_RELAXED);
                    break;
                }
            have = 0;
        }
    }
    g_PxReadDone = 1;
    return arg;
}

static void *PxCloser(void *arg)
{
    g_PxCloseRet = close(g_PxWr);
    return arg;
}

static void test_posix_threads(void)
{
    UWORD cnt0 = master_opencnt(), cnt1;    /* before our own base */
    struct Library *SocketBase = OpenLibrary("bsdsocket.library", 4);
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    pthread_t thr[MAXCPUS + 1];
    LONG l, before, after;
    ULONG i;
    BOOL done;

    if (!SocketBase)
        return;
    before = socket(AF_INET, SOCK_STREAM, 0);
    CloseSocket(before);

    l = socket(AF_INET, SOCK_STREAM, 0);
    loopback(&sin, 0);
    bind(l, (struct sockaddr *)&sin, sizeof(sin));
    listen(l, 2);
    getsockname(l, (struct sockaddr *)&sin, &len);
    g_PxWr = socket(AF_INET, SOCK_STREAM, 0);
    connect(g_PxWr, (struct sockaddr *)&sin, sizeof(sin));
    g_PxRd = accept(l, NULL, NULL);
    CloseSocket(l);

    g_PxBad = g_PxRead = g_PxReadDone = 0;
    g_PxCloseRet = -2;
    g_PxWriters = g_NumCPUs - 1;
    pthread_create(&thr[0], NULL, PxReader, NULL);
    for (i = 0; i < g_PxWriters; i++)
        pthread_create(&thr[i + 1], NULL, PxWriter, (void *)(IPTR)(i + 1));

    for (i = 0; i < DONE_TICKS && !g_PxReadDone; i++)
        Delay(1);
    done = g_PxReadDone;
    if (done)
    {
        for (i = 0; i <= g_PxWriters; i++)
            pthread_join(thr[i], NULL);
        pthread_create(&thr[0], NULL, PxCloser, NULL);
        pthread_join(thr[0], NULL);
    }
    CloseSocket(g_PxRd);

    after = socket(AF_INET, SOCK_STREAM, 0);
    CloseSocket(after);
    CloseLibrary(SocketBase);
    Delay(10);
    cnt1 = master_opencnt();

    bug("[smppthreadsnet] posix: %lu writers, read %lu bytes, bad=%lu, close=%ld, fd %ld -> %ld, opencnt %u -> %u\n",
        (unsigned long)g_PxWriters, (unsigned long)g_PxRead, (unsigned long)g_PxBad,
        (long)g_PxCloseRet, (long)before, (long)after, cnt0, cnt1);
    check(done && g_PxRead == g_PxWriters * PX_ROUNDS * PX_CHUNK, "posixc threads moved all the data");
    check(g_PxBad == 0, "no write failed or was split");
    check(g_PxCloseRet == 0, "another thread closed the socket");
    check(after == before && cnt1 == cnt0, "no base or descriptor leaked");
}

int main(void)
{
    struct Library *SocketBase;

    KernelBase = OpenResource("kernel.resource");
    if (KernelBase)
        g_NumCPUs = KrnGetCPUCount();
    if (g_NumCPUs > MAXCPUS)
        g_NumCPUs = MAXCPUS;

    bug("[smppthreadsnet] start: cpus=%d\n", g_NumCPUs);

    if (!KernelBase || g_NumCPUs < 2)
    {
        bug("[smppthreadsnet] INVALID (needs at least 2 CPUs)\n");
        return RETURN_WARN;
    }

    SocketBase = OpenLibrary("bsdsocket.library", 4);
    if (SocketBase)
    {
        CloseLibrary(SocketBase);
        test_posix_threads();
    }
    else
    {
        bug("[smppthreadsnet] INVALID bsdsocket: bsdsocket.library not available (stack not running?)\n");
        invalid++;
    }

    bug("[smppthreadsnet] DONE: %d PASS, %d FAIL, %d INVALID\n", pass, fail, invalid);
    return fail ? RETURN_FAIL : RETURN_OK;
}
#else
int main(void)
{
    return RETURN_FAIL;
}
#endif
