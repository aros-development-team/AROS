/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: RunProcess() - Run a process from an entry point with args
    Lang: english
*/
#include <aros/asmcall.h>	/* LONG_FUNC */
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <aros/debug.h>

#include <string.h>

static ULONG CallEntry(APTR pReturn_Addr, struct StackSwapStruct* sss,
		       STRPTR argptr, ULONG argsize, LONG_FUNC entry)
{

#ifndef AROS_UFC3R
#error You need to write the AROS_UFC3R macro for your CPU
#endif

    return AROS_UFC3R(ULONG, entry,
		      AROS_UFCA(STRPTR, argptr, A0),
		      AROS_UFCA(ULONG, argsize, D0),
		      AROS_UFCA(struct ExecBase *, SysBase, A6),
		      pReturn_Addr,
		      (sss->stk_Upper - (ULONG)sss->stk_Lower) /* used by m68k-linux arch, needed?? */
		     );

}

/**************************************************************************

    NAME */
	LONG AROS_SLIB_ENTRY(RunProcess,Dos) (

/*  SYNOPSIS */
	struct Process		* proc,
	struct StackSwapStruct  * sss,
	CONST_STRPTR		  argptr,
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
    LONG ret;
    APTR oldReturnAddr = proc->pr_ReturnAddr; /* might be changed by CallEntry */
    struct StackSwapArgs args = {{
	(IPTR) &proc->pr_ReturnAddr,
	(IPTR) sss,
	(IPTR) argptr,
	argsize == -1 ? strlen(argptr) : argsize, /* Compute argsize automatically */
	(IPTR) entry
    }};
    
    /* Call the function with the new stack */
    ret = NewStackSwap(sss, CallEntry, &args);

    proc->pr_ReturnAddr = oldReturnAddr;
    return ret;
}
