/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PrepareContext() - Prepare a task context for dispatch.
    Lang: english
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <utility/tagitem.h>

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
	This function is private and is not meant to be used
	by any software. On other operating systems of Amiga(tm) family
	it does not exist.

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

    /*
     * The actual implementation of this function is CPU-specific.
     * Please see files in arch/<cpu>-all/exec/ for working examples.
     */
    return FALSE;

    AROS_LIBFUNC_EXIT
} /* PrepareContext() */
