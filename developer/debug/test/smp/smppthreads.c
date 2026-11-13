/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: SMP coverage for pthreads (the native-task SMP tests do not use
          them; SMP-PThreads-Net covers sockets from pthreads). Threads are
          placed with the affinity API - by default they inherit their
          creator's CPUs:
          1. pthread_create() while every CPU hammers OpenLibrary() - the
             child's DosEntry waits on the LDDemon semaphore as it starts;
             each thread is created on one CPU (pthread_attr_setaffinity_np)
             and must run there
          2. pthread_setaffinity_np() moves a running thread to each CPU in
             turn; pthread_getaffinity_np() and sched_getcpu() confirm it
          A corrupted semaphore or a lost wakeup can show up as a hang or
          an alert instead of a FAIL line: a thread that never starts may
          own the LDDemon semaphore, and every later OpenLibrary() waits.
*/

#include <exec/tasks.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <dos/dostags.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/kernel.h>

#include <pthread.h>
#include <sched.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#if defined(__AROSEXEC_SMP__)
#define DEBUG 1
#include <aros/debug.h>

APTR KernelBase;
static int g_NumCPUs = 1;
static int pass, fail, invalid;

#define STACKSIZE       32768
#define MAXCPUS         8

#define THR_ROUNDS      100
#define THR_COUNT       8
#define START_TICKS     250     /* 5s for a thread to start */

#define DONE_TICKS      3000    /* 60s for a part to finish */

static volatile ULONG g_Stop;

static void check(BOOL ok, const char *what)
{
    if (ok)
    {
        bug("[smppthreads] %s OK\n", what);
        pass++;
    }
    else
    {
        bug("[smppthreads] *** FAIL *** %s\n", what);
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

/* ---- 1. thread start under OpenLibrary contention ---- */

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
    ULONG           ts_Want;        /* the cpu it was created on */
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
    ULONG cpus = 0, created = 0, misplaced = 0;
    int r, i, n, hammers = 0;
    BOOL ok = TRUE;

    g_Stop = 0;
    g_Hammers = 0;
    for (i = 0; i < g_NumCPUs; i++)
    {
        snprintf(names[i], sizeof(names[i]), "smppthreads.hammer.%d", i);
        if (spawn(Hammer, names[i], i, NULL))
            hammers++;
    }

    for (r = 0; r < THR_ROUNDS && ok; r++)
    {
        memset(slot, 0, sizeof(slot));
        for (n = 0; n < THR_COUNT; n++)
        {
            /* Each thread asks for one cpu, round robin over all of them */
            pthread_attr_t attr;
            cpu_set_t set;
            int err;

            slot[n].ts_Want = (r * THR_COUNT + n) % g_NumCPUs;
            CPU_ZERO(&set);
            CPU_SET(slot[n].ts_Want, &set);
            pthread_attr_init(&attr);
            pthread_attr_setaffinity_np(&attr, sizeof(set), &set);
            err = pthread_create(&thr[n], &attr, ThreadFunc, &slot[n]);
            pthread_attr_destroy(&attr);
            if (err != 0)
                break;
        }
        created += n;

        for (i = 0; i < n; i++)
        {
            int ticks = START_TICKS;

            while (!slot[i].ts_Started && ticks-- > 0)
                Delay(1);
            if (!slot[i].ts_Started)
            {
                /* It will never finish: joining it would hang the test */
                bug("[smppthreads] round %d: thread %d never started (lost wakeup?)\n", r, i);
                ok = FALSE;
                break;
            }
            cpus |= 1UL << slot[i].ts_CPU;
            if (slot[i].ts_CPU != slot[i].ts_Want)
                misplaced++;
        }
        if (!ok)
            break;
        for (i = 0; i < n; i++)
            pthread_join(thr[i], NULL);
        if (n < THR_COUNT)
        {
            bug("[smppthreads] round %d: pthread_create failed after %d threads\n", r, n);
            ok = FALSE;
        }
    }

    g_Stop = 1;
    wait_count(&g_Hammers, hammers, DONE_TICKS);

    bug("[smppthreads] pthread: %lu threads, cpu mask 0x%lx, %lu off their cpu, %d hammers\n",
        (unsigned long)created, (unsigned long)cpus, (unsigned long)misplaced, hammers);
    check(ok, "pthread start under OpenLibrary load");
    check(misplaced == 0, "every thread ran on the cpu it asked for");
}

/* ---- 2. moving a running thread ---- */

static void test_setaffinity(void)
{
    cpu_set_t set, back, saved;
    pthread_t self = pthread_self();
    int i, err, moved = 0, readback = 0;

    if ((err = pthread_getaffinity_np(self, sizeof(saved), &saved)) != 0)
    {
        bug("[smppthreads] pthread_getaffinity_np failed (%d)\n", err);
        check(FALSE, "reading this thread's affinity");
        return;
    }

    for (i = 0; i < g_NumCPUs; i++)
    {
        int ticks = START_TICKS;

        CPU_ZERO(&set);
        CPU_SET(i, &set);
        if (pthread_setaffinity_np(self, sizeof(set), &set) != 0)
            break;
        /* It moves at its CPU's next scheduling point */
        while (sched_getcpu() != i && ticks-- > 0)
            Delay(1);
        if (sched_getcpu() == i)
            moved++;
        if (pthread_getaffinity_np(self, sizeof(back), &back) == 0 &&
            CPU_COUNT(&back) == 1 && CPU_ISSET(i, &back))
            readback++;
    }
    pthread_setaffinity_np(self, sizeof(saved), &saved);

    bug("[smppthreads] setaffinity: moved to %d of %d cpus, %d read back\n",
        moved, g_NumCPUs, readback);
    check(moved == g_NumCPUs, "pthread_setaffinity_np moves the thread to each cpu");
    check(readback == g_NumCPUs, "pthread_getaffinity_np reads the set back");

    CPU_ZERO(&set);
    CPU_SET(CPU_SETSIZE - 1, &set);
    check(pthread_setaffinity_np(self, sizeof(set), &set) == EINVAL,
          "a set naming no existing cpu is rejected");
}

int main(void)
{
    KernelBase = OpenResource("kernel.resource");
    if (KernelBase)
        g_NumCPUs = KrnGetCPUCount();
    if (g_NumCPUs > MAXCPUS)
        g_NumCPUs = MAXCPUS;

    bug("[smppthreads] start: cpus=%d\n", g_NumCPUs);

    if (!KernelBase || g_NumCPUs < 2)
    {
        bug("[smppthreads] INVALID (needs at least 2 CPUs)\n");
        return RETURN_WARN;
    }

    test_pthread_start();
    test_setaffinity();

    bug("[smppthreads] DONE: %d PASS, %d FAIL, %d INVALID\n", pass, fail, invalid);
    return fail ? RETURN_FAIL : RETURN_OK;
}
#else
int main(void)
{
    return RETURN_FAIL;
}
#endif
