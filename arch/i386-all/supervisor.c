/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Call one function in supervisor mode
    Lang: english
*/

/******************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH1(void, Supervisor,

/*  SYNOPSIS */
	AROS_LHA(ULONG_FUNC, userFunction, A5),

/*  LOCATION */
	struct ExecBase *, SysBase, 5, Exec)

/*  FUNCTION
	Call the specified function in supervisor mode.

    INPUTS
	userFunction - Call this function

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
	The emulation has no real supervisor mode. So we just call the
	function.

    HISTORY

******************************************************************************/
{
    (*userFunction) ();
} /* Supervisor */

