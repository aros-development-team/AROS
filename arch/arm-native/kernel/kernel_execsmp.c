/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/kernel.h>
#include <proto/exec.h>

#include <stdio.h>

#include "exec_intern.h"

#include "etask.h"

#if defined(__AROSEXEC_SMP__)
extern BOOL Exec_InitETask(struct Task *, struct ExecBase *);
extern int Exec_ARMCPUInit(struct ExecBase *);

struct Task *cpu_InitBootStrap(struct ExecBase *SysBase)
{
    struct ExceptionContext *bsctx;
    struct MemList *ml;
    struct Task *bstask;
    uint32_t tmp;

    asm volatile (" mrc p15, 0, %0, c0, c0, 5 " : "=r" (tmp));

    if ((bstask = AllocMem(sizeof(struct Task),    MEMF_PUBLIC|MEMF_CLEAR)) == NULL)
    {
        bug("[KRN] Core %d FATAL : Failed to allocate task for bootstrap", (tmp & 0x3));
        return NULL;
    }

    if ((ml = AllocMem(sizeof(struct MemList), MEMF_PUBLIC|MEMF_CLEAR)) == NULL)
    {
        bug("[KRN] Core %d FATAL : Failed to allocate memory for bootstrap task", (tmp & 0x3));
        FreeMem(bstask, sizeof(struct Task));
        return NULL;
    }

    bug("[KRN] Core %d Bootstrap task @ 0x%p\n", (tmp & 0x3), bstask);

    if ((bsctx = KrnCreateContext()) == NULL)
    {
        bug("[KRN] Core %d FATAL : Failed to create the boostrap task context\n", (tmp & 0x3));
        FreeMem(ml, sizeof(struct MemList));
        FreeMem(bstask, sizeof(struct Task));
        return NULL;
    }

    bug("[KRN] Core %d cpu ctx @ 0x%p\n", (tmp & 0x3), bsctx);

    NEWLIST(&bstask->tc_MemEntry);

    if ((bstask->tc_Node.ln_Name = AllocVec(20, MEMF_CLEAR)) != NULL)
    {
        sprintf( bstask->tc_Node.ln_Name, "Core(%d) Bootstrap", (tmp & 0x3));
    }
    bstask->tc_Node.ln_Type = NT_TASK;
    bstask->tc_Node.ln_Pri  = 0;
    bstask->tc_State        = TS_RUN;
    bstask->tc_SigAlloc     = 0xFFFF;

    /* Build bootstraps memory list */
    ml->ml_NumEntries      = 1;
    ml->ml_ME[0].me_Addr   = bstask;
    ml->ml_ME[0].me_Length = sizeof(struct Task);
    AddHead(&bstask->tc_MemEntry, &ml->ml_Node);

    /* Create a ETask structure and attach CPU context */
    if (!Exec_InitETask(bstask, SysBase))
    {
        bug("[KRN] Core %d FATAL : Failed to initialize boostrap etask\n", (tmp & 0x3));
        FreeVec(bstask->tc_Node.ln_Name);
        FreeMem(ml, sizeof(struct MemList));
        FreeMem(bstask, sizeof(struct Task));
        return NULL;
    }
    bstask->tc_UnionETask.tc_ETask->et_RegFrame = bsctx;

    /* This Bootstrap task can run only on one of the available cores */
    IntETask(bstask->tc_UnionETask.tc_ETask)->iet_CpuNumber = (tmp & 0x3);
    IntETask(bstask->tc_UnionETask.tc_ETask)->iet_CpuAffinity = 1 << (tmp & 0x3);

    return bstask;
}

void cpu_BootStrap(struct Task *bstask, struct ExecBase *SysBase)
{
    KrnSpinLock(&PrivExecBase(SysBase)->TaskRunningSpinLock, SPINLOCK_MODE_WRITE);
    Enqueue(&PrivExecBase(SysBase)->TaskRunning, &bstask->tc_Node);
    KrnSpinUnLock(&PrivExecBase(SysBase)->TaskRunningSpinLock);

    Exec_ARMCPUInit(SysBase);
}
#endif
