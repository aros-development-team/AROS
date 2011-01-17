/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: RunProcess() - Run a process from an entry point with args
    Lang: english
*/

#define DEBUG 0
#include <aros/asmcall.h>	/* LONG_FUNC */
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <aros/debug.h>
#include <string.h>

/**************************************************************************

    NAME */
	LONG AROS_SLIB_ENTRY(RunProcess,Dos) (

/*  SYNOPSIS */
	struct Process		* proc,
	struct StackSwapStruct  * sss,
	STRPTR			  argptr,
	ULONG			  argsize,
	LONG_FUNC		  entry,
	struct DosLibrary 	* DOSBase)

/* FUNCTION
	Sets the stack as specified and calls the routine with the given
	arguments.

    INPUTS
	proc	- Process context
	sss	- New Stack
	argptr	- Pointer to argument string
	argsize	- Size of the argument string
	entry	- The entry point of the function
	DOSBase	- Pointer to dos.library structure

    RESULT
	The return value of (*entry)();

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

**************************************************************************/
{
    ULONG *	retptr;
    ULONG	ret;
    APTR 	oldReturnAddr;
    register ULONG real_sp asm("r1");
    ULONG *sp;
    ULONG *src;
    ULONG *dst;
    void *tmp;

    /* Get the real stack pointer */
    asm volatile ("mr %0,%1":"=r"(sp):"r"(real_sp));
        
    /* Go one stack frame upper - now src points to the stackframe of caller */
    src = (ULONG*)*sp;
        
    /* Go one more stack frame up. Now you may copy from src to dst (src - sp) IPTR's */ 
    src = (ULONG*)*src;

    dst = (ULONG*)((IPTR)sss->stk_Upper - SP_OFFSET);

    /* Rewind the dst pointer too. */
    //dst += (src - sp);
    
    /* Copy the two stack frames */
    while (src != sp)
    {
        *--dst = *--src;
    }
    
    sss->stk_Pointer = dst;
    
    retptr = &ret;

    oldReturnAddr = proc->pr_ReturnAddr;

    /* Compute argsize automatically */
    if (argsize == -1)
    {
	argsize = strlen(argptr);
    }

    D(bug("In RunProcess() entry=%lx, *entry=%lx\n", (IPTR)entry, (IPTR)*entry));

    D(bug("[sss] %08x %08x %08x\n", sss->stk_Lower, sss->stk_Pointer, sss->stk_Upper));
    
    StackSwap(sss);

    D(bug("[sss] %08x %08x %08x\n", sss->stk_Lower, sss->stk_Pointer, sss->stk_Upper));

    /* Call the function with the new stack */
    /*
	We have to set the pr_ReturnAddr pointer to the correct value
	before we call the entry() otherwise some startup code will
	not work.

	This can be done rather more easily on the m68k than elsewhere.
    */

    *retptr = AROS_UFC3R(ULONG, entry,
		AROS_UFCA(STRPTR, argptr, A0),
		AROS_UFCA(ULONG, argsize, D0),
		AROS_UFCA(struct ExecBase *, SysBase, A6),
		&proc->pr_ReturnAddr, (sss->stk_Upper - (ULONG)sss->stk_Lower)
	      );

    StackSwap(sss);

    D(bug("[sss] %08x %08x %08x\n", sss->stk_Lower, sss->stk_Pointer, sss->stk_Upper));

    proc->pr_ReturnAddr = oldReturnAddr;

    return ret;
}

