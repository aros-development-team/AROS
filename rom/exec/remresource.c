/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.6  1996/12/10 13:51:53  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.5  1996/10/24 15:50:56  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:56:07  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:17  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <dos/dos.h>

/*****************************************************************************

    NAME */
#include <clib/exec_protos.h>

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

