/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: i386unix version of Disable()
    Lang: english
*/

#include <exec/tasks.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <aros/atomic.h>
#include <proto/exec.h>

#undef  Exec
#ifdef UseExecstubs
#    define Exec _Exec
#endif

AROS_LH0(void, Disable,
    struct ExecBase *, SysBase, 20, Exec)
{
#undef Exec
    AROS_LIBFUNC_INIT

    /* Georg Steger */

	__asm__ __volatile__ ("cli");

    AROS_ATOMIC_INC(SysBase->IDNestCnt);

    AROS_LIBFUNC_EXIT
}
