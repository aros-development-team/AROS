/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: i386unix version of Enable()
    Lang: english
*/

#include <exec/tasks.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <aros/atomic.h>
#include <proto/exec.h>

#include <stdlib.h>
#include <signal.h>

extern sigset_t sig_int_mask;	/* mask of sig_t that are ints not traps */

#undef  Exec
#ifdef UseExecstubs
#    define Exec _Exec
#endif

AROS_LH0(void, Enable,
    struct ExecBase *, SysBase, 21, Exec)
{

#undef Exec

    AROS_LIBFUNC_INIT

    AROS_ATOMIC_DEC(SysBase->IDNestCnt);

    if(SysBase->IDNestCnt < 0)
    {
	sigprocmask(SIG_UNBLOCK, &sig_int_mask, NULL);
    }

    AROS_LIBFUNC_EXIT
}
