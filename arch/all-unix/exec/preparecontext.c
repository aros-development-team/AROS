/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: i386unix version of PrepareContext().
    Lang: english
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <sigcore.h>
#include "etask.h"
#include "exec_util.h"

#include <aros/libcall.h>

AROS_LH4(BOOL, PrepareContext,
    AROS_LHA(struct Task *, task, A0),
    AROS_LHA(APTR, entryPoint, A1),
    AROS_LHA(APTR, fallBack, A2),
    AROS_LHA(struct TagItem *, tagList, A3),
    struct ExecBase *, SysBase, 6, Exec)
{
    AROS_LIBFUNC_INIT

    IPTR args[8] = {0};
    WORD numargs = 0;
    
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
	appears. Most of the work is done in the sigcore.h macros.
    */

    if (!(task->tc_Flags & TF_ETASK) )
	return FALSE;

    GetIntETask (task)->iet_Context = AllocTaskMem (task
	, SIZEOF_ALL_REGISTERS
	, MEMF_PUBLIC|MEMF_CLEAR
    );

    if (!GetIntETask (task)->iet_Context)
	return FALSE;

    if (numargs)
    {
    	#ifdef PREPARE_INITIAL_ARGS
	
	PREPARE_INITIAL_ARGS(task, args, numargs);
	
	#else
	
	/* Assume C function gets all param on stack */
	
	while(numargs--)
	{
	    _PUSH(GetSP(task), args[numargs]);
	}
	
	#endif
    }

    /* First we push the return address */
    _PUSH(GetSP(task), fallBack);
    
    /* Then set up the frame to be used by Dispatch() */
    PREPARE_INITIAL_FRAME(GetSP(task), entryPoint);
    PREPARE_INITIAL_CONTEXT(task, entryPoint);

    /* We return the new stack pointer back to the caller. */
    return TRUE;

    AROS_LIBFUNC_EXIT
}
