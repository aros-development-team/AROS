/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Forbid() - Prevent tasks switches from taking place.
    Lang: english
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <aros/atomic.h>

/*****************************************************************************/
#undef  Exec
#ifdef UseExecstubs
#    define Exec _Exec
#endif

/*  NAME */
#include <proto/exec.h>

	AROS_LH0(void, Forbid,

/*  LOCATION */
	struct ExecBase *, SysBase, 22, Exec)

/*  FUNCTION
	Forbid any further taskswitches until a matching call to Permit().
	Naturally disabling taskswitches means:

	THIS CALL IS DANGEROUS

	Do not use it without thinking very well about it or better
	do not use it at all. Most of the time you can live without
	it by using semaphores or similar.

	Calls to Forbid() nest, i.e. for each call to Forbid() you
	need one call to Permit().

    INPUTS
	None.

    RESULT
	The multitasking state will be disabled AFTER this function
	returns to the caller.

    NOTES
	This function preserves all registers.

	To prevent deadlocks calling Wait() in forbidden state breaks
	the forbid - thus taskswitches may happen again.

    EXAMPLE
	No you really don't want to use this function.

    BUGS
	The only architecture that you can rely on the registers being
	saved is on the Motorola mc68000 family.

    SEE ALSO
	Permit(), Disable(), Enable(), Wait()

    INTERNALS
	If you want to preserve all the registers, replace this function
	in your $(KERNEL) directory. Otherwise this function is
	satisfactory.

******************************************************************************/
{
#undef Exec

    AROS_LIBFUNC_INIT

    AROS_ATOMIC_INC(SysBase->TDNestCnt);

    AROS_LIBFUNC_EXIT
} /* Forbid() */
