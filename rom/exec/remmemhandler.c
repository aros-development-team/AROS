/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.5  1996/10/24 15:50:55  aros
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

/*****************************************************************************

    NAME */
	#include <clib/exec_protos.h>

	AROS_LH1(void, RemMemHandler,

/*  SYNOPSIS */
	AROS_LHA(struct Interrupt *, memHandler, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 130, Exec)

/*  FUNCTION
	Remove some function added with AddMemHandler again.

    INPUTS
	memHandler - The same Interrupt structure you gave to AddMemHandler().

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
       18-10-95    created by m. fleischer

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Protect the low memory handler list */
    Forbid();
	/* Nothing spectacular: Just remove node */
	Remove(&memHandler->is_Node);
    Permit();
    AROS_LIBFUNC_EXIT
} /* RemMemHandler */

