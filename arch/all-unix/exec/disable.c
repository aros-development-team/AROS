/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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

#ifndef UseExecstubs
AROS_LH0(void, Disable,
    struct ExecBase *, SysBase, 20, Exec)
{
    AROS_LIBFUNC_INIT
#else
void _Exec_Disable(struct ExecBase * SysBase)
{
#endif

    sigprocmask(SIG_BLOCK, &sig_int_mask, NULL);
    if (++SysBase->IDNestCnt < 0)
    {
	/* If we get here we have big trouble. Someone called
	   1x Disable() and 2x Enable(). IDNestCnt < 0 would
	   mean enable interrupts, but the caller of Disable
	   relies on the function to disable them, so we don´t
	   do anything here (or maybe a deadend alert?) */
    }

#ifndef UseExecstubs
    AROS_LIBFUNC_EXIT
#endif
}
