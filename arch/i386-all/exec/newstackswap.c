/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id: newstackswap.c 30792 2009-03-07 22:40:04Z sonic $

    Desc: NewStackSwap() - Call a function with swapped stack.
    Lang: english
*/

/*****************************************************************************

    NAME */
#include <aros/debug.h>
#include <exec/tasks.h>
#include <proto/exec.h>

	AROS_LH3(IPTR, NewStackSwap,

/*  SYNOPSIS */
	AROS_LHA(struct StackSwapStruct *,  sss, A0),
	AROS_LHA(LONG_FUNC, entry, A1),
	AROS_LHA(struct StackSwapArgs *, args, A2),

/*  LOCATION */
	struct ExecBase *, SysBase, 122, Exec)

/*  FUNCTION
	Calls a function with a new stack.

    INPUTS
	sss     -   A structure containing the values for the upper, lower
		    and current bounds of the stack you wish to use.
	entry	-   Address of the function to call.
	args	-   A structure (actually an array) containing up to 8
		    function arguments. The function is called using C calling
		    convention (no AROS_UHFx macro needed).

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

    APTR *	oldSP;
    APTR *	sp;
    ULONG *	retptr;
    ULONG	ret;

    retptr = &ret;

    sp = (APTR *)(sss->stk_Upper);
    oldSP = &SysBase;

    /* Copy stack + locals + regs + everything */
    while ( oldSP != retptr )
    {
	*--sp = *oldSP--;
    }

    sss->stk_Pointer = sp;

    D(bug("In NewStackSwap() entry=%lx, *entry=%lx\n", (IPTR)entry, (IPTR)*entry));
    StackSwap(sss);

    /* Call the function with the new stack */
    *retptr = entry(args->Args[0], args->Args[1], args->Args[2], args->Args[3],
		    args->Args[4], args->Args[5], args->Args[6], args->Args[7]);

    StackSwap(sss);

    return ret;
    
    AROS_LIBFUNC_EXIT
} /* NewStackSwap() */
