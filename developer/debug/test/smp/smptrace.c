/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: SMP stress test for exec primitives.
          Workers on each CPU hammer semaphores and memory allocation
          concurrently.  Logs progress so hangs are easy to localize.
*/

#include <exec/memory.h>
#include <exec/semaphores.h>
#include <exec/tasks.h>
#include <utility/tagitem.h>
#include <dos/dos.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/kernel.h>

#include <stdio.h>
#include <string.h>

#define DEBUG 1
#include <aros/debug.h>

APTR KernelBase;

#define NUM_WORKERS 4
#define STACKSIZE   16384
#define SEM_ITERS   500
#define ALLOC_ITERS 200

enum
{
    STAGE_INIT = 0,
    STAGE_READY,
    STAGE_GO,
    STAGE_SEM,
    STAGE_ALLOC,
    STAGE_DONE
};

struct TraceData
{
    struct SignalSemaphore td_Sem;
    volatile LONG          td_Counter;
    volatile LONG          td_ReadyCount;
    volatile LONG          td_DoneCount;
    volatile LONG          td_GoFlag;
    volatile ULONG         td_Stage[NUM_WORKERS];
    volatile ULONG         td_CPUReady[NUM_WORKERS];
    volatile ULONG         td_CPUGo[NUM_WORKERS];
    volatile ULONG         td_CPUDone[NUM_WORKERS];
    volatile ULONG         td_AllocCount[NUM_WORKERS];
};

struct TraceWorkerCtx
{
    struct TraceData *twc_Data;
    ULONG             twc_WorkerID;
    cpumask_t        *twc_Affinity;
};

static void TraceWorker(void)
{
    struct Task *me = FindTask(NULL);
    struct TraceWorkerCtx *ctx = (struct TraceWorkerCtx *)me->tc_UserData;
    struct TraceData *data = ctx->twc_Data;
    ULONG worker_id = ctx->twc_WorkerID;
    ULONG i;

    data->td_CPUReady[worker_id] = KrnGetCPUNumber();
    data->td_Stage[worker_id] = STAGE_READY;
    __sync_add_and_fetch(&data->td_ReadyCount, 1);

    while (!data->td_GoFlag)
        Delay(1);

    data->td_CPUGo[worker_id] = KrnGetCPUNumber();
    data->td_Stage[worker_id] = STAGE_GO;

    data->td_Stage[worker_id] = STAGE_SEM;
    for (i = 0; i < SEM_ITERS; i++)
    {
        ObtainSemaphore(&data->td_Sem);
        data->td_Counter++;
        ReleaseSemaphore(&data->td_Sem);
    }

    data->td_Stage[worker_id] = STAGE_ALLOC;
    for (i = 0; i < ALLOC_ITERS; i++)
    {
        APTR mem = AllocVec(128 + (i * 16), MEMF_PUBLIC | MEMF_CLEAR);

        if (mem != NULL)
        {
            data->td_AllocCount[worker_id]++;
            FreeVec(mem);
        }
    }

    data->td_CPUDone[worker_id] = KrnGetCPUNumber();
    data->td_Stage[worker_id] = STAGE_DONE;
    __sync_add_and_fetch(&data->td_DoneCount, 1);
    Wait(0);
}

