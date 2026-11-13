/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: SMP stress test for exec primitives.
          Tests concurrent semaphores, pool allocation, message passing,
          and software interrupts from multiple tasks pinned across CPUs.
*/

#include <aros/config.h>

#include <exec/semaphores.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <exec/interrupts.h>
#include <exec/tasks.h>
#include <utility/tagitem.h>
#include <dos/dos.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/kernel.h>

#include <clib/alib_protos.h>

#include <stdio.h>
#include <string.h>

#if defined(__AROSEXEC_SMP__)
#define DEBUG 1
#include <aros/debug.h>

#if defined(__AROSEXEC_SMP__)
#include <aros/atomic.h>
#endif

APTR KernelBase;
static int g_NumCPUs = 1;

#define NUM_WORKERS     4
#define STACKSIZE       16384
static cpumask_t *g_WorkerMasks[NUM_WORKERS + 1];

/* Number of iterations per test */
#define SEM_ITERS       50000
#define POOL_ITERS      5000
#define MSG_ITERS       10000
#define SOFTINT_ITERS   5000

/* Signals for synchronization */
#define SIGF_READY      SIGBREAKF_CTRL_D
#define SIGF_GO         SIGBREAKF_CTRL_E
#define SIGF_DONE       SIGBREAKF_CTRL_F

/*
 * Helper: wait for all workers to signal SIGF_READY / SIGF_DONE.
 * Signals are bitmasks and coalesce — if multiple workers signal
 * before the main task wakes, only one Wait() return occurs.
 * We use a shared volatile counter to track actual completions.
 */
static BOOL g_Aborted = FALSE;

static BOOL WaitAllSignals(const char *phase, ULONG sigf, volatile LONG *counter, int target)
{
    ULONG stagnant_ticks = 0;
    LONG last = *counter;

    while (*counter < target)
    {
        ULONG sigs = SetSignal(0, 0);
        if (sigs & SIGBREAKF_CTRL_C)
        {
            g_Aborted = TRUE;
            return FALSE;
        }

        if (*counter != last)
        {
            last = *counter;
            stagnant_ticks = 0;
        }
        else if (++stagnant_ticks >= 250)
        {
            bug(" TIMEOUT (%s %ld/%d)\n", phase, (long)*counter, target);
            g_Aborted = TRUE;
            return FALSE;
        }

        Delay(1);
    }

    return TRUE;
}

/*
 * Create a worker task pinned to CPU (worker_index % g_NumCPUs).
 * Spreads workers across all available CPUs for real SMP stress.
 *
 * NOTE: CleanupETask frees the task's iet_CpuAffinity via KrnFreeCPUMask,
 * so our g_WorkerMasks[i] pointer becomes invalid after RemTask.
 * Call InvalidateWorkerMasks() after each RemTask batch.
 */
static struct Task *CreateWorkerTask(const char *name, APTR entry, int worker_index,
                                     LONG pri, IPTR userdata)
{
    struct Task *t;

    if (KernelBase && g_NumCPUs > 1)
    {
        if (worker_index >= 0 && worker_index <= NUM_WORKERS)
        {
            if (!g_WorkerMasks[worker_index])
                g_WorkerMasks[worker_index] = KrnAllocCPUMask();

            if (g_WorkerMasks[worker_index])
            {
                KrnClearCPUMask(g_WorkerMasks[worker_index]);
                KrnGetCPUMask(worker_index % g_NumCPUs, g_WorkerMasks[worker_index]);
                t = NewCreateTask(TASKTAG_NAME, (IPTR)name,
                                  TASKTAG_PRI, pri,
                                  TASKTAG_PC, (IPTR)entry,
                                  TASKTAG_STACKSIZE, STACKSIZE,
                                  TASKTAG_USERDATA, userdata,
                                  TASKTAG_AFFINITY, (IPTR)g_WorkerMasks[worker_index],
                                  TAG_DONE);
                return t;
            }
        }
    }

    return NewCreateTask(TASKTAG_NAME, (IPTR)name,
                         TASKTAG_PRI, pri,
                         TASKTAG_PC, (IPTR)entry,
                         TASKTAG_STACKSIZE, STACKSIZE,
                         TASKTAG_USERDATA, userdata,
                         TAG_DONE);
}

/*
 * CleanupETask calls KrnFreeCPUMask on iet_CpuAffinity, which frees
 * the memory our g_WorkerMasks[] entries point to.  Clear the pointers
 * so CreateWorkerTask re-allocates fresh masks for the next test.
 */
static void InvalidateWorkerMasks(void)
{
    int i;
    for (i = 0; i <= NUM_WORKERS; i++)
        g_WorkerMasks[i] = NULL;
}

