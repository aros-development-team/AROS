/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Change the stack of a task.
    Lang: english
*/
#include <stdlib.h>
#include <aros/debug.h>

/******************************************************************************

    NAME */
#include <exec/tasks.h>
#include <proto/exec.h>

	AROS_LH1(void, StackSwap,

/*  SYNOPSIS */
	AROS_LHA(struct StackSwapStruct *, sss, A0),

/*  LOCATION */
	struct ExecBase *, SysBase, 122, Exec)

/*  FUNCTION
	Change the stack of a task.

    INPUTS
	sss - The description of the new stack

    RESULT
	There will be a new stack.

    NOTES
	Calling this routine the first time will change sss and
	calling it a second time, the changes will be undone.

    EXAMPLE

    BUGS
        This function isn't really portable across compilers, as
	it relies on the compiler NOT TO perform certain kind
	of optimizations. 
	
	This function is therefore declared obsolete and should
	never be used. In its place CallWithStack() can be used instead.

    SEE ALSO

    INTERNALS
	This is a symmetrical routine. If you call it twice, then
	everything will be as it was before.

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    kprintf("StackSwap() is deprecated and should NEVER be called. ABORTING.\n");
    abort();

    AROS_LIBFUNC_EXIT
} /* StackSwap */
