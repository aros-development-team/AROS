/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PrepareContext() - Prepare a task context for dispatch.
    Lang: english
*/

#include <exec/types.h>
#include <aros/libcall.h>
#include <exec/execbase.h>

/*****i***********************************************************************

    NAME */
	AROS_LH3(APTR, PrepareContext,

/*  SYNOPSIS */
	AROS_LHA(APTR, stackPointer,    A0),
	AROS_LHA(APTR, entryPoint,      A1),
	AROS_LHA(APTR, fallBack,        A2),

/*  LOCATION */
	struct ExecBase *, SysBase, 6, Exec)

/*  FUNCTION
	Allocates the space required to hold a new set of registers
	on the stack given by stackPointer and clears the area
	except the for the PC which is set to the address given by
	entryPoint. The stack will be set so that when the
	entryPoint function returns, the fallback function will be
	called.

    INPUTS
	stackPointer    -   Pointer to specific stack.
	entryPoint      -   Function to call when the new context
			    comes alive.
	fallBack        -   Address of the function to be called
			    when the entryPoint function returns.

    RESULT
	The new stackPointer with the context saved.

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
