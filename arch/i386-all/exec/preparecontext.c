/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PrepareContext() - Prepare a task context for dispatch, i386 version
    Lang: english
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <proto/alib.h>
#include <proto/kernel.h>
#include <aros/i386/cpucontext.h>

#include "exec_intern.h"
#include "exec_util.h"

#define _PUSH(sp, val) *--sp = (IPTR)val

BOOL PrepareContext(struct Task *task, APTR entryPoint, APTR fallBack,
                    struct TagItem *tagList, struct ExecBase *SysBase)
{
    IPTR args[8] = {0};
    WORD numargs = 0;
    IPTR *sp = task->tc_SPReg;
    struct TagItem *t;
    struct ExceptionContext *ctx;

    if (!(task->tc_Flags & TF_ETASK) )
	return FALSE;
  
    ctx = KrnCreateContext();
    task->tc_UnionETask.tc_ETask->et_RegFrame = ctx;
    if (!ctx)
	return FALSE;

    while((t = LibNextTagItem(&tagList)))
    {
    	switch(t->ti_Tag)
	{
		
#define HANDLEARG(x) \
	    case TASKTAG_ARG ## x: \
	    	args[x - 1] = t->ti_Data; \
		if (x > numargs) numargs = x; \
		break;

	    HANDLEARG(1)
	    HANDLEARG(2)
	    HANDLEARG(3)
	    HANDLEARG(4)
	    HANDLEARG(5)
	    HANDLEARG(6)
	    HANDLEARG(7)
	    HANDLEARG(8)
	    	
	    #undef HANDLEARG
	}
    }

    /*
	There is not much to do here, or at least that is how it
	appears. Most of the work is done in the kernel_cpu.h macros.
    */

    if (numargs)
    {
	/* On i386 C function gets all param on stack */
	while(numargs--)
	{
	    _PUSH(sp, args[numargs]);
	}
    }

    /* First we push the return address */
    _PUSH(sp, fallBack);

    /* Then set up the frame to be used by Dispatch() */
    ctx->ebp = 0;
    ctx->eip = (IPTR)entryPoint;
    ctx->esp = (IPTR)sp;

    /* We return the new stack pointer back to the caller. */
    task->tc_SPReg = sp;

    return TRUE;
} /* PrepareContext() */
