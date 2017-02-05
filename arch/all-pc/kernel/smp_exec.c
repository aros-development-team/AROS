/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/config.h>

#if defined(__AROSEXEC_SMP__)
#include <proto/exec.h>
#define __KERNEL_NOLIBBASE__
#include <proto/kernel.h>

#include <exec/rawfmt.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_intern.h"
#include "apic.h"

#define __AROS_KERNEL__

#include "exec_intern.h"

#include "etask.h"

extern BOOL Exec_InitETask(struct Task *, struct Task *, struct ExecBase *);

struct Task *cpu_InitBootStrap(apicid_t cpuNo)
{
    struct ExceptionContext *bsctx;
    struct MemList *ml;
#define bstask          ((struct Task *)(ml->ml_ME[0].me_Addr))
#define bstaskmlsize    (sizeof(struct MemList) + sizeof(struct MemEntry))

    /* Build bootstraps memory list */
    if ((ml = AllocMem(bstaskmlsize, MEMF_PUBLIC|MEMF_CLEAR)) == NULL)
    {
        bug("[Kernel:%03u] FATAL : Failed to allocate memory for bootstrap task", cpuNo);
        return NULL;
    }

    ml->ml_NumEntries      = 2;

    ml->ml_ME[0].me_Length = sizeof(struct Task);
    if ((ml->ml_ME[0].me_Addr = AllocMem(sizeof(struct Task),    MEMF_PUBLIC|MEMF_CLEAR)) == NULL)
    {
        bug("[Kernel:%03u] FATAL : Failed to allocate task for bootstrap", cpuNo);
        FreeMem(ml, bstaskmlsize);
        return NULL;
    }

    /* allocate some stack space for user mode .. */
    ml->ml_ME[1].me_Length = 15 + (sizeof(IPTR) * 128);
    if ((ml->ml_ME[1].me_Addr = AllocMem(ml->ml_ME[1].me_Length, MEMF_PUBLIC|MEMF_CLEAR)) == NULL)
    {
        bug("[Kernel:%03u] FATAL : Failed to allocate stack for bootstrap task", cpuNo);
        FreeMem(ml->ml_ME[0].me_Addr, ml->ml_ME[0].me_Length);
        FreeMem(ml, bstaskmlsize);
        return NULL;
    }
    bstask->tc_SPLower = (APTR)(((IPTR)ml->ml_ME[1].me_Addr + 15) & ~0xF);
    bstask->tc_SPUpper = bstask->tc_SPLower + (sizeof(IPTR) * 128);

    AddHead(&bstask->tc_MemEntry, &ml->ml_Node);

    D(bug("[Kernel:%03u] Bootstrap task @ 0x%p\n", cpuNo, bstask));

    if ((bsctx = KrnCreateContext()) == NULL)
    {
        bug("[Kernel:%03u] FATAL : Failed to create the bootstrap Task context\n", cpuNo);
        FreeMem(ml->ml_ME[1].me_Addr, ml->ml_ME[1].me_Length);
        FreeMem(ml->ml_ME[0].me_Addr, ml->ml_ME[0].me_Length);
        FreeMem(ml, bstaskmlsize);
        return NULL;
    }

    D(bug("[Kernel:%03u] CPU Ctx @ 0x%p\n", cpuNo, bsctx));

    NEWLIST(&bstask->tc_MemEntry);

    if ((bstask->tc_Node.ln_Name = AllocVec(20, MEMF_CLEAR)) != NULL)
    {
        IPTR bstNameArg[] = 
        {
            cpuNo
        };
        RawDoFmt("CPU #%03u Bootstrap", (RAWARG)bstNameArg, RAWFMTFUNC_STRING, bstask->tc_Node.ln_Name);
    }
    bstask->tc_Node.ln_Type = NT_TASK;
    bstask->tc_Node.ln_Pri  = 0;
    bstask->tc_State        = TS_READY;
    bstask->tc_SigAlloc     = 0xFFFF;

    /* Create a ETask structure and attach CPU context */
    if (!Exec_InitETask(bstask, NULL, SysBase))
    {
        bug("[Kernel:%03u] FATAL : Failed to initialize bootstrap ETask\n", cpuNo);
        FreeVec(bstask->tc_Node.ln_Name);
        FreeMem(ml->ml_ME[1].me_Addr, ml->ml_ME[1].me_Length);
        FreeMem(ml->ml_ME[0].me_Addr, ml->ml_ME[0].me_Length);
        FreeMem(ml, bstaskmlsize);
        return NULL;
    }
    bstask->tc_UnionETask.tc_ETask->et_RegFrame = bsctx;

    /* the bootstrap can only run on this CPU */
    IntETask(bstask->tc_UnionETask.tc_ETask)->iet_CpuNumber = cpuNo;
    IntETask(bstask->tc_UnionETask.tc_ETask)->iet_CpuAffinity = KrnGetCPUMask(cpuNo);

#if (0)
    //TODO: set tasks SysBase->TaskExitCode
#endif
    bsctx->Flags = 0;

    return bstask;
#undef  bstask
}

void cpu_BootStrap(struct Task *bstask)
{
    D(
        apicid_t cpuNo = KrnGetCPUNumber();
    
        bug("[Kernel:SMP] %s[%03u]()\n", __func__, cpuNo);
        
        if (IntETask(bstask->tc_UnionETask.tc_ETask)->iet_CpuNumber != cpuNo)
            bug("[Kernel:SMP] %s[%03u]: bstask running on wrong CPU? (task cpu = %03u)\n", __func__, cpuNo, IntETask(bstask->tc_UnionETask.tc_ETask)->iet_CpuNumber);
    )

    bstask->tc_State = TS_RUN;
    SET_THIS_TASK(bstask);

    D(bug("[Kernel:SMP] %s[%03u]: Leaving supervisor mode\n", __func__, cpuNo));

    krnLeaveSupervisorRing(FLAGS_INTENABLED);

    D(bug("[Kernel:SMP] %s[%03u]: Done\n", __func__, cpuNo));
}
#endif
