/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: RunProcess() - Run a process from an entry point with args
    Lang: english
*/

#include <aros/asmcall.h>	/* LONG_FUNC */
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <aros/debug.h>

#include "dos_intern.h"

#ifdef __m68000

ULONG BCPL_CallEntry(STRPTR argptr, ULONG argsize, LONG_FUNC entry, struct Process *me);

#else

/* On non-m68k systems we don't implement BCPL ABI, and use the same entry code */

#define BCPL_CallEntry CallEntry

#endif

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
    	This function is actually obsolete and should be removed. It is still here
    	only for compatibility with older source code. Some architectures use own
    	version of this code. The following is needed in order to do it:
    	1. Test NewStackSwap() on PPC and x86-64 native versions.
    	2. Remove architecture-specific RunProcess() implementations from these architectures.
    	3. Move this code into RunCommand().

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

**************************************************************************/
{
    LONG ret;
    APTR oldReturnAddr = proc->pr_ReturnAddr; /* might be changed by CallEntry */
    struct StackSwapArgs args = {{
	(IPTR) argptr,
	argsize,
	(IPTR) entry,
	(IPTR) proc
    }};

    /* Call the (BCPL) function with the new stack */
    ret = NewStackSwap(sss, BCPL_CallEntry, &args);

    proc->pr_ReturnAddr = oldReturnAddr;
    return ret;
}
