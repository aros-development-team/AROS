/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: SMP coverage for starting threads and using bsdsocket.library from
          several CPUs at once:
          1. pthread_create() while every CPU hammers OpenLibrary() - the
             child's DosEntry waits on the LDDemon semaphore as it starts
          2. per-task bsdsocket bases opened and closed on every CPU
          3. TCP echo over loopback with a client on every CPU
          A corrupted semaphore or a lost wakeup can show up as a hang or
          an alert instead of a FAIL line: a thread that never starts may
          own the LDDemon semaphore, and every later OpenLibrary() waits.
          Parts 2 and 3 need the TCP/IP stack running.
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

#include <pthread.h>
#include <stdio.h>
#include <string.h>

#if defined(__AROSEXEC_SMP__)
#define DEBUG 1
#include <aros/debug.h>

APTR KernelBase;
struct Library *SocketBase;     /* unused: every task opens its own base */
static int g_NumCPUs = 1;
static int pass, fail, invalid;

#define STACKSIZE       32768
#define MAXCPUS         8

#define THR_ROUNDS      100
#define THR_COUNT       8
#define START_TICKS     250     /* 5s for a thread to start */

#define BASE_PER_CPU    2
#define BASE_ROUNDS     300

#define TCP_ROUNDS      50
#define MSGLEN          512
#define IO_SECS         5

#define DONE_TICKS      3000    /* 60s for a part to finish */

static volatile ULONG g_Stop;

static void check(BOOL ok, const char *what)
{
    if (ok)
    {
        bug("[smpnet] %s OK\n", what);
        pass++;
    }
    else
    {
        bug("[smpnet] *** FAIL *** %s\n", what);
        fail++;
    }
}

/* Start a process pinned to one CPU; the mask becomes the system's. */
static struct Process *spawn(void (*entry)(void), const char *name, int cpu, APTR data)
{
    struct Process *p;
    APTR mask = KrnAllocCPUMask();

    if (!mask)
        return NULL;
    KrnClearCPUMask(mask);
    KrnGetCPUMask(cpu, mask);

    p = CreateNewProcTags(NP_Entry,     (IPTR)entry,
                          NP_Name,      (IPTR)name,
                          NP_StackSize, STACKSIZE,
                          NP_UserData,  (IPTR)data,
                          NP_Affinity,  (IPTR)mask,
                          TAG_DONE);
    if (!p)
        KrnFreeCPUMask(mask);
    return p;
}

static BOOL wait_count(volatile ULONG *count, ULONG want, int ticks)
{
    while (*count < want && ticks-- > 0)
        Delay(1);
    return *count >= want;
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

/* ---- 1. pthread start under OpenLibrary contention ---- */

static volatile ULONG g_Hammers;

static void Hammer(void)
{
    while (!g_Stop)
    {
        struct Library *lib = OpenLibrary("dos.library", 0);

        if (lib)
            CloseLibrary(lib);
    }
    __atomic_add_fetch(&g_Hammers, 1, __ATOMIC_RELAXED);
}

struct ThrSlot
{
    volatile ULONG  ts_Started;
    volatile ULONG  ts_CPU;
};

static void *ThreadFunc(void *arg)
{
    struct ThrSlot *s = arg;

    s->ts_CPU = (ULONG)KrnGetCPUNumber();
    s->ts_Started = 1;
    return arg;
}

static void test_pthread_start(void)
{
    struct ThrSlot slot[THR_COUNT];
    pthread_t thr[THR_COUNT];
    char names[MAXCPUS][32];
    ULONG cpus = 0, created = 0;
    int r, i, n, hammers = 0;
    BOOL ok = TRUE;

    g_Stop = 0;
    g_Hammers = 0;
    for (i = 0; i < g_NumCPUs; i++)
    {
        snprintf(names[i], sizeof(names[i]), "smpnet.hammer.%d", i);
        if (spawn(Hammer, names[i], i, NULL))
            hammers++;
    }

    for (r = 0; r < THR_ROUNDS && ok; r++)
    {
        memset(slot, 0, sizeof(slot));
        for (n = 0; n < THR_COUNT; n++)
            if (pthread_create(&thr[n], NULL, ThreadFunc, &slot[n]) != 0)
                break;
        created += n;

        for (i = 0; i < n; i++)
        {
            int ticks = START_TICKS;

            while (!slot[i].ts_Started && ticks-- > 0)
                Delay(1);
            if (!slot[i].ts_Started)
            {
                /* It will never finish: joining it would hang the test */
                bug("[smpnet] round %d: thread %d never started (lost wakeup?)\n", r, i);
                ok = FALSE;
                break;
            }
            cpus |= 1UL << slot[i].ts_CPU;
        }
        if (!ok)
            break;
        for (i = 0; i < n; i++)
            pthread_join(thr[i], NULL);
        if (n < THR_COUNT)
        {
            bug("[smpnet] round %d: pthread_create failed after %d threads\n", r, n);
            ok = FALSE;
        }
    }

    g_Stop = 1;
    wait_count(&g_Hammers, hammers, DONE_TICKS);

    bug("[smpnet] pthread: %lu threads, cpu mask 0x%lx, %d hammers\n",
        (unsigned long)created, (unsigned long)cpus, hammers);
    if (ok && (cpus & ~1UL) == 0)
    {
        bug("[smpnet] INVALID pthread start: no thread ran off cpu 0\n");
        invalid++;
    }
    else
        check(ok, "pthread start under OpenLibrary load");
}

/* ---- 2. bsdsocket bases on every CPU ---- */

static volatile ULONG g_BaseDone, g_NullOpens, g_BadSockets;

static void BaseWorker(void)
{
    int i;

    for (i = 0; i < BASE_ROUNDS; i++)
    {
        struct Library *SocketBase = OpenLibrary("bsdsocket.library", 4);
        LONG fd;

        if (!SocketBase)
        {
            __atomic_add_fetch(&g_NullOpens, 1, __ATOMIC_RELAXED);
            continue;
        }
        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0)
            __atomic_add_fetch(&g_BadSockets, 1, __ATOMIC_RELAXED);
        else
            CloseSocket(fd);
        CloseLibrary(SocketBase);
    }
    __atomic_add_fetch(&g_BaseDone, 1, __ATOMIC_RELAXED);
}

