/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/execbase.h>

/******************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH0(void, Forbid,

/*  LOCATION */
	struct ExecBase *, SysBase, 22, Exec)

/*  FUNCTION
	Forbid any further taskswitches until a matching call to Permit().
	Naturally disabling taskswitches means:

	THIS CALL IS DANGEROUS

	Do not use it without thinking very well about it or better do not
	use it at all. Most of the time you can live without it by using
	semaphores or similar.

	Calls to Forbid() nest, i.e. for each call to Forbid() you need one
	call to Enable().

    INPUTS
	None.

    RESULT
	None.

    NOTES
	To prevent deadlocks calling Wait() in forbidden state breaks the
	forbid - thus taskswitches may happen again.

	This function preserves all registers.

    EXAMPLE

    BUGS

    SEE ALSO
	Permit(), Disable(), Enable(), Wait()

    INTERNALS

    HISTORY

******************************************************************************/
{
    ++SysBase->TDNestCnt;
} /* Forbid */