/******************************************************************************/

/* Shared state for semaphore test */
struct SemTestData
{
    struct SignalSemaphore  std_Sem;
    struct Task            *std_MainTask;
    volatile LONG           std_Counter;
    volatile LONG           std_Errors;
    volatile LONG           std_ReadyCount;
    volatile LONG           std_DoneCount;
    ULONG                   std_Iters;
};

/* Shared state for pool test */
struct PoolTestData
{
    APTR                    ptd_Pool;
    struct Task            *ptd_MainTask;
    volatile LONG           ptd_Errors;
    volatile LONG           ptd_ReadyCount;
    volatile LONG           ptd_DoneCount;
    ULONG                   ptd_Iters;
};

/* Shared state for message test */
struct MsgTestData
{
    struct MsgPort         *mtd_ServerPort;
    struct Task            *mtd_MainTask;
    volatile LONG           mtd_Received;
    volatile LONG           mtd_Errors;
    volatile LONG           mtd_ReadyCount;
    volatile LONG           mtd_DoneCount;
    ULONG                   mtd_Iters;
};

/* Simple test message */
struct TestMessage
{
    struct Message  tm_Msg;
    ULONG           tm_Value;
    ULONG           tm_WorkerID;
};

/* Shared state for softint test */
struct SoftIntTestData
{
    struct Task            *sitd_MainTask;
    volatile LONG           sitd_Counter;
    volatile LONG           sitd_Errors;
    volatile LONG           sitd_ReadyCount;
    volatile LONG           sitd_DoneCount;
    ULONG                   sitd_Iters;
};

/******************************************************************************/
/*  Test 1: Semaphore contention                                              */
/*                                                                            */
/*  Multiple tasks obtain/release a shared semaphore while incrementing a     */
/*  shared counter. The final counter value must equal NUM_WORKERS * iters.   */
/******************************************************************************/

static void SemWorker(void)
{
    struct Task *me = FindTask(NULL);
    struct SemTestData *data = (struct SemTestData *)me->tc_UserData;
    ULONG i;

    __sync_add_and_fetch(&data->std_ReadyCount, 1);
    Signal(data->std_MainTask, SIGF_READY);
    Wait(SIGF_GO);

    for (i = 0; i < data->std_Iters; i++)
    {
        ObtainSemaphore(&data->std_Sem);
        data->std_Counter++;
        ReleaseSemaphore(&data->std_Sem);
        D(
            if (i % 1000 == 0)
                bug("[smpstress] SemWorker '%s' iter %lu/%lu\n",
                    me->tc_Node.ln_Name, (unsigned long)i, (unsigned long)data->std_Iters);
        )
    }

    __sync_add_and_fetch(&data->std_DoneCount, 1);
    Signal(data->std_MainTask, SIGF_DONE);
    Wait(0);
}

static void SemSharedWorker(void)
{
    struct Task *me = FindTask(NULL);
    struct SemTestData *data = (struct SemTestData *)me->tc_UserData;
    ULONG i;
    LONG val;

    __sync_add_and_fetch(&data->std_ReadyCount, 1);
    Signal(data->std_MainTask, SIGF_READY);
    Wait(SIGF_GO);

    for (i = 0; i < data->std_Iters; i++)
    {
        ObtainSemaphoreShared(&data->std_Sem);
        val = data->std_Counter;
        if (val < 0)
            __sync_add_and_fetch(&data->std_Errors, 1);
        ReleaseSemaphore(&data->std_Sem);
        D(
            if (i % 1000 == 0)
                bug("[smpstress] SemSharedWorker '%s' iter %lu/%lu\n",
                    me->tc_Node.ln_Name, (unsigned long)i, (unsigned long)data->std_Iters);
        )
    }

    __sync_add_and_fetch(&data->std_DoneCount, 1);
    Signal(data->std_MainTask, SIGF_DONE);
    Wait(0);
}

