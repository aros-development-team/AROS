/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PrepareContext() - Prepare a task context for dispatch, ARM version.
    Lang: english
*/

#include <exec/execbase.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <proto/arossupport.h>
#include <proto/kernel.h>
#include <aros/arm/cpucontext.h>

#include "exec_intern.h"
#include "exec_util.h"

BOOL PrepareContext(struct Task *task, APTR entryPoint, APTR fallBack,
                    const struct TagItem *tagList, struct ExecBase *SysBase)
{
    struct TagItem *t;
    struct ExceptionContext *ctx;
    ULONG args[4] = {0};
    int numargs = 0;
    STACKULONG *sp = task->tc_SPReg;

    if (!(task->tc_Flags & TF_ETASK) )
	return FALSE;

    ctx = KrnCreateContext();
    task->tc_UnionETask.tc_ETask->et_RegFrame = ctx;
    if (!ctx)
	return FALSE;

    /* Set up function arguments */
    while((t = LibNextTagItem(&tagList)))
    {
    	switch(t->ti_Tag)
	{
#define REGARG(x)			\
	case TASKTAG_ARG ## x:		\
	    ctx->r[x - 1] = t->ti_Data;	\
	    break;

#define STACKARG(x)			\
	case TASKTAG_ARG ## x:		\
	    args[x - 5] = t->ti_Data;	\
	    if (x - 4 > numargs)	\
		numargs = x - 4;	\
	    break;

	REGARG(1)
	REGARG(2)
	REGARG(3)
	REGARG(4)
	STACKARG(5)
	STACKARG(6)
	STACKARG(7)
	STACKARG(8)
	}
    }

    /* Last four arguments are put on stack */
    while (numargs > 0)
    	*--sp = args[--numargs];

    task->tc_SPReg = sp;

    /* Now prepare return address */
    ctx->r[11] = 0;
    ctx->lr = (ULONG)fallBack;

    ctx->Flags = 0;

    /* Then set up the frame to be used by Dispatch() */
    ctx->sp = (ULONG)task->tc_SPReg;
    ctx->pc = (ULONG)entryPoint;

    return TRUE;
} /* PrepareContext() */
