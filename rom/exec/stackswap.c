/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: StackSwap() - Swap the stack of a task.
    Lang: english
*/

/*****************************************************************************

    NAME */
#include <exec/tasks.h>
#include <proto/exec.h>

	AROS_LH1(void, StackSwap,

/*  SYNOPSIS */
	AROS_LHA(struct StackSwapStruct *,  sss, A0),

/*  LOCATION */
	struct ExecBase *, SysBase, 122, Exec)

/*  FUNCTION
	Changes the stack used by a task. The StackSwapStruct will contain
	the value of the old stack such that the stack can be reset to the
	previous version by another call to StackSwap().

	When the stack is swapped, the data on the stack(s) will not be
	altered, so the stack may not be set up for you. It is generally
	required that you replace your stack before exiting from the
	current stack frame (procedure, function call etc.).

    INPUTS
	sss     -   A structure containing the values for the upper, lower
		    and current bounds of the stack you wish to use. The
		    values will be replaced by the current values and you
		    can restore the values later.

    RESULT
	The program will be running on a new stack and sss will contain
	the old stack.

	Calling StackSwap() twice consequtively will effectively do
	nothing.

    NOTES
	Returning from the function that you call StackSwap() in can have
	unexpected results.

    EXAMPLE

    BUGS

    SEE ALSO
	AddTask(), RemTask()

    INTERNALS
	This function MUST be replaced in $(KERNEL) or $(ARCH).

******************************************************************************/
{
    /* For an example see the StackSwap() function in either i386 or
       m68k-native.

       Note that you must save any state information on the stack that is
       used in the current process, for example you should really save
       return addresses.

       Note that even if you do save that information, it is not a good
       idea to return from the procedure that StackSwap() was invoked in
       as the stack will be quite incorrect.
    */

#ifndef __CXREF__
#error The function StackSwap() has not been implemented in the kernel.
#endif

} /* StackSwap() */