static void test_bases(void)
{
    char names[MAXCPUS * BASE_PER_CPU][32];
    UWORD before, after;
    int i, started = 0;
    BOOL done;

    g_BaseDone = g_NullOpens = g_BadSockets = 0;
    before = master_opencnt();

    for (i = 0; i < g_NumCPUs * BASE_PER_CPU; i++)
    {
        snprintf(names[i], sizeof(names[i]), "smpnet.base.%d", i);
        if (spawn(BaseWorker, names[i], i % g_NumCPUs, NULL))
            started++;
    }

    done = wait_count(&g_BaseDone, started, DONE_TICKS);
    check(done, "all base workers finished");
    if (!done)
        return;

    /* Another program may have the library open briefly */
    after = master_opencnt();
    if (after != before)
    {
        Delay(50);
        after = master_opencnt();
    }

    bug("[smpnet] bases: %d workers x %d opens, null=%lu badsocket=%lu, opencnt %u -> %u\n",
        started, BASE_ROUNDS, (unsigned long)g_NullOpens, (unsigned long)g_BadSockets,
        before, after);
    check(g_NullOpens == 0, "every OpenLibrary returned a base");
    check(g_BadSockets == 0, "every socket() succeeded");
    check(after == before, "master base open count restored");
}

/* ---- 3. TCP echo over loopback ---- */

static volatile UWORD g_Port;           /* network order, 0 until listening */
static volatile ULONG g_ServerUp, g_ServerDone, g_ClientDone;
static volatile ULONG g_Served, g_TcpOK, g_TcpFail;

static int wait_ready(struct Library *SocketBase, LONG fd, BOOL forwrite)
{
    fd_set fs;
    struct timeval tv;

    FD_ZERO(&fs);
    FD_SET(fd, &fs);
    tv.tv_sec = IO_SECS;
    tv.tv_usec = 0;
    if (forwrite)
        return WaitSelect(fd + 1, NULL, &fs, NULL, &tv, NULL);
    return WaitSelect(fd + 1, &fs, NULL, NULL, &tv, NULL);
}

/* Receive exactly len bytes; FALSE on error, EOF or timeout. */
static BOOL recv_all(struct Library *SocketBase, LONG fd, UBYTE *buf, LONG len)
{
    while (len > 0)
    {
        LONG got;

        if (wait_ready(SocketBase, fd, FALSE) <= 0)
            return FALSE;
        got = recv(fd, buf, len, 0);
        if (got <= 0)
            return FALSE;
        buf += got;
        len -= got;
    }
    return TRUE;
}

static void loopback(struct sockaddr_in *sin, UWORD port)
{
    memset(sin, 0, sizeof(*sin));
    sin->sin_len = sizeof(*sin);
    sin->sin_family = AF_INET;
    sin->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    sin->sin_port = port;
}

static void EchoServer(void)
{
    struct Library *SocketBase = OpenLibrary("bsdsocket.library", 4);
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    UBYTE buf[MSGLEN];
    LONG lfd = -1;

    if (!SocketBase)
        goto out;
    lfd = socket(AF_INET, SOCK_STREAM, 0);
    loopback(&sin, 0);
    if (lfd < 0 || bind(lfd, (struct sockaddr *)&sin, sizeof(sin)) < 0 ||
        listen(lfd, 16) < 0 || getsockname(lfd, (struct sockaddr *)&sin, &len) < 0)
        goto out;
    g_Port = sin.sin_port;
    g_ServerUp = 1;

    while (!g_Stop)
    {
        LONG fd;

        if (wait_ready(SocketBase, lfd, FALSE) <= 0)
            continue;
        fd = accept(lfd, NULL, NULL);
        if (fd < 0)
            continue;
        if (recv_all(SocketBase, fd, buf, MSGLEN) &&
            wait_ready(SocketBase, fd, TRUE) > 0 &&
            send(fd, buf, MSGLEN, 0) == MSGLEN)
            __atomic_add_fetch(&g_Served, 1, __ATOMIC_RELAXED);
        CloseSocket(fd);
    }

out:
    if (lfd >= 0)
        CloseSocket(lfd);
    if (SocketBase)
        CloseLibrary(SocketBase);
    g_ServerUp = 1;             /* let main stop waiting on a failed start */
    __atomic_add_fetch(&g_ServerDone, 1, __ATOMIC_RELAXED);
}

