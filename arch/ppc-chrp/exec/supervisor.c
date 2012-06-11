/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Supervisor() - Execute some code in a privileged environment.
    Lang: english
*/

/*****************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH1(ULONG, Supervisor,

/*  SYNOPSIS */
	AROS_LHA(ULONG_FUNC, userFunction, A5),

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

	The code will not be passed any parameters.

    INPUTS
	userFunc    -   The address of the code you want called in supervisor
			mode.

    RESULT
	The code will be called.

    NOTES
	This function has completely different effects on different
	processors and architectures.

	Currently defined effects are:

	Kernel                      Effect
	-------------------------------------------------------------------
	i386 (under emulation)      None
	m68k (native)               Runs the process in supervisor mode.
				    The process must end with an RTE
				    instruction. It should save any
				    registers which is uses.
	m68k (under emulation)

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

    register ULONG retval;
    register APTR stack;

    stack = SuperState();

    retval = (*userFunction)();

    UserState(stack);

    return retval;

    AROS_LIBFUNC_EXIT
} /* Supervisor() */
