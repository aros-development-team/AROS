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
extern void Exec_CleanupETask(struct Task *, struct ExecBase *);

/*
 * Build the bootstrap task for core cpunum. Runs on the boot CPU before the
 * core is started (bcm27xx_init), so the core never allocates before it has
 * a task: if the allocator's semaphore were contended then, it would wait
 * on behalf of no task at all.
 */
struct Task *cpu_InitBootStrap(struct ExecBase *SysBase, cpuid_t cpunum)
{
    struct ExceptionContext *bsctx;
    struct MemList *ml;
#define bstask          ((struct Task *)(ml->ml_ME[0].me_Addr))
#define bstaskmlsize    (sizeof(struct MemList) + sizeof(struct MemEntry))

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

    /* cpu_Register makes library calls on this stack. */
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

    /* Its core only. Exec_InitETask gave it the creating CPU's mask, so
     * rewrite that one in place. */
    IntETask(bstask->tc_UnionETask.tc_ETask)->iet_CpuNumber = cpunum;
    KrnClearCPUMask(IntETask(bstask->tc_UnionETask.tc_ETask)->iet_CpuAffinity);
    KrnGetCPUMask(cpunum, IntETask(bstask->tc_UnionETask.tc_ETask)->iet_CpuAffinity);

    bsctx->fp = 0;
    bsctx->lr = (UQUAD)(IPTR)SysBase->TaskExitCode;

    return bstask;
#undef  bstask
}

/* Free a bootstrap task whose core was never started */
void cpu_DisposeBootStrap(struct Task *bstask, struct ExecBase *SysBase)
{
    struct MemList *mb, *mbnext;

    KrnDeleteContext(bstask->tc_UnionETask.tc_ETask->et_RegFrame);
    Exec_CleanupETask(bstask, SysBase);
    FreeVec(bstask->tc_Node.ln_Name);

    /* The task itself is in one of these MemLists */
    bstask->tc_MemEntry.lh_TailPred->ln_Succ = NULL;
    for (mb = (struct MemList *)bstask->tc_MemEntry.lh_Head; mb; mb = mbnext)
    {
        mbnext = (struct MemList *)mb->ml_Node.ln_Succ;
        FreeEntry(mb);
    }
}

void cpu_BootStrap(struct Task *bstask, struct ExecBase *SysBase)
{
    bstask->tc_State = TS_RUN;
    SET_THIS_TASK(bstask);
}
#endif
