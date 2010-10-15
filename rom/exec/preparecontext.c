/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PrepareContext() - Prepare a task context for dispatch.
    Lang: english
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <sigcore.h>
#include <utility/tagitem.h>
#include <proto/kernel.h>

#include "etask.h"
#include "exec_intern.h"
#include "exec_util.h"
#include "kernel_cpu.h"

#define _PUSH(sp, val) *--sp = (IPTR)val

/*****i***********************************************************************

    NAME */
	AROS_LH4(BOOL, PrepareContext,

/*  SYNOPSIS */
	AROS_LHA(VOLATILE struct Task *, task,       A0),
	AROS_LHA(APTR,                   entryPoint, A1),
	AROS_LHA(APTR,                   fallBack,   A2),
	AROS_LHA(struct TagItem *,       tagList,    A3),

/*  LOCATION */
	struct ExecBase *, SysBase, 6, Exec)

/*  FUNCTION
	Prepare the context (set of registers) for a new task.
	The context/stack will be set so that when the entryPoint
	function returns, the fallback function will be called.

    INPUTS
	task        	-   Pointer to task
	entryPoint      -   Function to call when the new context
			    comes alive.
	fallBack        -   Address of the function to be called
			    when the entryPoint function returns.
	tagList     	-   Additional options. Like for passing
	    	    	    arguments to the entryPoint() functions.

    RESULT
	TRUE on success. FALSE on failure.

    NOTES
	This function is very CPU dependant. In fact it can differ
	over different models of the same processor family.

    EXAMPLE

    BUGS

    SEE ALSO
	Dispatch()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    IPTR args[8] = {0};
    WORD numargs = 0;
    IPTR *sp = task->tc_SPReg;
    struct AROSCPUContext *ctx;

    if (!(task->tc_Flags & TF_ETASK) )
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
		
	    #define HANDLEARG(x) \
	    case TASKTAG_ARG ## x: \
	    	args[x - 1] = (IPTR)tagList->ti_Data; \
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
	
	if (tagList) tagList++;
    }
    
    /*
	There is not much to do here, or at least that is how it
	appears. Most of the work is done in the kernel_cpu.h macros.
    */

    if (numargs)
    {
    	#ifdef PREPARE_INITIAL_ARGS
	
	PREPARE_INITIAL_ARGS(sp, ctx, args, numargs);
	
	#else
	
	/* Assume C function gets all param on stack */
	
	while(numargs--)
	{
	    _PUSH(sp, args[numargs]);
	}
	
	#endif
    }

    #ifdef PREPARE_RETURN_ADDRESS
    
    PREPARE_RETURN_ADDRESS(ctx, fallBack);
    
    #else
    
    /* First we push the return address */
    _PUSH(sp, fallBack);
    
    #endif
    
    /* Then set up the frame to be used by Dispatch() */
    PREPARE_INITIAL_FRAME(ctx, sp, entryPoint);

    /* We return the new stack pointer back to the caller. */
    task->tc_SPReg = sp;
    return TRUE;

    AROS_LIBFUNC_EXIT
} /* PrepareContext() */
