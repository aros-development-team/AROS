/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1996/08/13 13:56:07  digulla
    Replaced __AROS_LA by __AROS_LHA
    Replaced some __AROS_LH*I by __AROS_LH*
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

	__AROS_LH1(void, RemMemHandler,

/*  SYNOPSIS */
	__AROS_LHA(struct Interrupt *, memHandler, A1),

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
    __AROS_FUNC_INIT

    /* Protect the low memory handler list */
    Forbid();
	/* Nothing spectacular: Just remove node */
	Remove(&memHandler->is_Node);
    Permit();
    __AROS_FUNC_EXIT
} /* RemMemHandler */

