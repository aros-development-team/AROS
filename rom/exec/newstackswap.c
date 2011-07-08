/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: NewStackSwap() - Call a function with swapped stack.
    Lang: english
*/

/*****************************************************************************

    NAME */
#include <exec/tasks.h>
#include <proto/exec.h>

	AROS_LH3(IPTR, NewStackSwap,

/*  SYNOPSIS */
	AROS_LHA(struct StackSwapStruct *,  sss, A0),
	AROS_LHA(LONG_FUNC, entry, A1),
	AROS_LHA(struct StackSwapArgs *, args, A2),

/*  LOCATION */
	struct ExecBase *, SysBase, 134, Exec)

/*  FUNCTION
	Calls a function with a new stack. 

    INPUTS
	sss     -   A structure containing the values for the upper, lower
		    and current bounds of the stack you wish to use.
	entry	-   Address of the function to call.
	args	-   A structure (actually an array) containing up to 8
		    function arguments

    RESULT
	A value actually returned by your function. The function will be
	running on a new stack.

    NOTES

    EXAMPLE

    BUGS
        Do not attempt to pass in a prebuilt stack - it will be erased.

    SEE ALSO
	StackSwap()

    INTERNALS
	This function MUST be replaced in $(KERNEL) or $(ARCH).

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* For an example see the NewStackSwap() function in either i386 or
       m68k-native.

       Note that you must save any state information on the stack that is
       used in the current process, for example you should really save
       return addresses.

       Note that even if you do save that information, it is not a good
       idea to return from the procedure that StackSwap() was invoked in
       as the stack will be quite incorrect.
    */

#ifndef __CXREF__
#error The function NewStackSwap() has not been implemented in the kernel.
#endif

    return 0;
    
    AROS_LIBFUNC_EXIT
} /* NewStackSwap() */