static BOOL TestSemaphores(ULONG iters)
{
    struct SemTestData data;
    struct Task *workers[NUM_WORKERS];
    char names[NUM_WORKERS][32];
    ULONG expected;
    int i;

    bug("  Semaphore exclusive contention (%u iters x %d tasks)...", (unsigned)iters, NUM_WORKERS);

    memset(&data, 0, sizeof(data));
    InitSemaphore(&data.std_Sem);
    data.std_MainTask = FindTask(NULL);
    data.std_Iters = iters;

    for (i = 0; i < NUM_WORKERS; i++)
    {
        snprintf(names[i], sizeof(names[i]), "SemWorker.%d", i);
        workers[i] = CreateWorkerTask(names[i], SemWorker, i, 0, (IPTR)&data);
        if (!workers[i])
        {
            bug(" FAIL (can't create task %d)\n", i);
            Forbid();
            while (--i >= 0)
                RemTask(workers[i]);
            Permit();
            InvalidateWorkerMasks();
            return FALSE;
        }
    }

    if (!WaitAllSignals("sem-ready", SIGF_READY, &data.std_ReadyCount, NUM_WORKERS))
    {
        Forbid();
        for (i = 0; i < NUM_WORKERS; i++)
            if (workers[i])
                RemTask(workers[i]);
        Permit();
        InvalidateWorkerMasks();
        return FALSE;
    }

    Forbid();
    for (i = 0; i < NUM_WORKERS; i++)
        Signal(workers[i], SIGF_GO);
    Permit();

    expected = (ULONG)NUM_WORKERS * iters;

    /* Monitor std_Counter (increments every iteration) instead of
     * std_DoneCount (increments only when a worker finishes ALL iters).
     * With 200k sequential semaphore handoffs on Pi hardware, no single
     * worker finishes within 5s, but the counter shows steady progress. */
    if (!WaitAllSignals("sem-done", SIGF_DONE, &data.std_Counter, (int)expected))
    {
        Forbid();
        for (i = 0; i < NUM_WORKERS; i++)
            if (workers[i])
                RemTask(workers[i]);
        Permit();
        InvalidateWorkerMasks();
        return FALSE;
    }

    /* Workers are finishing up — brief wait for DoneCount */
    if (!WaitAllSignals("sem-cleanup", SIGF_DONE, &data.std_DoneCount, NUM_WORKERS))
    {
        Forbid();
        for (i = 0; i < NUM_WORKERS; i++)
            if (workers[i])
                RemTask(workers[i]);
        Permit();
        InvalidateWorkerMasks();
        return FALSE;
    }

    Forbid();
    for (i = 0; i < NUM_WORKERS; i++)
        RemTask(workers[i]);
    Permit();
    InvalidateWorkerMasks();

    if ((ULONG)data.std_Counter != expected)
    {
        bug(" FAIL (counter=%ld, expected=%lu)\n", (long)data.std_Counter, (unsigned long)expected);
        return FALSE;
    }
    bug(" OK\n");

    /* Now test shared + exclusive mix */
    bug("  Semaphore shared/exclusive mix (%u iters x %d tasks)...", (unsigned)iters, NUM_WORKERS);

    data.std_Counter = 0;
    data.std_Errors = 0;
    data.std_ReadyCount = 0;
    data.std_DoneCount = 0;

    for (i = 0; i < NUM_WORKERS; i++)
    {
        snprintf(names[i], sizeof(names[i]), "SemMixWorker.%d", i);
        workers[i] = CreateWorkerTask(names[i],
                                      i < NUM_WORKERS / 2 ? (APTR)SemWorker : (APTR)SemSharedWorker,
                                      i, 0, (IPTR)&data);
        if (!workers[i])
        {
            bug(" FAIL (can't create task %d)\n", i);
            Forbid();
            while (--i >= 0)
                RemTask(workers[i]);
            Permit();
            InvalidateWorkerMasks();
            return FALSE;
        }
    }

    if (!WaitAllSignals("sem-mix-ready", SIGF_READY, &data.std_ReadyCount, NUM_WORKERS))
    {
        Forbid();
        for (i = 0; i < NUM_WORKERS; i++)
            if (workers[i])
                RemTask(workers[i]);
        Permit();
        InvalidateWorkerMasks();
        return FALSE;
    }

    Forbid();
    for (i = 0; i < NUM_WORKERS; i++)
        Signal(workers[i], SIGF_GO);
    Permit();

    expected = (ULONG)(NUM_WORKERS / 2) * iters;

    if (!WaitAllSignals("sem-mix-done", SIGF_DONE, &data.std_Counter, (int)expected))
    {
        Forbid();
        for (i = 0; i < NUM_WORKERS; i++)
            if (workers[i])
                RemTask(workers[i]);
        Permit();
        InvalidateWorkerMasks();
        return FALSE;
    }

    if (!WaitAllSignals("sem-mix-cleanup", SIGF_DONE, &data.std_DoneCount, NUM_WORKERS))
    {
        Forbid();
        for (i = 0; i < NUM_WORKERS; i++)
            if (workers[i])
                RemTask(workers[i]);
        Permit();
        InvalidateWorkerMasks();
        return FALSE;
    }

    Forbid();
    for (i = 0; i < NUM_WORKERS; i++)
        RemTask(workers[i]);
    Permit();
    InvalidateWorkerMasks();

    if ((ULONG)data.std_Counter != expected || data.std_Errors != 0)
    {
        bug(" FAIL (counter=%ld, expected=%lu, errors=%ld)\n",
               (long)data.std_Counter, (unsigned long)expected, (long)data.std_Errors);
        return FALSE;
    }
    bug(" OK\n");
    return TRUE;
}

