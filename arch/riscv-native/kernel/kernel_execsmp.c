/*
    Copyright (C) 2015-2026, The AROS Development Team. All rights reserved.

    Desc: The bootstrap task each secondary hart starts scheduling with
          (from aarch64-native).
*/

#include <aros/config.h>
#include <aros/kernel.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/tasks.h>

#include <aros/riscv64/cpucontext.h>

#define __KERNEL_NOLIBBASE__
#include <proto/kernel.h>
#include <proto/exec.h>

#include <stdio.h>

#include <kernel_base.h>
#include <kernel_debug.h>

#include "etask.h"
#define __AROS_KERNEL__
#include "exec_intern.h"
#include "kernel_cpu.h"
#include "kernel_riscv.h"

#undef D
#define D(x)

#if defined(__AROSEXEC_SMP__)
extern BOOL Exec_InitETask(struct Task *, struct Task *, struct ExecBase *);

struct Task *cpu_InitBootStrap(struct ExecBase *SysBase)
{
    struct ExceptionContext *bsctx;
    struct MemList *ml;
#define bstask          ((struct Task *)(ml->ml_ME[0].me_Addr))
#define bstaskmlsize    (sizeof(struct MemList) + 2 * sizeof(struct MemEntry))
    uint32_t cpunum = krnRVCPUNumber();

    /* Build bootstraps memory list */
    if ((ml = AllocMem(bstaskmlsize, MEMF_PUBLIC|MEMF_CLEAR)) == NULL)
    {
        bug("[Kernel:%02d] FATAL : Failed to allocate memory for bootstrap task", cpunum);
        return NULL;
    }

    ml->ml_NumEntries      = 3;

    ml->ml_ME[0].me_Length = sizeof(struct Task);
    if ((ml->ml_ME[0].me_Addr = AllocMem(sizeof(struct Task),    MEMF_PUBLIC|MEMF_CLEAR)) == NULL)
    {
        bug("[Kernel:%02d] FATAL : Failed to allocate task for bootstrap", cpunum);
        FreeMem(ml, bstaskmlsize);
        return NULL;
    }

    /* cpu_Register and the task's own code (cpu_BootStrapEntry) make
     * library calls on this stack. */
#define BOOTSTRAP_STACKWORDS 1024
    ml->ml_ME[1].me_Length = 15 + (sizeof(IPTR) * BOOTSTRAP_STACKWORDS);
    if ((ml->ml_ME[1].me_Addr = AllocMem(ml->ml_ME[1].me_Length, MEMF_PUBLIC|MEMF_CLEAR)) == NULL)
    {
        bug("[Kernel:%02d] FATAL : Failed to allocate stack for bootstrap task", cpunum);
        FreeMem(ml->ml_ME[0].me_Addr, ml->ml_ME[0].me_Length);
        FreeMem(ml, bstaskmlsize);
        return NULL;
    }
    bstask->tc_SPLower = (APTR)(((uintptr_t)ml->ml_ME[1].me_Addr + 15) & ~0xF);
    bstask->tc_SPUpper = bstask->tc_SPLower + (sizeof(IPTR) * BOOTSTRAP_STACKWORDS);

    /* The list must exist before anything goes on it (the task is
     * fresh MEMF_CLEAR memory, so lh_Head is NULL until then) */
    NEWLIST(&bstask->tc_MemEntry);
    AddHead(&bstask->tc_MemEntry, &ml->ml_Node);

    D(bug("[Kernel:%02d] Bootstrap task @ 0x%p\n", cpunum, bstask));

    if ((bsctx = KrnCreateContext()) == NULL)
    {
        bug("[Kernel:%02d] FATAL : Failed to create the bootstrap Task context\n", cpunum);
        FreeMem(ml->ml_ME[1].me_Addr, ml->ml_ME[1].me_Length);
        FreeMem(ml->ml_ME[0].me_Addr, ml->ml_ME[0].me_Length);
        FreeMem(ml, bstaskmlsize);
        return NULL;
    }

    D(bug("[Kernel:%02d] CPU Ctx @ 0x%p\n", cpunum, bsctx));


    ml->ml_ME[2].me_Length = 20;
    if ((ml->ml_ME[2].me_Addr = AllocMem(20, MEMF_PUBLIC|MEMF_CLEAR)) != NULL)
    {
        bstask->tc_Node.ln_Name = ml->ml_ME[2].me_Addr;
        sprintf(bstask->tc_Node.ln_Name, "CPU #%02d Bootstrap", cpunum);
    }
    else
        ml->ml_ME[2].me_Length = 0;
    bstask->tc_Node.ln_Type = NT_TASK;
    bstask->tc_Node.ln_Pri  = 0;
    bstask->tc_State        = TS_READY;
    bstask->tc_SigAlloc     = 0xFFFF;

    /* Create a ETask structure and attach CPU context */
    if (!Exec_InitETask(bstask, NULL, SysBase))
    {
        bug("[Kernel:%02d] FATAL : Failed to initialize bootstrap ETask\n", cpunum);
        if (ml->ml_ME[2].me_Addr)
            FreeMem(ml->ml_ME[2].me_Addr, ml->ml_ME[2].me_Length);
        FreeMem(ml->ml_ME[1].me_Addr, ml->ml_ME[1].me_Length);
        FreeMem(ml->ml_ME[0].me_Addr, ml->ml_ME[0].me_Length);
        FreeMem(ml, bstaskmlsize);
        return NULL;
    }
    bstask->tc_UnionETask.tc_ETask->et_RegFrame = bsctx;

    /* This CPU only. iet_CpuAffinity is a mask buffer, not a bitmask. */
    IntETask(bstask->tc_UnionETask.tc_ETask)->iet_CpuNumber = cpunum;
    {
        void *aff = KrnAllocCPUMask();

        /* A NULL mask is invalid: without one this task can not be */
        if (!aff)
        {
            bug("[Kernel:%02d] FATAL : Failed to allocate the bootstrap affinity mask\n", cpunum);
            return NULL;
        }
        KrnGetCPUMask(cpunum, aff);
        IntETask(bstask->tc_UnionETask.tc_ETask)->iet_CpuAffinity = aff;
    }

    bsctx->x[REG_X_FP_OFF] = 0;
    bsctx->ra = (IPTR)SysBase->TaskExitCode;

    return bstask;
#undef  bstask
}

void cpu_BootStrap(struct Task *bstask, struct ExecBase *SysBase)
{
    bstask->tc_State = TS_RUN;
    SET_THIS_TASK(bstask);
}
#endif
