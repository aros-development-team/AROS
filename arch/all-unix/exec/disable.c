/*
    Copyright (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: i386unix version of Disable()
    Lang: english
*/

#include <exec/tasks.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include <stdlib.h>
#include <signal.h>

extern sigset_t sig_int_mask;	/* Mask of sig_t that are ints, not traps */

AROS_LH0(void, Disable,
    struct ExecBase *, SysBase, 20, Exec)
{
    AROS_LIBFUNC_INIT

    if(SysBase->IDNestCnt++ < 0)
    {
	/* Block all signals. We should really only block those that
	   map to interrupts.
	*/
	sigprocmask(SIG_BLOCK, &sig_int_mask, NULL);
    }

    AROS_LIBFUNC_EXIT
}
