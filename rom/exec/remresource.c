/*
    $Id$
    $Log$
    Revision 1.1  1996/07/28 16:37:24  digulla
    Initial revision

    Desc:
    Lang: english
*/
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <dos/dos.h>

/*****************************************************************************

    NAME */
	#include <clib/exec_protos.h>

	__AROS_LH1(void, RemResource,

/*  SYNOPSIS */
	__AROS_LA(APTR, resource,A1),

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
    __AROS_FUNC_INIT

    /* Arbitrate for the resource list */
    Forbid();

    Remove((struct Node *)resource);

    /* All done. */
    Permit();
    __AROS_FUNC_EXIT
} /* RemResource */