/******************************************************************************/
/*  Test 2: Pool allocation stress                                            */
/*                                                                            */
/*  Multiple tasks allocate and free from a shared pool. Tests that the pool  */
/*  allocator doesn't corrupt its internal structures under contention.       */
/******************************************************************************/

static void PoolWorker(void)
{
    struct Task *me = FindTask(NULL);
    struct PoolTestData *data = (struct PoolTestData *)me->tc_UserData;
    ULONG i;

    __sync_add_and_fetch(&data->ptd_ReadyCount, 1);
    Signal(data->ptd_MainTask, SIGF_READY);
    Wait(SIGF_GO);

    for (i = 0; i < data->ptd_Iters; i++)
    {
        ULONG size = 16 + (i % 7) * 32;
        APTR mem = AllocPooled(data->ptd_Pool, size);
        if (!mem)
        {
            __sync_add_and_fetch(&data->ptd_Errors, 1);
            continue;
        }
        memset(mem, 0xAA, size);
        FreePooled(data->ptd_Pool, mem, size);
        D(
            if (i % 1000 == 0)
                bug("[smpstress] PoolWorker '%s' iter %lu/%lu\n",
                    me->tc_Node.ln_Name, (unsigned long)i, (unsigned long)data->ptd_Iters);
        )
    }

    __sync_add_and_fetch(&data->ptd_DoneCount, 1);
    Signal(data->ptd_MainTask, SIGF_DONE);
    Wait(0);
}

static BOOL TestPools(ULONG iters)
{
    struct PoolTestData data;
    struct Task *workers[NUM_WORKERS];
    char names[NUM_WORKERS][32];
    int i;

    bug("  Pool concurrent alloc/free (%u iters x %d tasks)...", (unsigned)iters, NUM_WORKERS);

    memset(&data, 0, sizeof(data));
    data.ptd_Pool = CreatePool(MEMF_ANY | MEMF_SEM_PROTECTED, 8192, 4096);
    data.ptd_MainTask = FindTask(NULL);
    data.ptd_Iters = iters;

    if (!data.ptd_Pool)
    {
        bug(" FAIL (can't create pool)\n");
        return FALSE;
    }

    for (i = 0; i < NUM_WORKERS; i++)
    {
        snprintf(names[i], sizeof(names[i]), "PoolWorker.%d", i);
        workers[i] = CreateWorkerTask(names[i], PoolWorker, i, 0, (IPTR)&data);
        if (!workers[i])
        {
            bug(" FAIL (can't create task %d)\n", i);
            Forbid();
            while (--i >= 0)
                RemTask(workers[i]);
            Permit();
            InvalidateWorkerMasks();
            DeletePool(data.ptd_Pool);
            return FALSE;
        }
    }

    if (!WaitAllSignals("pool-ready", SIGF_READY, &data.ptd_ReadyCount, NUM_WORKERS))
    {
        Forbid();
        for (i = 0; i < NUM_WORKERS; i++)
            if (workers[i])
                RemTask(workers[i]);
        Permit();
        InvalidateWorkerMasks();
        DeletePool(data.ptd_Pool);
        return FALSE;
    }

    Forbid();
    for (i = 0; i < NUM_WORKERS; i++)
        Signal(workers[i], SIGF_GO);
    Permit();

    if (!WaitAllSignals("pool-done", SIGF_DONE, &data.ptd_DoneCount, NUM_WORKERS))
    {
        Forbid();
        for (i = 0; i < NUM_WORKERS; i++)
            if (workers[i])
                RemTask(workers[i]);
        Permit();
        InvalidateWorkerMasks();
        DeletePool(data.ptd_Pool);
        return FALSE;
    }

    Forbid();
    for (i = 0; i < NUM_WORKERS; i++)
        RemTask(workers[i]);
    Permit();
    InvalidateWorkerMasks();

    DeletePool(data.ptd_Pool);

    if (data.ptd_Errors != 0)
    {
        bug(" FAIL (%ld allocation failures)\n", (long)data.ptd_Errors);
        return FALSE;
    }
    bug(" OK\n");
    return TRUE;
}

