/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Add a low memory handler.
    Lang: english
*/

#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

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
    ASSERT_VALID_PTR(memHandler);

    /* Protect the low memory handler list */
    Forbid();
	/* Nothing spectacular: Just add the new node */
	Enqueue((struct List *)&SysBase->ex_MemHandlers,&memHandler->is_Node);
    Permit();
    AROS_LIBFUNC_EXIT
} /* AddMemHandler */