int main(void)
{
    struct TraceData data;
    struct TraceWorkerCtx worker_ctx[NUM_WORKERS];
    struct Task *workers[NUM_WORKERS];
    char names[NUM_WORKERS][32];
    ULONG expected_counter;
    int i, ncpus;

    KernelBase = OpenResource("kernel.resource");
    ncpus = KernelBase ? KrnGetCPUCount() : 1;

    bug("[smptrace] start: workers=%ld cpus=%ld sem_iters=%ld alloc_iters=%ld\n",
           (LONG)NUM_WORKERS, (LONG)ncpus, (LONG)SEM_ITERS, (LONG)ALLOC_ITERS);

    memset(&data, 0, sizeof(data));
    InitSemaphore(&data.td_Sem);

    for (i = 0; i < NUM_WORKERS; i++)
    {
        snprintf(names[i], sizeof(names[i]), "smptrace.%d", i);
        worker_ctx[i].twc_Data = &data;
        worker_ctx[i].twc_WorkerID = (ULONG)i;
        worker_ctx[i].twc_Affinity = NULL;
        if (KernelBase)
        {
            worker_ctx[i].twc_Affinity = KrnAllocCPUMask();
            if (worker_ctx[i].twc_Affinity)
                KrnGetCPUMask(i % ncpus, worker_ctx[i].twc_Affinity);
        }
        bug("[smptrace] creating %s cpu=%ld affinity=%08lx\n",
               (IPTR)names[i], (LONG)(i % ncpus),
               worker_ctx[i].twc_Affinity ? *worker_ctx[i].twc_Affinity : 0);
        workers[i] = NewCreateTask(TASKTAG_NAME, (IPTR)names[i],
                                   TASKTAG_PC, (IPTR)TraceWorker,
                                   TASKTAG_STACKSIZE, STACKSIZE,
                                   TASKTAG_USERDATA, (IPTR)&worker_ctx[i],
                                   TASKTAG_AFFINITY, (IPTR)worker_ctx[i].twc_Affinity,
                                   TAG_DONE);
        if (workers[i] == NULL)
        {
            bug("[smptrace] FAIL: create %s\n", (IPTR)names[i]);
            /* Reap the ones already created - they poll td_GoFlag on THIS
             * stack frame and would spin on a dead frame after we return. */
            Forbid();
            while (--i >= 0)
                RemTask(workers[i]);
            Permit();
            return RETURN_FAIL;
        }
    }

    bug("[smptrace] waiting for workers to be ready...\n");
    while (data.td_ReadyCount < NUM_WORKERS)
    {
        Delay(1);
    }
    bug("[smptrace] all ready: cpus=%lu,%lu,%lu,%lu\n",
           data.td_CPUReady[0], data.td_CPUReady[1],
           data.td_CPUReady[2], data.td_CPUReady[3]);

    bug("[smptrace] GO!\n");
    data.td_GoFlag = 1;

    while (data.td_DoneCount < NUM_WORKERS)
    {
        ULONG done_mask = 0;

        Delay(1);
        for (i = 0; i < NUM_WORKERS; i++)
        {
            if (data.td_Stage[i] >= STAGE_DONE)
                done_mask |= (1UL << i);
        }
        bug("[smptrace] progress: done=%02lx stages=%lu,%lu,%lu,%lu\n",
               done_mask,
               data.td_Stage[0], data.td_Stage[1],
               data.td_Stage[2], data.td_Stage[3]);
    }

    expected_counter = NUM_WORKERS * SEM_ITERS;
    bug("[smptrace] RESULT: counter=%ld expected=%lu %s\n",
           (LONG)data.td_Counter, expected_counter,
           (IPTR)(data.td_Counter == (LONG)expected_counter ? "OK" : "MISMATCH"));
    bug("[smptrace] cpus: ready=%lu,%lu,%lu,%lu go=%lu,%lu,%lu,%lu done=%lu,%lu,%lu,%lu\n",
           data.td_CPUReady[0], data.td_CPUReady[1],
           data.td_CPUReady[2], data.td_CPUReady[3],
           data.td_CPUGo[0], data.td_CPUGo[1],
           data.td_CPUGo[2], data.td_CPUGo[3],
           data.td_CPUDone[0], data.td_CPUDone[1],
           data.td_CPUDone[2], data.td_CPUDone[3]);
    bug("[smptrace] allocs: %lu,%lu,%lu,%lu\n",
           data.td_AllocCount[0], data.td_AllocCount[1],
           data.td_AllocCount[2], data.td_AllocCount[3]);

    Forbid();
    for (i = 0; i < NUM_WORKERS; i++)
        RemTask(workers[i]);
    Permit();

    bug("[smptrace] exit\n");
    return RETURN_OK;
}