/******************************************************************************/
/*  Test 3: Message passing stress                                            */
/*                                                                            */
/*  Multiple tasks send messages to a single server port. The server task     */
/*  receives and replies. Tests PutMsg/GetMsg/ReplyMsg under contention.      */
/******************************************************************************/

static void MsgSendWorker(void)
{
    struct Task *me = FindTask(NULL);
    struct MsgTestData *data = (struct MsgTestData *)me->tc_UserData;
    struct MsgPort *replyPort;
    struct TestMessage msg;
    ULONG i;

    replyPort = CreateMsgPort();
    if (!replyPort)
    {
        __sync_add_and_fetch(&data->mtd_Errors, 1);
        __sync_add_and_fetch(&data->mtd_ReadyCount, 1);
        Signal(data->mtd_MainTask, SIGF_READY);
        Wait(SIGF_GO);
        __sync_add_and_fetch(&data->mtd_DoneCount, 1);
        Signal(data->mtd_MainTask, SIGF_DONE);
        Wait(0);
        return;
    }

    __sync_add_and_fetch(&data->mtd_ReadyCount, 1);
    Signal(data->mtd_MainTask, SIGF_READY);
    Wait(SIGF_GO);

    memset(&msg, 0, sizeof(msg));
    msg.tm_Msg.mn_ReplyPort = replyPort;
    msg.tm_Msg.mn_Length = sizeof(struct TestMessage);

    for (i = 0; i < data->mtd_Iters; i++)
    {
        msg.tm_Value = i;
        PutMsg(data->mtd_ServerPort, &msg.tm_Msg);
        WaitPort(replyPort);
        GetMsg(replyPort);
        D(
            if (i % 1000 == 0)
                bug("[smpstress] MsgSender '%s' iter %lu/%lu\n",
                    me->tc_Node.ln_Name, (unsigned long)i, (unsigned long)data->mtd_Iters);
        )
    }

    DeleteMsgPort(replyPort);
    __sync_add_and_fetch(&data->mtd_DoneCount, 1);
    Signal(data->mtd_MainTask, SIGF_DONE);
    Wait(0);
}

static void MsgServerTask(void)
{
    struct Task *me = FindTask(NULL);
    struct MsgTestData *data = (struct MsgTestData *)me->tc_UserData;
    struct MsgPort *port;
    ULONG total = (ULONG)NUM_WORKERS * data->mtd_Iters;
    ULONG count = 0;

    /* Server must create its own port so mp_SigTask = this task.
     * If main created it, PutMsg would signal main instead of us. */
    port = CreateMsgPort();
    if (!port)
    {
        __sync_add_and_fetch(&data->mtd_Errors, 1);
        __sync_add_and_fetch(&data->mtd_ReadyCount, 1);
        Signal(data->mtd_MainTask, SIGF_READY);
        Wait(SIGF_GO);
        __sync_add_and_fetch(&data->mtd_DoneCount, 1);
        Signal(data->mtd_MainTask, SIGF_DONE);
        Wait(0);
        return;
    }

    /* Publish port so senders can find it */
    data->mtd_ServerPort = port;

    __sync_add_and_fetch(&data->mtd_ReadyCount, 1);
    Signal(data->mtd_MainTask, SIGF_READY);
    Wait(SIGF_GO);

    while (count < total)
    {
        struct TestMessage *tmsg;

        WaitPort(port);
        while ((tmsg = (struct TestMessage *)GetMsg(port)) != NULL)
        {
            count++;
            ReplyMsg(&tmsg->tm_Msg);
            D(
                if (count % 1000 == 0)
                    bug("[smpstress] MsgServer received %lu/%lu\n",
                        (unsigned long)count, (unsigned long)total);
            )
        }
    }

    data->mtd_Received = count;
    DeleteMsgPort(port);
    data->mtd_ServerPort = NULL;
    __sync_add_and_fetch(&data->mtd_DoneCount, 1);
    Signal(data->mtd_MainTask, SIGF_DONE);
    Wait(0);
}

