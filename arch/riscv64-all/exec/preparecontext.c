/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: PrepareContext() - Prepare a task context for dispatch, 64bit RISC-V version.
*/

#include <exec/execbase.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <proto/arossupport.h>
#include <proto/kernel.h>
#include <aros/riscv64/cpucontext.h>
#include <asm/cpu.h>

#include "exec_intern.h"
#include "exec_util.h"
#if defined(__AROSEXEC_SMP__)
#include "etask.h"
#endif

BOOL PrepareContext(struct Task *task, APTR entryPoint, APTR fallBack,
                    const struct TagItem *tagList, struct ExecBase *SysBase)
{
    struct TagItem *t;
    struct ExceptionContext *ctx;

    if (!(task->tc_Flags & TF_ETASK) )
        return FALSE;

    ctx = KrnCreateContext();
    task->tc_UnionETask.tc_ETask->et_RegFrame = ctx;
    if (!ctx)
        return FALSE;

    /*
     * Set up function arguments. The LP64 ABI passes the first eight
     * integer arguments in a0-a7, so all of TASKTAG_ARG1..8 fit in
     * registers and none go to the stack.
     */
    while((t = LibNextTagItem((struct TagItem **)&tagList)))
    {
        switch(t->ti_Tag)
        {
#if defined(__AROSEXEC_SMP__)
            case TASKTAG_AFFINITY:
                IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuAffinity = t->ti_Data;
                break;
#endif
#define REGARG(argno)                       \
        case TASKTAG_ARG ## argno:          \
            ctx->x[REG_X_Ax_OFF + argno - 1] = t->ti_Data; \
            break;

        REGARG(1)
        REGARG(2)
        REGARG(3)
        REGARG(4)
        REGARG(5)
        REGARG(6)
        REGARG(7)
        REGARG(8)
#undef REGARG
        }
    }

    /* Prepare entry/exit values */
    ctx->fp = 0;
    ctx->ra = (IPTR)fallBack;
    ctx->Flags = 0;
    /* Return to S-mode with interrupts enabled and a fresh (Initial)
       FPU state - first FP use marks it Dirty for the lazy switcher */
    ctx->sr = SSTATUS_SPP | SSTATUS_SPIE | SSTATUS_FS_INITIAL;

    /* Set up the frame to be used by Dispatch() */
    ctx->sp = (IPTR)task->tc_SPReg;
    ctx->pc = (IPTR)entryPoint;

    return TRUE;
} /* PrepareContext() */
