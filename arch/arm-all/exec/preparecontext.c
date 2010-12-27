/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PrepareContext() - Prepare a task context for dispatch, ARM version.
    Lang: english
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <proto/kernel.h>
#include <aros/arm/cpucontext.h>

#include "etask.h"
#include "exec_intern.h"
#include "exec_util.h"

AROS_LH4(BOOL, PrepareContext,
	 AROS_LHA(VOLATILE struct Task *, task,       A0),
	 AROS_LHA(APTR,                   entryPoint, A1),
	 AROS_LHA(APTR,                   fallBack,   A2),
	 AROS_LHA(struct TagItem *,       tagList,    A3),
	 struct ExecBase *, SysBase, 6, Exec)
{
    AROS_LIBFUNC_INIT

    struct ExceptionContext *ctx;
    STACKULONG args[8] = {0};
    int numargs = 0;
    STACKULONG *sp = task->tc_SPReg;

    if (!(task->tc_Flags & TF_ETASK) )
	return FALSE;

    ctx = KrnCreateContext();
    GetIntETask (task)->iet_Context = ctx;
    if (!ctx)
	return FALSE;

    /* Set up function arguments */
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

#define HANDLEARG(x)				   \
	    case TASKTAG_ARG ## x:		   \
	        args[x-1] = tagList->ti_Data;      \
		if (numargs < x) numargs = x;	   \
	        break;
		
	    HANDLEARG(1)
	    HANDLEARG(2)
	    HANDLEARG(3)
	    HANDLEARG(4)
	    HANDLEARG(5)
	    HANDLEARG(6)
	    HANDLEARG(7)
	    HANDLEARG(8)
	}
	
	if (tagList) tagList++;
    }

    if (numargs)
    {
	switch (numargs)
	{
	    case 8:
		*--sp = args[7];
	    case 7:
		*--sp = args[6];
	    case 6:
		*--sp = args[5];
	    case 5:
		*--sp = args[4];
	    case 4:
		ctx->r[3] = args[3];
	    case 3:
		ctx->r[2] = args[2];
	    case 2:
		ctx->r[1] = args[1];
	    case 1:
		ctx->r[0] = args[0];
		break;
	}
    }

    task->tc_SPReg = sp;

    /* Now prepare return address */
    ctx->lr = (ULONG)fallBack;

    ctx->Flags = 0;

    /* Then set up the frame to be used by Dispatch() */
    ctx->sp = (ULONG)task->tc_SPReg;
    ctx->pc = (ULONG)entryPoint;

    return TRUE;

    AROS_LIBFUNC_EXIT
} /* PrepareContext() */
