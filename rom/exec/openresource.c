/*
    (C) 1995 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1996/07/28 16:37:24  digulla
    Initial revision

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

	__AROS_LH1(APTR, OpenResource,

/*  SYNOPSIS */
	__AROS_LA(STRPTR, resName, A1),

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
    __AROS_FUNC_INIT

    __AROS_BASE_EXT_DECL(struct ExecBase *,SysBase)
    APTR resource;

    /* Arbitrate for the resource list */
    Forbid();

    /* Look for the resource in our list */
    resource = (APTR) FindName (&SysBase->ResourceList, resName);

    /* All done. */
    Permit();
    return resource;
    __AROS_FUNC_EXIT
} /* OpenResource */
