/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Removes a resource from the list of public resources.
    Lang: english
*/
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(void, RemResource,

/*  SYNOPSIS */
	AROS_LHA(APTR, resource,A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 82, Exec)

/*  FUNCTION
	Removes a resource from the system resource list.

    INPUTS
	resource - Pointer to the resource.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	AddResource(), OpenResource()

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Arbitrate for the resource list */
    Forbid();

    Remove((struct Node *)resource);

    /* All done. */
    Permit();
    AROS_LIBFUNC_EXIT
} /* RemResource */

