/*
    Copyright (C) 1997 AROS - The Amiga Research OS
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

#ifndef UseExecstubs
AROS_LH0(void, Enable,
    struct ExecBase *, SysBase, 21, Exec)
#else
void _Exec_Enable(struct ExecBase * SysBase)
#endif
{
    AROS_LIBFUNC_INIT

    if(--SysBase->IDNestCnt < 0)
    {
	/* Block all signals. We should really only block those that
	   map to interrupts.
	*/

#if 0

	if( (SysBase->AttnResched & 0x80) 
	 && (SysBase->TDNestCnt < 1)
	)
	{
		SysBase->AttnResched &= ~0x80;
		Switch();
	}

#endif

	sigprocmask(SIG_UNBLOCK, &sig_int_mask, NULL);

    }

    AROS_LIBFUNC_EXIT
}