static BOOL TestMessages(ULONG iters)
{
    struct MsgTestData data;
    struct Task *workers[NUM_WORKERS];
    struct Task *server;
    char names[NUM_WORKERS][32];
    ULONG expected;
    int i;

    bug("  Message passing (%u iters x %d senders)...", (unsigned)iters, NUM_WORKERS);

    memset(&data, 0, sizeof(data));
    data.mtd_MainTask = FindTask(NULL);
    data.mtd_Iters = iters;

    /* Server task creates its own port (so mp_SigTask is correct) */
    server = CreateWorkerTask("MsgServer", MsgServerTask, 0, 1, (IPTR)&data);
    if (!server)
    {
        bug(" FAIL (can't create server task)\n");
        return FALSE;
    }

    /* Wait for server to be ready (port is now published) */
    if (!WaitAllSignals("msg-server-ready", SIGF_READY, &data.mtd_ReadyCount, 1))
    {
        Forbid();
        RemTask(server);
        Permit();
        InvalidateWorkerMasks();
        return FALSE;
    }

    if (!data.mtd_ServerPort)
    {
        bug(" FAIL (server couldn't create port)\n");
        Forbid();
        RemTask(server);
        Permit();
        InvalidateWorkerMasks();
        return FALSE;
    }

    /* Create sender tasks — server port is guaranteed valid now */
    for (i = 0; i < NUM_WORKERS; i++)
    {
        snprintf(names[i], sizeof(names[i]), "MsgSender.%d", i);
        workers[i] = CreateWorkerTask(names[i], MsgSendWorker, i + 1, 0, (IPTR)&data);
        if (!workers[i])
        {
            bug(" FAIL (can't create sender %d)\n", i);
            Signal(server, SIGF_GO);
            Forbid();
            RemTask(server);
            while (--i >= 0)
                RemTask(workers[i]);
            Permit();
            InvalidateWorkerMasks();
            return FALSE;
        }
    }

    /* Wait for all senders to be ready */
    if (!WaitAllSignals("msg-ready", SIGF_READY, &data.mtd_ReadyCount, NUM_WORKERS + 1))
    {
        Forbid();
        for (i = 0; i < NUM_WORKERS; i++)
            if (workers[i])
                RemTask(workers[i]);
        RemTask(server);
        Permit();
        InvalidateWorkerMasks();
        return FALSE;
    }

    /* Start everyone simultaneously */
    Forbid();
    Signal(server, SIGF_GO);
    for (i = 0; i < NUM_WORKERS; i++)
        Signal(workers[i], SIGF_GO);
    Permit();

    /* Wait for all senders + server to finish */
    if (!WaitAllSignals("msg-done", SIGF_DONE, &data.mtd_DoneCount, NUM_WORKERS + 1))
    {
        Forbid();
        for (i = 0; i < NUM_WORKERS; i++)
            if (workers[i])
                RemTask(workers[i]);
        RemTask(server);
        Permit();
        InvalidateWorkerMasks();
        return FALSE;
    }

    Forbid();
    for (i = 0; i < NUM_WORKERS; i++)
        RemTask(workers[i]);
    RemTask(server);
    Permit();
    InvalidateWorkerMasks();

    expected = (ULONG)NUM_WORKERS * iters;
    if ((ULONG)data.mtd_Received != expected || data.mtd_Errors != 0)
    {
        bug(" FAIL (received=%ld, expected=%lu, errors=%ld)\n",
               (long)data.mtd_Received, (unsigned long)expected, (long)data.mtd_Errors);
        return FALSE;
    }
    bug(" OK\n");
    return TRUE;
}

/******************************************************************************/
/*  Test 4: Software interrupt stress                                         */
/*                                                                            */
/*  Multiple tasks Cause() software interrupts that increment a counter.      */
/*  Tests the SoftInts list protection under contention.                      */
/******************************************************************************/

static volatile LONG g_SoftIntCounter;

AROS_INTH1(SoftIntHandler, APTR, data)
{
    AROS_INTFUNC_INIT

    __sync_add_and_fetch(&g_SoftIntCounter, 1);

    return FALSE;

    AROS_INTFUNC_EXIT
}

static void SoftIntWorker(void)
{
    struct Task *me = FindTask(NULL);
    struct SoftIntTestData *data = (struct SoftIntTestData *)me->tc_UserData;
    struct Interrupt softint;
    ULONG i;

    memset(&softint, 0, sizeof(softint));
    softint.is_Code = (VOID_FUNC)SoftIntHandler;
    softint.is_Data = NULL;
    softint.is_Node.ln_Type = NT_INTERRUPT;
    softint.is_Node.ln_Pri = 0;
    softint.is_Node.ln_Name = "SMP SoftInt Test";

    __sync_add_and_fetch(&data->sitd_ReadyCount, 1);
    Signal(data->sitd_MainTask, SIGF_READY);
    Wait(SIGF_GO);

    for (i = 0; i < data->sitd_Iters; i++)
    {
        while (*(volatile UBYTE *)&softint.is_Node.ln_Type == NT_SOFTINT)
            asm volatile("" ::: "memory");
        Cause(&softint);
        D(
            if (i % 1000 == 0)
                bug("[smpstress] SoftIntWorker '%s' iter %lu/%lu\n",
                    me->tc_Node.ln_Name, (unsigned long)i, (unsigned long)data->sitd_Iters);
        )
    }

    while (*(volatile UBYTE *)&softint.is_Node.ln_Type == NT_SOFTINT)
        asm volatile("" ::: "memory");

    __sync_add_and_fetch(&data->sitd_DoneCount, 1);
    Signal(data->sitd_MainTask, SIGF_DONE);
    Wait(0);
}

