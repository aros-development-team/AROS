/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PrepareContext() - Prepare a task context for dispatch, x86-64 version
    Lang: english
*/

#include <exec/execbase.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <proto/kernel.h>
#include <aros/x86_64/cpucontext.h>

#include "exec_intern.h"
#include "exec_util.h"

#define _PUSH(sp, val) *--sp = (IPTR)val

AROS_LH4(BOOL, PrepareContext,
         AROS_LHA(VOLATILE struct Task *, task,       A0),
         AROS_LHA(APTR,                   entryPoint, A1),
         AROS_LHA(APTR,                   fallBack,   A2),
         AROS_LHA(const struct TagItem *, tagList,    A3),
         struct ExecBase *, SysBase, 6, Exec)
{
    AROS_LIBFUNC_INIT

    IPTR args[2] = {0};
    WORD numargs = 0;
    IPTR *sp = task->tc_SPReg;
    struct TagItem *t;
    struct ExceptionContext *ctx;

    if (!(task->tc_Flags & TF_ETASK))
        return FALSE;

    ctx = KrnCreateContext();
    task->tc_UnionETask.tc_ETask->et_RegFrame = ctx;
    if (!ctx)
        return FALSE;

    while ((t = LibNextTagItem(&tagList)))
    {
        switch(t->ti_Tag)
        {

#define REGARG(x, reg)                \
        case TASKTAG_ARG ## x:        \
            ctx->reg = t->ti_Data;    \
            break;

#define STACKARG(x)                   \
        case TASKTAG_ARG ## x:        \
            args[x - 7] = t->ti_Data; \
            if (x - 6 > numargs)      \
                numargs = x - 6;      \
            break;

        REGARG(1, rdi)
        REGARG(2, rsi)
        REGARG(3, rdx)
        REGARG(4, rcx)
        REGARG(5, r8)
        REGARG(6, r9)
        STACKARG(7)
        STACKARG(8)
        }
    }

    /* On x86-64 C function gets on stack only last two arguments */
    while (numargs > 0)
        _PUSH(sp, args[--numargs]);

    /* First we push the return address */
    _PUSH(sp, fallBack);
        
    /* Then set up the frame to be used by Dispatch() */
    ctx->rbp = 0;
    ctx->rip = (UQUAD)entryPoint;
    ctx->rsp = (UQUAD)sp;

    /* We return the new stack pointer back to the caller. */
    task->tc_SPReg = sp;

    return TRUE;

    AROS_LIBFUNC_EXIT
} /* PrepareContext() */
