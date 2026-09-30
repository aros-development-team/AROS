/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: SMP coverage for starting threads and using bsdsocket.library from
          several CPUs at once:
          1. pthread_create() while every CPU hammers OpenLibrary() - the
             child's DosEntry waits on the LDDemon semaphore as it starts
          2. per-task bsdsocket bases opened and closed on every CPU
          3. TCP echo over loopback with a client on every CPU; every other
             round connects non-blocking and waits in WaitSelect()
          4. sockets handed between tasks on different CPUs with
             ReleaseSocket()/ReleaseCopyOfSocket() and ObtainSocket()
          5. posixc read()/write()/close() from threads that never
             opened bsdsocket.library, and an exited FIOSETOWN owner
          A corrupted semaphore or a lost wakeup can show up as a hang or
          an alert instead of a FAIL line: a thread that never starts may
          own the LDDemon semaphore, and every later OpenLibrary() waits.
          Parts 2-5 need the TCP/IP stack running.
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
#include <sys/ioctl.h>
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

#define HAND_ROUNDS     200

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

/* Non-blocking connect: wait until writable, then back to blocking */
static BOOL connect_nbio(struct Library *SocketBase, LONG fd, struct sockaddr_in *sin)
{
    LONG on = 1, off = 0, err = 0;
    socklen_t len = sizeof(err);

    if (IoctlSocket(fd, FIONBIO, (char *)&on) < 0)
        return FALSE;
    if (connect(fd, (struct sockaddr *)sin, sizeof(*sin)) < 0 && Errno() != EINPROGRESS)
        return FALSE;
    if (wait_ready(SocketBase, fd, TRUE) <= 0 ||
        getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0 || err != 0)
        return FALSE;
    return IoctlSocket(fd, FIONBIO, (char *)&off) == 0;
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
        LONG tos = 0x10;
        BOOL ok = FALSE;

        for (i = 0; i < MSGLEN; i++)
            out[i] = (UBYTE)(id * 31 + r * 7 + i);
        loopback(&sin, g_Port);
        if (fd >= 0 &&
            setsockopt(fd, IPPROTO_IP, IP_TOS, &tos, sizeof(tos)) == 0 &&
            ((r & 1) ? connect_nbio(SocketBase, fd, &sin)
                     : connect(fd, (struct sockaddr *)&sin, sizeof(sin)) == 0) &&
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

/* ---- 4. socket hand-off between CPUs ---- */

#define HAND_MAX (MAXCPUS * HAND_ROUNDS)

static volatile LONG g_Ids[HAND_MAX];
static volatile ULONG g_IdReady[HAND_MAX];
static volatile ULONG g_Pushed, g_Popped, g_HandDone, g_HandFail;
static ULONG g_HandTotal;

static void Giver(void)
{
    struct Library *SocketBase = OpenLibrary("bsdsocket.library", 4);
    int r;

    for (r = 0; r < HAND_ROUNDS; r++)
    {
        ULONG slot = __atomic_fetch_add(&g_Pushed, 1, __ATOMIC_RELAXED);
        LONG fd = SocketBase ? socket(AF_INET, SOCK_STREAM, 0) : -1;
        LONG id = -1;

        if (fd >= 0)
        {
            /* Every other one is a copy, so the refcount goes 2 -> 1 -> 0 */
            if (r & 1)
            {
                id = ReleaseCopyOfSocket(fd, UNIQUE_ID);
                CloseSocket(fd);
            }
            else
                id = ReleaseSocket(fd, UNIQUE_ID);
        }
        if (id == -1)
            __atomic_add_fetch(&g_HandFail, 1, __ATOMIC_RELAXED);
        g_Ids[slot] = id;
        __atomic_store_n(&g_IdReady[slot], 1, __ATOMIC_RELEASE);
    }
    if (SocketBase)
        CloseLibrary(SocketBase);
    __atomic_add_fetch(&g_HandDone, 1, __ATOMIC_RELAXED);
}

static void Taker(void)
{
    struct Library *SocketBase = OpenLibrary("bsdsocket.library", 4);
    ULONG slot;

    while (SocketBase &&
           (slot = __atomic_fetch_add(&g_Popped, 1, __ATOMIC_RELAXED)) < g_HandTotal)
    {
        LONG fd;
        int ticks = START_TICKS;

        while (!__atomic_load_n(&g_IdReady[slot], __ATOMIC_ACQUIRE) && ticks-- > 0)
            Delay(1);
        if (g_Ids[slot] == -1)
            continue;       /* the giver already counted it */
        fd = ObtainSocket(g_Ids[slot], AF_INET, SOCK_STREAM, 0);
        if (fd < 0)
        {
            bug("[smpnet] ObtainSocket(%ld) failed (errno %ld)\n", (long)g_Ids[slot], (long)Errno());
            __atomic_add_fetch(&g_HandFail, 1, __ATOMIC_RELAXED);
        }
        else
            CloseSocket(fd);
    }
    if (SocketBase)
        CloseLibrary(SocketBase);
    __atomic_add_fetch(&g_HandDone, 1, __ATOMIC_RELAXED);
}

static void test_handoff(void)
{
    struct Library *SocketBase = OpenLibrary("bsdsocket.library", 4);
    char names[MAXCPUS * 2][32];
    LONG before, after;
    int i, j, tasks = 0, dups = 0;
    BOOL done;

    if (!SocketBase)
        return;
    before = socket(AF_INET, SOCK_STREAM, 0);
    CloseSocket(before);

    g_Pushed = g_Popped = g_HandDone = g_HandFail = 0;
    g_HandTotal = g_NumCPUs * HAND_ROUNDS;
    memset((void *)g_IdReady, 0, sizeof(g_IdReady));

    for (i = 0; i < g_NumCPUs; i++)
    {
        snprintf(names[2 * i], sizeof(names[0]), "smpnet.giver.%d", i);
        snprintf(names[2 * i + 1], sizeof(names[0]), "smpnet.taker.%d", i);
        if (spawn(Giver, names[2 * i], i, NULL))
            tasks++;
        if (spawn(Taker, names[2 * i + 1], (i + 1) % g_NumCPUs, NULL))
            tasks++;
    }

    done = wait_count(&g_HandDone, tasks, DONE_TICKS);

    for (i = 0; i < (int)g_HandTotal; i++)
        for (j = i + 1; j < (int)g_HandTotal; j++)
            if (g_Ids[i] != -1 && g_Ids[i] == g_Ids[j])
                dups++;

    after = socket(AF_INET, SOCK_STREAM, 0);
    CloseSocket(after);
    CloseLibrary(SocketBase);

    bug("[smpnet] handoff: %d tasks, %lu sockets, fail=%lu dup ids=%d, fd %ld -> %ld\n",
        tasks, (unsigned long)g_HandTotal, (unsigned long)g_HandFail, dups,
        (long)before, (long)after);
    check(done, "all hand-off tasks finished");
    check(g_HandFail == 0, "every release and obtain succeeded");
    check(dups == 0, "released ids are unique");
    check(after == before, "no descriptor number leaked");
}

/* ---- 5. posixc threads on a socket they did not open ---- */

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

static volatile ULONG g_OwnerUp, g_OwnerGo;

static void ShortOwner(void)
{
    struct Library *SocketBase = OpenLibrary("bsdsocket.library", 4);

    g_OwnerUp = 1;
    while (!g_OwnerGo)
        Delay(1);
    if (SocketBase)
        CloseLibrary(SocketBase);
    g_OwnerUp = 2;
}

static void test_posix_threads(void)
{
    UWORD cnt0 = master_opencnt(), cnt1;    /* before our own base */
    struct Library *SocketBase = OpenLibrary("bsdsocket.library", 4);
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    pthread_t thr[MAXCPUS + 1];
    struct Task *owner = NULL, *got = (struct Task *)1;
    struct Process *op;
    LONG l, before, after, on = 1;
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

    /* A socket owned by a task that has exited must not name its base */
    op = spawn(ShortOwner, "smpnet.owner", 1, NULL);
    wait_count(&g_OwnerUp, 1, START_TICKS);
    l = socket(AF_INET, SOCK_STREAM, 0);
    owner = (struct Task *)op;
    IoctlSocket(l, FIOSETOWN, (char *)&owner);
    IoctlSocket(l, FIOASYNC, (char *)&on);
    g_OwnerGo = 1;
    wait_count(&g_OwnerUp, 2, START_TICKS);
    Delay(10);
    IoctlSocket(l, FIOGETOWN, (char *)&got);
    CloseSocket(l);

    after = socket(AF_INET, SOCK_STREAM, 0);
    CloseSocket(after);
    CloseLibrary(SocketBase);
    Delay(10);
    cnt1 = master_opencnt();

    bug("[smpnet] posix: %lu writers, read %lu bytes, bad=%lu, close=%ld, fd %ld -> %ld, opencnt %u -> %u, owner after exit %p\n",
        (unsigned long)g_PxWriters, (unsigned long)g_PxRead, (unsigned long)g_PxBad,
        (long)g_PxCloseRet, (long)before, (long)after, cnt0, cnt1, got);
    check(done && g_PxRead == g_PxWriters * PX_ROUNDS * PX_CHUNK, "posixc threads moved all the data");
    check(g_PxBad == 0, "no write failed or was split");
    check(g_PxCloseRet == 0, "another thread closed the socket");
    check(after == before && cnt1 == cnt0, "no base or descriptor leaked");
    check(got == NULL, "an exited owner is forgotten");
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
        test_handoff();
        test_posix_threads();
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
