/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.7  1997/01/01 03:46:04  ldp
    Committed Amiga native (support) code

    Changed clib to proto

    Revision 1.6  1996/12/10 13:51:35  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.5  1996/10/24 15:50:42  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:55:56  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:02  digulla
    Added standard header for all files

    Desc:
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

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

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

