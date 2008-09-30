/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id: disable.c 27888 2008-02-19 23:01:50Z schulz $

    Desc: i386unix version of Disable()
    Lang: english
*/

#include <exec/tasks.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <aros/atomic.h>
#include <proto/exec.h>
#include <proto/kernel.h>

extern void *priv_KernelBase;

#undef  Exec
#ifdef UseExecstubs
#    define Exec _Exec
#endif

AROS_LH0(void, Disable,
    struct ExecBase *, SysBase, 20, Exec)
{
#undef Exec
    AROS_LIBFUNC_INIT
    void *KernelBase = priv_KernelBase;

    /* Georg Steger */
    if (KernelBase)
        KrnCli();

    AROS_ATOMIC_INC(SysBase->IDNestCnt);

    AROS_LIBFUNC_EXIT
}
