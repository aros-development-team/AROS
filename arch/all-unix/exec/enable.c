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
{
    AROS_LIBFUNC_INIT
#else
void _Exec_Enable(struct ExecBase * SysBase)
{
#endif

    if(--SysBase->IDNestCnt < 0)
    {
	sigprocmask(SIG_UNBLOCK, &sig_int_mask, NULL);
    }

#ifndef UseExecstubs
    AROS_LIBFUNC_EXIT
#endif
}
