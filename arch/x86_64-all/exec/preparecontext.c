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

extern void TaskExitStub(void);

#define _PUSH(sp, val) *--sp = (IPTR)val

BOOL PrepareContext(struct Task *task, APTR entryPoint, APTR fallBack,
                    const struct TagItem *tagList, struct ExecBase *SysBase)
{
    IPTR args[2] = {0};
    IPTR *sp = task->tc_SPReg;
    struct TagItem *t, *tstate = (struct TagItem *)tagList;
    struct ExceptionContext *ctx;

    if (!(task->tc_Flags & TF_ETASK))
        return FALSE;

    ctx = KrnCreateContext();
    task->tc_UnionETask.tc_ETask->et_RegFrame = ctx;
    if (!ctx)
        return FALSE;

    while ((t = LibNextTagItem(&tstate)))
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

    /*
     * 64-bit ABI says that the stack must be 16-byte aligned *BEFORE*
     * calling a function. When a return address is pushed by call instruction,
     * the stack loses alignment, the callee knows about it and fixes it up.
     * Commonly it's fixed automatically by pushing old frame pointer.
     * A little of magic below ensures proper stack alignment for both entryPoint
     * and fallBack routines.
     */
    _PUSH(sp, 0);        /* De-align the stack (this will be RSP value when fallBack is called) */
    _PUSH(sp, fallBack); /* Push real exit code address for TaskExitStub */

    /*
     * Now stacked arguments for entry point. On x86-64 C function gets on stack only
     * last two arguments, and we always push them both even if they are empty.
     * This is needed for two reasons:
     * 1. Keep stack 16-byte-aligned (we can't push only one, for example).
     * 2. Simplify TaskExitStub, so that it always knows there are two arguments pushed.
     */
    _PUSH(sp, args[1]);
    _PUSH(sp, args[0]);

    /*
     * Now push the return address. Stack becomes de-aligned here, this will be RSP
     * value upon entering entryPoint.
     * TaskExitStub will remove two arguments pushed above and execute 'ret' instruction.
     * This way fallBack will be called with proper RSP value.
     */
    _PUSH(sp, TaskExitStub);

    /* Then set up the frame to be used by Dispatch() */
    ctx->rbp = 0;
    ctx->rip = (UQUAD)entryPoint;
    ctx->rsp = (UQUAD)sp;

    /* We return the new stack pointer back to the caller. */
    task->tc_SPReg = sp;

    return TRUE;
} /* PrepareContext() */
