/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#define timeval sys_timeval
#include <signal.h>
#undef timeval
#include <exec/execbase.h>

void en (void);

/******************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH0(void, Enable,

/*  LOCATION */
	struct ExecBase *, SysBase, 21, Exec)

/*  FUNCTION
	This function reenables the delivery of interrupts after a call to
	Disable().

    INPUTS
	None.

    RESULT
	None.

    NOTES
	This function may be used from interrupts.

	This function preserves all registers.

    EXAMPLE

    BUGS

    SEE ALSO
	Forbid(), Permit(), Disable()

    INTERNALS

    HISTORY

******************************************************************************/
{
    if ((-- SysBase->IDNestCnt) < 0)
	en ();

    if ((SysBase->AttnResched & 0x80) && SysBase->TDNestCnt<0)
    {
	SysBase->AttnResched &= ~0x80;

	Switch ();
    }
} /* Enable */

void en (void)
{
    sigset_t set;

    sigfillset (&set);

    sigprocmask (SIG_UNBLOCK, &set, NULL);
} /* en */
