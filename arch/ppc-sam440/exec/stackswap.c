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
    AROS_LIBFUNC_INIT
    register IPTR real_sp asm("r1");
    IPTR *sp;
    IPTR *src;
    IPTR *dst;
    struct Task *thisTask = FindTask(NULL);
    APTR tmp;
    
    dst = sss->stk_Pointer;
    
    Disable();
    
    /* Get the real stack pointer */
    asm volatile ("mr %0,%1":"=r"(sp):"r"(real_sp));
    
    /* Go one stack frame upper - now src points to the stackframe of caller */
    src = (IPTR*)*sp;
    
    /* Go one more stack frame up. Now you may copy from src to dst (src - sp) IPTR's */ 
    src = (IPTR*)*src;
    
    /* Copy the two stack frames */
    while (src != sp)
    {
        *--dst = *--src;
    }
    
    sss->stk_Pointer = dst;
    
    tmp = thisTask->tc_SPLower;
    thisTask->tc_SPLower = sss->stk_Lower;
    sss->stk_Lower = tmp;

    tmp = thisTask->tc_SPUpper;
    thisTask->tc_SPUpper = sss->stk_Upper;
    sss->stk_Upper = tmp;
    
    tmp = thisTask->tc_SPReg;
    thisTask->tc_SPReg = sss->stk_Pointer;
    sss->stk_Pointer = tmp;

    asm volatile("mr %0,%1"::"r"(real_sp),"r"(dst));
    
    Enable();
    
    AROS_LIBFUNC_EXIT
} /* StackSwap() */
