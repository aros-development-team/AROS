/*
    Copyright (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: i386unix version of Disable()
    Lang: english
*/

#include <exec/tasks.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

AROS_LH0(void, Disable,
    struct ExecBase *, SysBase, 20, Exec)
{
    AROS_LIBFUNC_INIT

    /* Georg Steger */

	__asm__ __volatile__ ("cli");

    SysBase->IDNestCnt++;

    AROS_LIBFUNC_EXIT
}
