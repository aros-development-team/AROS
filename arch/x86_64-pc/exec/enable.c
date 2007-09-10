/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: enable.c 22006 2004-08-05 19:06:53Z stegerg $

    Desc: i386unix version of Enable()
    Lang: english
*/

#include <exec/tasks.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <aros/atomic.h>
#include <aros/debug.h>
//#include <asm/segments.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include "exec_intern.h"

#include "x86_64.h"

#undef  Exec
#ifdef UseExecstubs
#    define Exec _Exec
#endif

AROS_LH0(void, Enable,
    struct ExecBase *, SysBase, 21, Exec)
{
#undef Exec
    AROS_LIBFUNC_INIT

    void *KernelBase = TLS_GET(KernelBase);
    
    AROS_ATOMIC_DEC(SysBase->IDNestCnt);
    
    if(SysBase->IDNestCnt < 0)
    {
    	__asm__ __volatile__ ("sti");

	/* There's no dff09c like thing in x86 native which would allow
	   us to set delayed (mark it as pending but it gets triggered
	   only once interrupts are enabled again) software interrupt,
	   so we check it manually here in Enable() == same stuff as
	   in Permit(). */
	   
	if ((SysBase->TDNestCnt < 0) && (SysBase->AttnResched & ARF_AttnSwitch))
	{
	    if (IN_USER_MODE) KrnSchedule();	    
	}
	
	if (SysBase->SysFlags & SFF_SoftInt)
	{
	    if (IN_USER_MODE)
	    {
	    	/* sys_Cause */
                KrnCause();
	    }
	}
    }

    AROS_LIBFUNC_EXIT
}