static BOOL TestSoftInts(ULONG iters)
{
    struct SoftIntTestData data;
    struct Task *workers[NUM_WORKERS];
    char names[NUM_WORKERS][32];
    ULONG expected;
    int i;

    bug("  Software interrupts (%u iters x %d tasks)...", (unsigned)iters, NUM_WORKERS);

    memset(&data, 0, sizeof(data));
    data.sitd_MainTask = FindTask(NULL);
    data.sitd_Iters = iters;
    g_SoftIntCounter = 0;

    for (i = 0; i < NUM_WORKERS; i++)
    {
        snprintf(names[i], sizeof(names[i]), "SoftIntWorker.%d", i);
        workers[i] = CreateWorkerTask(names[i], SoftIntWorker, i, 0, (IPTR)&data);
        if (!workers[i])
        {
            bug(" FAIL (can't create task %d)\n", i);
            Forbid();
            while (--i >= 0)
                RemTask(workers[i]);
            Permit();
            InvalidateWorkerMasks();
            return FALSE;
        }
    }

    if (!WaitAllSignals("softint-ready", SIGF_READY, &data.sitd_ReadyCount, NUM_WORKERS))
    {
        Forbid();
        for (i = 0; i < NUM_WORKERS; i++)
            if (workers[i])
                RemTask(workers[i]);
        Permit();
        InvalidateWorkerMasks();
        return FALSE;
    }

    Forbid();
    for (i = 0; i < NUM_WORKERS; i++)
        Signal(workers[i], SIGF_GO);
    Permit();

    if (!WaitAllSignals("softint-done", SIGF_DONE, &data.sitd_DoneCount, NUM_WORKERS))
    {
        Forbid();
        for (i = 0; i < NUM_WORKERS; i++)
            if (workers[i])
                RemTask(workers[i]);
        Permit();
        InvalidateWorkerMasks();
        return FALSE;
    }

    Forbid();
    for (i = 0; i < NUM_WORKERS; i++)
        RemTask(workers[i]);
    Permit();
    InvalidateWorkerMasks();

    expected = (ULONG)NUM_WORKERS * iters;
    if ((ULONG)g_SoftIntCounter != expected)
    {
        bug(" FAIL (counter=%ld, expected=%lu)\n",
               (long)g_SoftIntCounter, (unsigned long)expected);
        return FALSE;
    }
    bug(" OK\n");
    return TRUE;
}

/******************************************************************************/
/*  Test 5: AttemptSemaphore contention                                       */
/*                                                                            */
/*  Multiple tasks use AttemptSemaphore in a tight loop. On failure they      */
/*  yield and retry. Tests the try-lock path under contention.                */
/******************************************************************************/

struct AttemptTestData
{
    struct SignalSemaphore  atd_Sem;
    struct Task            *atd_MainTask;
    volatile LONG           atd_Counter;
    volatile LONG           atd_ReadyCount;
    volatile LONG           atd_DoneCount;
    ULONG                   atd_Iters;
};

static void AttemptWorker(void)
{
    struct Task *me = FindTask(NULL);
    struct AttemptTestData *data = (struct AttemptTestData *)me->tc_UserData;
    ULONG done = 0;

    __sync_add_and_fetch(&data->atd_ReadyCount, 1);
    Signal(data->atd_MainTask, SIGF_READY);
    Wait(SIGF_GO);

    while (done < data->atd_Iters)
    {
        if (AttemptSemaphore(&data->atd_Sem))
        {
            data->atd_Counter++;
            done++;
            ReleaseSemaphore(&data->atd_Sem);
            D(
                if (done % 1000 == 0)
                    bug("[smpstress] AttemptWorker '%s' iter %lu/%lu\n",
                        me->tc_Node.ln_Name, (unsigned long)done, (unsigned long)data->atd_Iters);
            )
        }
    }

    __sync_add_and_fetch(&data->atd_DoneCount, 1);
    Signal(data->atd_MainTask, SIGF_DONE);
    Wait(0);
}

