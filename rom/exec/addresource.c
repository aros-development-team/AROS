/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Add a resource to the public list of resources.
    Lang: english
*/
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(void, AddResource,

/*  SYNOPSIS */
	AROS_LHA(APTR, resource, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 81, Exec)

/*  FUNCTION
	Adds a given resource to the system's resource list.

    INPUTS
	resource - Pointer to a ready for use resource.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	RemResource(), OpenResource()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    ASSERT_VALID_PTR(resource);

    /* Just in case the user forgot them */
    ((struct Node *)resource)->ln_Type=NT_RESOURCE;

    /* Arbitrate for the resource list */
    Forbid();

    /* And add the resource */
    Enqueue(&SysBase->ResourceList,(struct Node *)resource);

    /* All done. */
    Permit();
    AROS_LIBFUNC_EXIT
} /* AddResource */

