/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.6  1996/12/10 13:51:50  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.5  1996/10/24 15:50:53  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:56:05  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:15  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/execbase.h>
#include <exec/lists.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
#include <exec/libraries.h>
#include <clib/exec_protos.h>

	AROS_LH1(APTR, OpenResource,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, resName, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 83, Exec)

/*  FUNCTION
	Return a pointer to a previously installed resource addressed by name.
	It this name can't be found NULL is returned.

    INPUTS
	libName - Pointer to the resource's name.

    RESULT
	Pointer to resource or NULL.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	AddResource(), RemResource()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    AROS_LIBBASE_EXT_DECL(struct ExecBase *,SysBase)
    APTR resource;

    /* Arbitrate for the resource list */
    Forbid();

    /* Look for the resource in our list */
    resource = (APTR) FindName (&SysBase->ResourceList, resName);

    /* All done. */
    Permit();
    return resource;
    AROS_LIBFUNC_EXIT
} /* OpenResource */
