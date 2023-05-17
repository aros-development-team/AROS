/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.

    Desc: PrepareContext() - Prepare a task context for dispatch, RISC-V version.
*/

#include <exec/execbase.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <proto/arossupport.h>
#include <proto/kernel.h>
#include <aros/riscv/cpucontext.h>

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
    ULONG args[4] = {0};
    int numargs = 0;
    STACKULONG *ctxsp = task->tc_SPReg;

    if (!(task->tc_Flags & TF_ETASK) )
        return FALSE;

    ctx = KrnCreateContext();
    task->tc_UnionETask.tc_ETask->et_RegFrame = ctx;
    if (!ctx)
        return FALSE;

    /* Set up function arguments */
    while((t = LibNextTagItem((struct TagItem **)&tagList)))
    {
        switch(t->ti_Tag)
        {
#if defined(__AROSEXEC_SMP__)
            case TASKTAG_AFFINITY:
                IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuAffinity = t->ti_Data;
                break;
#endif
// Function args are passed in x10 -  x17 (A0-A7) on riscv
#define REGARG(argno)                       \
        case TASKTAG_ARG ## argno:          \
            ctx->x[REG_X_Ax_OFF + argno - 1] = t->ti_Data; \
            break;

#define STACKARG(argno)                     \
        case TASKTAG_ARG ## argno:          \
            args[argno - (RISCV_FUNCREG_CNT + 1)] = t->ti_Data;   \
            if (argno - RISCV_FUNCREG_CNT > numargs)        \
                numargs = argno - RISCV_FUNCREG_CNT;        \
            break;

#if (RISCV_FUNCREG_CNT > 0)
        REGARG(1)
#else
        STACKARG(1)
#endif
#if (RISCV_FUNCREG_CNT > 1)
        REGARG(2)
#else
        STACKARG(2)
#endif
#if (RISCV_FUNCREG_CNT > 2)
        REGARG(3)
#else
        STACKARG(3)
#endif
#if (RISCV_FUNCREG_CNT > 3)
        REGARG(4)
#else
        STACKARG(4)
#endif
#if (RISCV_FUNCREG_CNT > 4)
        REGARG(5)
#else
        STACKARG(5)
#endif
#if (RISCV_FUNCREG_CNT >5)
        REGARG(6)
#else
        STACKARG(6)
#endif
        STACKARG(7)
        STACKARG(8)
        }
    }

    /* Remaining arguments are put onto the stack */
    while (numargs > 0)
        *--ctxsp = args[--numargs];

    /* Prepare entry/exit values */
    task->tc_SPReg = ctxsp;
    ctx->fp = 0;
    ctx->ra = (ULONG)fallBack;
    ctx->Flags = 0;

    /* Set up the frame to be used by Dispatch() */
    ctx->sp = (ULONG)task->tc_SPReg;
    ctx->pc = (ULONG)entryPoint;

    return TRUE;
} /* PrepareContext() */