static void EchoClient(void)
{
    ULONG id = (ULONG)(IPTR)FindTask(NULL)->tc_UserData;
    struct Library *SocketBase = OpenLibrary("bsdsocket.library", 4);
    UBYTE out[MSGLEN], in[MSGLEN];
    int r, i;

    for (r = 0; SocketBase && r < TCP_ROUNDS; r++)
    {
        struct sockaddr_in sin;
        LONG fd = socket(AF_INET, SOCK_STREAM, 0);
        BOOL ok = FALSE;

        for (i = 0; i < MSGLEN; i++)
            out[i] = (UBYTE)(id * 31 + r * 7 + i);
        loopback(&sin, g_Port);
        if (fd >= 0 &&
            connect(fd, (struct sockaddr *)&sin, sizeof(sin)) == 0 &&
            send(fd, out, MSGLEN, 0) == MSGLEN &&
            recv_all(SocketBase, fd, in, MSGLEN))
            ok = memcmp(in, out, MSGLEN) == 0;
        if (!ok)
            bug("[smpnet] client %lu round %d failed (errno %ld)\n",
                (unsigned long)id, r, (long)Errno());
        if (fd >= 0)
            CloseSocket(fd);
        __atomic_add_fetch(ok ? &g_TcpOK : &g_TcpFail, 1, __ATOMIC_RELAXED);
    }

    if (SocketBase)
        CloseLibrary(SocketBase);
    else
        __atomic_add_fetch(&g_TcpFail, TCP_ROUNDS, __ATOMIC_RELAXED);
    __atomic_add_fetch(&g_ClientDone, 1, __ATOMIC_RELAXED);
}

static void test_tcp(void)
{
    char names[MAXCPUS][32];
    int i, clients = 0;
    BOOL done;

    g_Stop = 0;
    g_Port = 0;
    g_ServerUp = g_ServerDone = g_ClientDone = 0;
    g_Served = g_TcpOK = g_TcpFail = 0;

    if (!spawn(EchoServer, "smpnet.server", 0, NULL) ||
        !wait_count(&g_ServerUp, 1, START_TICKS) || g_Port == 0)
    {
        bug("[smpnet] INVALID tcp: echo server did not start\n");
        invalid++;
        g_Stop = 1;
        wait_count(&g_ServerDone, 1, DONE_TICKS);
        return;
    }

    for (i = 0; i < g_NumCPUs; i++)
    {
        snprintf(names[i], sizeof(names[i]), "smpnet.client.%d", i);
        if (spawn(EchoClient, names[i], i, (APTR)(IPTR)i))
            clients++;
    }

    done = wait_count(&g_ClientDone, clients, DONE_TICKS);
    g_Stop = 1;
    wait_count(&g_ServerDone, 1, DONE_TICKS);

    bug("[smpnet] tcp: %d clients x %d rounds, ok=%lu fail=%lu served=%lu\n",
        clients, TCP_ROUNDS, (unsigned long)g_TcpOK, (unsigned long)g_TcpFail,
        (unsigned long)g_Served);
    check(done, "all tcp clients finished");
    check(g_TcpFail == 0 && g_TcpOK == (ULONG)(clients * TCP_ROUNDS),
          "every loopback echo matched");
}

int main(void)
{
    struct Library *SocketBase;

    KernelBase = OpenResource("kernel.resource");
    if (KernelBase)
        g_NumCPUs = KrnGetCPUCount();
    if (g_NumCPUs > MAXCPUS)
        g_NumCPUs = MAXCPUS;

    bug("[smpnet] start: cpus=%d\n", g_NumCPUs);

    if (!KernelBase || g_NumCPUs < 2)
    {
        bug("[smpnet] INVALID (needs at least 2 CPUs)\n");
        return RETURN_WARN;
    }

    test_pthread_start();

    SocketBase = OpenLibrary("bsdsocket.library", 4);
    if (SocketBase)
    {
        CloseLibrary(SocketBase);
        test_bases();
        test_tcp();
    }
    else
    {
        bug("[smpnet] INVALID bsdsocket: bsdsocket.library not available (stack not running?)\n");
        invalid++;
    }

    bug("[smpnet] DONE: %d PASS, %d FAIL, %d INVALID\n", pass, fail, invalid);
    return fail ? RETURN_FAIL : RETURN_OK;
}
#else
int main(void)
{
    return RETURN_FAIL;
}
#endif
