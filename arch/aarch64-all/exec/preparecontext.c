/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: PrepareContext() - Prepare a task context for dispatch, AArch64 version.
*/

#include <exec/execbase.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <proto/arossupport.h>
#include <proto/kernel.h>
#include <aros/aarch64/cpucontext.h>

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

    if (!(task->tc_Flags & TF_ETASK))
        return FALSE;

    ctx = KrnCreateContext();
    task->tc_UnionETask.tc_ETask->et_RegFrame = ctx;
    if (!ctx)
        return FALSE;

    /*
     * Set up function arguments. AAPCS64 passes the first eight integer
     * arguments in x0-x7, so all of TASKTAG_ARG1..8 fit in registers and no
     * arguments ever need to go on the stack (unlike 32-bit ARM).
     */
    while ((t = LibNextTagItem((struct TagItem **)&tagList)))
    {
        switch (t->ti_Tag)
        {
#if defined(__AROSEXEC_SMP__)
        case TASKTAG_AFFINITY:
        {
            struct IntETask *iet = IntETask(task->tc_UnionETask.tc_ETask);
            void *aff = (void *)t->ti_Data;

            /*
             * NULL is not a valid mask. The value 0 is also
             * TASKAFFINITY_BOOT, so honour that meaning with a real mask
             * for the boot CPU rather than storing NULL (which no CPU
             * would dispatch). If even that can not be had, keep the
             * mask InitETask gave the task.
             */
            if ((IPTR)aff == TASKAFFINITY_BOOT)
            {
                if (!(aff = KrnAllocCPUMask()))
                    break;
                KrnGetCPUMask(0, aff);
            }

            /* The caller's mask replaces the one InitETask() allocated */
            if (iet->iet_CpuAffinity && (IPTR)iet->iet_CpuAffinity != TASKAFFINITY_ANY)
                KrnFreeCPUMask(iet->iet_CpuAffinity);
            iet->iet_CpuAffinity = aff;
            break;
        }
#endif
#define REGARG(n)                               \
        case TASKTAG_ARG ## n:                  \
            ctx->x[(n) - 1] = (UQUAD)t->ti_Data;\
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

    /* NULL frame pointer (x29) ends backtraces; x30 is the fall-back return. */
    ctx->fp = 0;
    ctx->lr = (UQUAD)(IPTR)fallBack;

    ctx->Flags = 0;

    /* The frame Dispatch() will load. SP must be 16-byte aligned (AAPCS64). */
    ctx->sp = (UQUAD)(IPTR)task->tc_SPReg & ~(UQUAD)15;
    ctx->pc = (UQUAD)(IPTR)entryPoint;
    ctx->cpsr = 0;

    return TRUE;
} /* PrepareContext() */
