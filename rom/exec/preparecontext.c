/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PrepareContext() - Prepare a task context for dispatch.
    Lang: english
*/

#include <exec/types.h>
#include <aros/libcall.h>
#include <exec/execbase.h>
#include <utility/tagitem.h>

/*****i***********************************************************************

    NAME */
	AROS_LH4(BOOL, PrepareContext,

/*  SYNOPSIS */
	AROS_LHA(struct Task *, task,    A0),
	AROS_LHA(APTR, entryPoint,      A1),
	AROS_LHA(APTR, fallBack,        A2),
	AROS_LHA(struct TagItem *, tagList, A3),

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
	This function is required to be implemented in the $(KERNEL).

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* This function is too CPU dependant to be described here, but
	basically you effectively do a state save (similar to in
	Dispatch()) but of a context that has not been run yet.
    */

#error The PrepareContext() function was not implemented in the kernel.

    AROS_LIBFUNC_EXIT
} /* PrepareContext() */
