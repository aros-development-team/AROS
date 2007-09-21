/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id: runprocess.c 24234 2006-03-27 20:09:51Z verhaegs $

    Desc: RunProcess() - Run a process from an entry point with args
    Lang: english
*/

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
    APTR               *sp;
    IPTR	       ret;
    register IPTR      *retptr = &ret;
    APTR 	      oldReturnAddr;
    
    oldReturnAddr = proc->pr_ReturnAddr;

    /* Compute argsize automatically */
    if (argsize == -1)
    {
	argsize = strlen(argptr);
    }

    sss->stk_Pointer = sss->stk_Upper;
    StackSwap(sss);

    /* Call the function with the new stack */
    /*
	We have to set the pr_ReturnAddr pointer to the correct value
	before we call the entry() otherwise some startup code will
	not work.

	This can be done rather more easily on the m68k than elsewhere.
    */

    /* On x86_64 one does not need to use any UFC3R macro! */

    *retptr = entry(argptr,argsize,SysBase);

    StackSwap(sss);

    proc->pr_ReturnAddr = oldReturnAddr;

    return ret;
}