static BOOL TestAttemptSemaphore(ULONG iters)
{
    struct AttemptTestData data;
    struct Task *workers[NUM_WORKERS];
    char names[NUM_WORKERS][32];
    ULONG expected;
    int i;

    bug("  AttemptSemaphore contention (%u iters x %d tasks)...", (unsigned)iters, NUM_WORKERS);

    memset(&data, 0, sizeof(data));
    InitSemaphore(&data.atd_Sem);
    data.atd_MainTask = FindTask(NULL);
    data.atd_Iters = iters;

    for (i = 0; i < NUM_WORKERS; i++)
    {
        snprintf(names[i], sizeof(names[i]), "AttemptWorker.%d", i);
        workers[i] = CreateWorkerTask(names[i], AttemptWorker, i, 0, (IPTR)&data);
        if (!workers[i])
        {
            bug(" FAIL (can't create task %d)\n", i);
            Forbid();
            while (--i >= 0)
                RemTask(workers[i]);
            Permit();
            InvalidateWorkerMasks();
            return FALSE;
        }
    }

    if (!WaitAllSignals("attempt-ready", SIGF_READY, &data.atd_ReadyCount, NUM_WORKERS))
    {
        Forbid();
        for (i = 0; i < NUM_WORKERS; i++)
            if (workers[i])
                RemTask(workers[i]);
        Permit();
        InvalidateWorkerMasks();
        return FALSE;
    }

    Forbid();
    for (i = 0; i < NUM_WORKERS; i++)
        Signal(workers[i], SIGF_GO);
    Permit();

    expected = (ULONG)NUM_WORKERS * iters;

    /* Monitor atd_Counter (advances every successful attempt), not
     * atd_DoneCount - same rationale as Test 1: no single worker may
     * finish all its iterations inside the stagnation window even
     * though the test as a whole makes steady progress. */
    if (!WaitAllSignals("attempt-done", SIGF_DONE, &data.atd_Counter, (int)expected))
    {
        Forbid();
        for (i = 0; i < NUM_WORKERS; i++)
            if (workers[i])
                RemTask(workers[i]);
        Permit();
        InvalidateWorkerMasks();
        return FALSE;
    }

    if (!WaitAllSignals("attempt-cleanup", SIGF_DONE, &data.atd_DoneCount, NUM_WORKERS))
    {
        Forbid();
        for (i = 0; i < NUM_WORKERS; i++)
            if (workers[i])
                RemTask(workers[i]);
        Permit();
        InvalidateWorkerMasks();
        return FALSE;
    }

    Forbid();
    for (i = 0; i < NUM_WORKERS; i++)
        RemTask(workers[i]);
    Permit();
    InvalidateWorkerMasks();

    if ((ULONG)data.atd_Counter != expected)
    {
        bug(" FAIL (counter=%ld, expected=%lu)\n",
               (long)data.atd_Counter, (unsigned long)expected);
        return FALSE;
    }
    bug(" OK\n");
    return TRUE;
}

/******************************************************************************/
/*  Main                                                                      */
/******************************************************************************/

int main(int argc, char **argv)
{
    int passed = 0, failed = 0;
    ULONG sem_iters = SEM_ITERS;
    ULONG pool_iters = POOL_ITERS;
    ULONG msg_iters = MSG_ITERS;
    ULONG softint_iters = SOFTINT_ITERS;

    KernelBase = OpenResource("kernel.resource");
    if (KernelBase)
        g_NumCPUs = KrnGetCPUCount();

    if (argc > 1 && strcmp(argv[1], "QUICK") == 0)
    {
        sem_iters /= 10;
        pool_iters /= 10;
        msg_iters /= 10;
        softint_iters /= 10;
    }

    bug("SMP Stress Test (%d workers, %d CPUs)\n", NUM_WORKERS, g_NumCPUs);
    bug("======================================\n\n");

    bug("Test 1: Semaphores\n");
    if (TestSemaphores(sem_iters))
        passed += 2;
    else
        failed += 2;

    bug("\nTest 2: Memory Pools\n");
    if (TestPools(pool_iters))
        passed++;
    else
        failed++;

    bug("\nTest 3: Message Passing\n");
    if (TestMessages(msg_iters))
        passed++;
    else
        failed++;

    bug("\nTest 4: Software Interrupts\n");
    if (TestSoftInts(softint_iters))
        passed++;
    else
        failed++;

    bug("\nTest 5: AttemptSemaphore\n");
    if (TestAttemptSemaphore(sem_iters))
        passed++;
    else
        failed++;

    bug("\n======================================\n");
    bug("Results: %d passed, %d failed\n", passed, failed);

    return failed ? RETURN_ERROR : RETURN_OK;
}
#else
int main(void)
{
    return RETURN_FAIL;
}
#endif