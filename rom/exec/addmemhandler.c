/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.5  1996/10/24 15:50:41  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:55:56  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:02  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
/*****************************************************************************

    NAME */
	#include <exec/execbase.h>
	#include <clib/exec_protos.h>

AROS_LH1(void, AddMemHandler,

/*  SYNOPSIS */
	AROS_LHA(struct Interrupt *, memHandler, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 129, Exec)

/*  FUNCTION
	Add some function to be called if the system is low on memory.

    INPUTS
	memHandler - An Interrupt structure to add to the low memory
		     handler list.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	8-10-95    created by m. fleischer

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    /* Protect the low memory handler list */
    Forbid();
	/* Nothing spectacular: Just add the new node */
	Enqueue((struct List *)&SysBase->ex_MemHandlers,&memHandler->is_Node);
    Permit();
    AROS_LIBFUNC_EXIT
} /* AddMemHandler */

