/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: i386unix version of Enable()
    Lang: english
*/

#include <exec/tasks.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

AROS_LH0(void, Enable,
    struct ExecBase *, SysBase, 21, Exec)
{
    AROS_LIBFUNC_INIT

    if(--SysBase->IDNestCnt < 0)
    {
    	__asm__ __volatile__ ("sti");
    }

    AROS_LIBFUNC_EXIT
}
