/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Exec function Disable
    Lang: english
*/
#define timeval sys_timeval
#include <signal.h>
#undef timeval
#include <exec/execbase.h>

void disable (void);

/******************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH0(void, Disable,

/*  LOCATION */
	struct ExecBase *, SysBase, 20, Exec)

/*  FUNCTION
	This function disables the delivery of all interrupts until a
	matching call to Enable() is done. This implies a Forbid(). Since
	the system needs the regular delivery of all interrupts it is
	forbidden to disable them for longer than ~250 microseconds.

	THIS CALL IS VERY DANGEROUS!!!

	Do not use it without thinking very well about it or better do not
	use it at all. Most of the time you can live without it by using
	semaphores or similar.

	Calls to Disable() nest, i.e. for each call to Disable() you need
	one call to Enable().


    INPUTS
	None.

    RESULT
	None.

    NOTES
	You should not use this function as a cheap way to avoid
	using a semaphore.

	If you can, use Forbid() instead.

	This function preserves all registers.

	This function may be used from interrupts to disable all higher
	priority interrupts. Lower priority interrupts are disabled anyway.

	To prevent deadlocks calling Wait() in disabled state breaks the
	disable - thus interrupts and taskswitches may happen again.

    EXAMPLE

    BUGS

    SEE ALSO
	Enable(), Forbid(), Permit(), Wait()

    INTERNALS

    HISTORY

******************************************************************************/
{
    disable ();

    SysBase->IDNestCnt ++;
} /* Disable */

void disable (void)
{
    sigset_t set;

    sigfillset (&set);

    sigprocmask (SIG_BLOCK, &set, NULL);
} /* disable */
