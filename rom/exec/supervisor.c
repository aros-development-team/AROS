/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Supervisor() - Execute some code in a privileged environment.
    Lang: english
*/

#include <exec/alerts.h>

#include "exec_util.h"

/*****************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH1(IPTR, Supervisor,

/*  SYNOPSIS */
	AROS_LHA(void *, userFunction, A5),

/*  LOCATION */
	struct ExecBase *, SysBase, 5, Exec)

/*  FUNCTION
	Supervisor will allow a short privileged instruction sequence to
	be called from user mode. This has very few uses, and it is probably
	better to use any system supplied method to do anything.

	The function supplied will be called as if it was a system interrupt,
	notably this means that you must *NOT* make any system calls or
	use any system data structures, and on certain systems you must
	use special methods to leave the code.

	The code will not be passed any parameters. However it has access to all
	CPU registers.

    INPUTS
	userFunction -  The address of the code you want called in supervisor
			mode.

    RESULT
	The code will be called.

    NOTES
    	On some architectures this function is impossible or infeasible to implement.
    	In this case it throws a recoverable alert.

	Currently this function works only on x86 and PowerPC native kickstarts.

    EXAMPLE

    BUGS
	You can very easily make the system unusable with this function.
	In fact it is recommended that you do not use it at all.

    SEE ALSO
	SuperState(), UserState()

    INTERNALS
	You can do what you want with this function, even ignoring it if
	you don't think it makes any sense. But it could be quite useful
	to make it run something under different protection levels.

	You should trust that the programmer know what they are doing :-)

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /*
     * This fallback implementation throws a recovertable alert and
     * returns dummy value. Since we should actually call a real interrupt,
     * there's no generic way to simulate this.
     * See architecture-specific code for working implementations.
     */
    Exec_ExtAlert(ACPU_PrivErr & ~AT_DeadEnd, __builtin_return_address(0), CALLER_FRAME, 0, NULL, SysBase);
    return 0;

    AROS_LIBFUNC_EXIT
} /* Supervisor() */
