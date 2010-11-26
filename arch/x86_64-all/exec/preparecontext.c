/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
    $Id: preparecontext.c 34764 2010-10-15 15:04:08Z jmcmullan $

    Desc: PrepareContext() - Prepare a task context for dispatch, x86-64 version
    Lang: english
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <proto/kernel.h>
#include <aros/x86_64/cpucontext.h>

#include "etask.h"
#include "exec_intern.h"
#include "exec_util.h"

#define _PUSH(sp, val) *--sp = (IPTR)val

AROS_LH4(BOOL, PrepareContext,
	 AROS_LHA(VOLATILE struct Task *, task,       A0),
	 AROS_LHA(APTR,                   entryPoint, A1),
	 AROS_LHA(APTR,                   fallBack,   A2),
	 AROS_LHA(struct TagItem *,       tagList,    A3),
	 struct ExecBase *, SysBase, 6, Exec)
{
    AROS_LIBFUNC_INIT

    IPTR args[2];
    WORD numargs = 0;
    IPTR *sp = task->tc_SPReg;
    struct ExceptionContext *ctx;

    if (!(task->tc_Flags & TF_ETASK))
	return FALSE;

    ctx = KrnCreateContext();
    GetIntETask (task)->iet_Context = ctx;
    if (!ctx)
	return FALSE;

    while(tagList)
    {
    	switch(tagList->ti_Tag)
	{
	    case TAG_MORE:
	    	tagList = (struct TagItem *)tagList->ti_Data;
		continue;
		
	    case TAG_SKIP:
	    	tagList += tagList->ti_Data;
		break;
		
	    case TAG_DONE:
	    	tagList = NULL;
    	    	break;
		
#define REGARG(x, reg)				\
	    case TASKTAG_ARG ## x:		\
	    	ctx->reg = tagList->ti_Data;	\
		break;

#define STACKARG(x)				\
	    case TASKTAG_ARG ## x:		\
		args[x - 7] = tagList->ti_Data;	\
		if (x - 6 > numargs)		\
		    numargs = x - 6;		\
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

	if (tagList) tagList++;
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
