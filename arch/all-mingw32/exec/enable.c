/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Windows-hosted version of Enable()
    Lang: english
*/
#define DEBUG 0

#include <exec/tasks.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <aros/atomic.h>
#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include "exec_intern.h"

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
    D(bug("[Exec] Enable(), new IDNestCnt is %d\n", SysBase->IDNestCnt));

    /* A supervisor mode is interrupt itself. Syscalls are not allowed,
       everything will be processed later. Otherwise we are in trouble
       (like Windows exception handler being preempted by task switcher) */
    if (KrnIsSuper())
	return;

    if(SysBase->IDNestCnt < 0)
    {
        D(bug("[Enable] Enabling interrupts\n"));
        KrnSti();

        /* There's no dff09c like thing in x86 native which would allow
           us to set delayed (mark it as pending but it gets triggered
           only once interrupts are enabled again) software interrupt,
           so we check it manually here in Enable() == same stuff as
           in Permit(). */
           
        if ((SysBase->TDNestCnt < 0) && (SysBase->AttnResched & ARF_AttnSwitch))
        {
            KrnSchedule();        
        }
        
        if (SysBase->SysFlags & SFF_SoftInt)
            KrnCause();
    }

    AROS_LIBFUNC_EXIT
}
