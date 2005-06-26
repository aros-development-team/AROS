/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Open a resource.
    Lang: english
*/
#include <exec/execbase.h>
#include <exec/lists.h>
#include <aros/libcall.h>
#include <exec/libraries.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(APTR, OpenResource,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, resName, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 83, Exec)

/*  FUNCTION
	Return a pointer to a previously installed resource addressed by
	name.  If this name can't be found NULL is returned.

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
