/*
    Copyright (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: i386unix version of Enable()
    Lang: english
*/

#include <exec/tasks.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include <stdlib.h>
#include <signal.h>

extern sigset_t sig_int_mask;	/* mask of sig_t that are ints not traps */

AROS_LH0(void, Enable,
    struct ExecBase *, SysBase, 21, Exec)
{
    AROS_LIBFUNC_INIT

    if(--SysBase->IDNestCnt < 0)
    {
	/* Block all signals. We should really only block those that
	   map to interrupts.
	*/
	sigprocmask(SIG_UNBLOCK, &sig_int_mask, NULL);

	if( (SysBase->AttnResched & 0x80) 
	 && (SysBase->TDNestCnt > 0)
	)
	{
		SysBase->AttnResched &= ~0x80;
		Switch();
	}
    }

    AROS_LIBFUNC_EXIT
}
