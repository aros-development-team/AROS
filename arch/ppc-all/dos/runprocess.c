/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id: $

    Desc: RunProcess() - Run a process from an entry point with args
    Lang: english
*/

#include <aros/asmcall.h>	/* LONG_FUNC */

#include <dos/dosextens.h>
#include <proto/exec.h>

#define DEBUG 0
#include <aros/debug.h>

#include <string.h>

#define SysBase	    (DOSBase->dl_SysBase)

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
    APTR *	newSP = (APTR *)(sss->stk_Upper);
    APTR *	sf = (APTR*) AROS_GET_SP;
    APTR *	oldSP = *sf - 4;
    ULONG	ret;
    ULONG *	retptr = &ret;
    APTR 	oldReturnAddr = proc->pr_ReturnAddr;

    /* Compute argsize automatically */
    if (argsize == -1)
    {
	argsize = strlen(argptr);
    }

    /* Copy stack + locals + regs + everything */
    while ( oldSP != sf - 1 )
    {
	*--newSP = *oldSP--;
    }

    // zero back chain
    *newSP = 0;
    sss->stk_Pointer = newSP;

    D(bug("In RunProcess() entry=%lx, *entry=%lx\n", (IPTR)entry, (IPTR)*entry));

    StackSwap(sss);
    D(bug("In RunProcess() After StackSwap\n"));

    /* Call the function with the new stack */
    /*
	We have to set the pr_ReturnAddr pointer to the correct value
	before we call the entry() otherwise some startup code will
	not work.

	This can be done rather more easily on the m68k than elsewhere.
    */
#ifndef AROS_UFC3R
#error You need to write the AROS_UFC3R macro for your CPU
#endif

    *retptr = AROS_UFC3R(ULONG, entry,
		AROS_UFCA(STRPTR, argptr, A0),
		AROS_UFCA(ULONG, argsize, D0),
		AROS_UFCA(struct ExecBase *, SysBase, A6),
		&proc->pr_ReturnAddr,
		(sss->stk_Upper - (ULONG) sss->stk_Lower)
	      );

    StackSwap(sss);

    proc->pr_ReturnAddr = oldReturnAddr;

    /*return ret; does not work with -O2 */
    return *retptr;
}
