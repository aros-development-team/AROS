/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id: newstackswap.c 30792 2009-03-07 22:40:04Z sonic $

    Desc: NewStackSwap() - Call a function with swapped stack.
    Lang: english
*/

#include <aros/asmcall.h>
#include <aros/debug.h>
#include <exec/tasks.h>
#include <proto/exec.h>
#include <ucontext.h>

static void SwapTaskStackLimits(struct StackSwapStruct *sss)
{
    struct Task* this = FindTask(NULL);
    APTR tmp;

    tmp              = this->tc_SPLower;
    this->tc_SPLower = sss->stk_Lower;
    sss->stk_Lower   = tmp;
	
    tmp              = this->tc_SPUpper;
    this->tc_SPUpper = sss->stk_Upper;
    sss->stk_Upper   = tmp;
}

static void trampoline(IPTR (*func)(), IPTR *ret, IPTR *args)
{
    /* this was called from NewStackSwap() which also called Disable */
    Enable();
    
    *ret = AROS_UFC8(IPTR, func,
		     AROS_UFCA(IPTR, args[0], D0),
		     AROS_UFCA(IPTR, args[1], D1),
		     AROS_UFCA(IPTR, args[2], D2),
		     AROS_UFCA(IPTR, args[3], D3),
		     AROS_UFCA(IPTR, args[4], D4),
		     AROS_UFCA(IPTR, args[5], D5),
		     AROS_UFCA(IPTR, args[6], D6),
		     AROS_UFCA(IPTR, args[7], D7));

    /* this was called from NewStackSwap() which will enable again */        
    Disable();
}

/*****************************************************************************

    NAME */

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
		    function arguments

    RESULT
	A value actually returned by your function. The function will be
	running on a new stack.

    NOTES

    EXAMPLE

    BUGS
        Do not attempt to pass in a prebuilt stack - it will be erased.

	getcontext(), makecontext() and swapcontext() theoretically may
	fail, we do not check for this.

    SEE ALSO
	StackSwap()

    INTERNALS
	This function MUST be replaced in $(KERNEL) or $(ARCH).

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    IPTR ret;
    ucontext_t ucx, ucx_return;
    
    Disable();  /* To avoid random crashes during startup */
    getcontext(&ucx);
    Enable();
    
    ucx.uc_stack.ss_sp    = sss->stk_Lower;
    ucx.uc_stack.ss_size  = (size_t)sss->stk_Upper - (size_t)sss->stk_Lower;
    ucx.uc_stack.ss_flags = SS_ONSTACK;
    
    ucx.uc_link = &ucx_return;

    makecontext(&ucx, (void (*)()) trampoline, 3, entry, &ret, args->Args);
    
    /*
       we enable again in trampoline, after we have swapped
       the new stack borders into the task structure
    */
    Disable();
    SwapTaskStackLimits(sss);

    swapcontext(&ucx_return, &ucx);

    SwapTaskStackLimits(sss);
    Enable();

    return ret;
    
    AROS_LIBFUNC_EXIT
} /* NewStackSwap() */
