/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: SMP coverage for pthreads (the native-task SMP tests do not use
          them; SMP-PThreads-Net covers sockets from pthreads):
          pthread_create() while every CPU hammers OpenLibrary() - the
          child's DosEntry waits on the LDDemon semaphore as it starts.
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

/* ---- thread start under OpenLibrary contention ---- */

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
        snprintf(names[i], sizeof(names[i]), "smppthreads.hammer.%d", i);
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
                bug("[smppthreads] round %d: thread %d never started (lost wakeup?)\n", r, i);
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
            bug("[smppthreads] round %d: pthread_create failed after %d threads\n", r, n);
            ok = FALSE;
        }
    }

    g_Stop = 1;
    wait_count(&g_Hammers, hammers, DONE_TICKS);

    bug("[smppthreads] pthread: %lu threads, cpu mask 0x%lx, %d hammers\n",
        (unsigned long)created, (unsigned long)cpus, hammers);
    if (ok && (cpus & ~1UL) == 0)
    {
        bug("[smppthreads] INVALID pthread start: no thread ran off cpu 0\n");
        invalid++;
    }
    else
        check(ok, "pthread start under OpenLibrary load");
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

    bug("[smppthreads] DONE: %d PASS, %d FAIL, %d INVALID\n", pass, fail, invalid);
    return fail ? RETURN_FAIL : RETURN_OK;
}
#else
int main(void)
{
    return RETURN_FAIL;
}
#endif
