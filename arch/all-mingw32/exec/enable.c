/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: enable.c 27888 2008-02-19 23:01:50Z schulz $

    Desc: i386unix version of Enable()
    Lang: english
*/
#define DEBUG 1

#include <exec/tasks.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <aros/atomic.h>
#include <aros/debug.h>
//#include <asm/segments.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include "exec_intern.h"

#include "../kernel/kernel_intern.h"

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
    
    if(SysBase->IDNestCnt < 0)
    {
        D(bug("[Enable] Enabling interrupts\n"));
        if (KernelBase)
            KrnSti();
        D(else bug("[Enable] KernelBase is NULL\n"));

        /* There's no dff09c like thing in x86 native which would allow
           us to set delayed (mark it as pending but it gets triggered
           only once interrupts are enabled again) software interrupt,
           so we check it manually here in Enable() == same stuff as
           in Permit(). */
           
        if ((SysBase->TDNestCnt < 0) && (SysBase->AttnResched & ARF_AttnSwitch))
        {
            if (!KrnIsSuper()) KrnSchedule();        
        }
        
        if (SysBase->SysFlags & SFF_SoftInt)
        {
            if (!KrnIsSuper())
            {
                /* sys_Cause */
                KrnCause();
            }
        }
    }

    AROS_LIBFUNC_EXIT
}
