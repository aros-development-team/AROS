/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ColdReboot() - Reboot the computer.
    Lang: english
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <aros/libcall.h>

#include <proto/exec.h>

#include "exec_util.h"

/* See rom/exec/coldreboot.c for documentation */

AROS_LH0(void, ColdReboot,
    struct ExecBase *, SysBase, 121, Exec)
{
    AROS_LIBFUNC_INIT

    Exec_DoResetCallbacks((struct IntExecBase *)SysBase, SD_ACTION_WARMREBOOT);

    asm volatile("li %%r3,%0; sc"::"i"(0x100 /*SC_REBOOT*/):"memory","r3");

    AROS_LIBFUNC_EXIT
}
