/*
    Copyright (C) 2015-2026, The AROS Development Team. All rights reserved.
*/

#include <proto/kernel.h>
#include <proto/exec.h>

#include <stdio.h>

#include "etask.h"
#include "exec_intern.h"

#include "kernel_cpu.h"

#undef D
#define D(x)

#if defined(__AROSEXEC_SMP__)
extern BOOL Exec_InitETask(struct Task *, struct Task *, struct ExecBase *);

struct Task *cpu_InitBootStrap(struct ExecBase *SysBase)
{
    struct ExceptionContext *bsctx;
    struct MemList *ml;
#define bstask          ((struct Task *)(ml->ml_ME[0].me_Addr))
#define bstaskmlsize    (sizeof(struct MemList) + sizeof(struct MemEntry))
    cpuid_t cpunum = GetCPUNumber();

    /* Build bootstraps memory list */
    if ((ml = AllocMem(bstaskmlsize, MEMF_PUBLIC|MEMF_CLEAR)) == NULL)
    {
        bug("[Kernel:%02d] FATAL : Failed to allocate memory for bootstrap task", cpunum);
        return NULL;
    }

    ml->ml_NumEntries      = 2;

    ml->ml_ME[0].me_Length = sizeof(struct Task);
    if ((ml->ml_ME[0].me_Addr = AllocMem(sizeof(struct Task),    MEMF_PUBLIC|MEMF_CLEAR)) == NULL)
    {
        bug("[Kernel:%02d] FATAL : Failed to allocate task for bootstrap", cpunum);
        FreeMem(ml, bstaskmlsize);
        return NULL;
    }

    /* cpu_Register makes library calls on this stack and it stays the
     * core's context until the first real dispatch. */
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

    NEWLIST(&bstask->tc_MemEntry);

    if ((bstask->tc_Node.ln_Name = AllocVec(20, MEMF_CLEAR)) != NULL)
    {
        sprintf(bstask->tc_Node.ln_Name, "CPU #%02d Bootstrap", cpunum);
    }
    bstask->tc_Node.ln_Type = NT_TASK;
    /* Below the per-core idle task at -127: this only provides a context
     * during bring-up and must yield to everything. */
    bstask->tc_Node.ln_Pri  = -128;
    bstask->tc_State        = TS_READY;
    bstask->tc_SigAlloc     = 0xFFFF;

    /* Create a ETask structure and attach CPU context */
    if (!Exec_InitETask(bstask, NULL, SysBase))
    {
        bug("[Kernel:%02d] FATAL : Failed to initialize bootstrap ETask\n", cpunum);
        FreeVec(bstask->tc_Node.ln_Name);
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
        if (aff)
            KrnGetCPUMask(cpunum, aff);
        IntETask(bstask->tc_UnionETask.tc_ETask)->iet_CpuAffinity = aff;
    }

    bsctx->fp = 0;
    bsctx->lr = (UQUAD)(IPTR)SysBase->TaskExitCode;

    return bstask;
#undef  bstask
}

void cpu_BootStrap(struct Task *bstask, struct ExecBase *SysBase)
{
    bstask->tc_State = TS_RUN;
    SET_THIS_TASK(bstask);
}
#endif
