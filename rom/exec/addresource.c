/*
    $Id$
    $Log$
    Revision 1.1  1996/07/28 16:37:23  digulla
    Initial revision

    Desc:
    Lang: english
*/
#include <exec/execbase.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
	#include <clib/exec_protos.h>

	__AROS_LH1(void, AddResource,

/*  SYNOPSIS */
	__AROS_LA(APTR, resource, A1),

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

    HISTORY

******************************************************************************/
{
    __AROS_FUNC_INIT

    /* Just in case the user forgot them */
    ((struct Node *)resource)->ln_Type=NT_RESOURCE;

    /* Arbitrate for the resource list */
    Forbid();

    /* And add the resource */
    Enqueue(&SysBase->ResourceList,(struct Node *)resource);

    /* All done. */
    Permit();
    __AROS_FUNC_EXIT
} /* AddResource */

