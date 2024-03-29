/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.

    Desc: ColdReboot() - Reboot the computer.
*/

#include <proto/exec.h>

#include <aros/libcall.h>

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/tasks.h>

#include "exec_intern.h"
#include "exec_util.h"

/* See rom/exec/coldreboot.c for documentation */

AROS_LH0(void, ColdReboot,
    struct ExecBase *, SysBase, 121, Exec)
{
    AROS_LIBFUNC_INIT

    Exec_DoResetCallbacks((struct IntExecBase *)SysBase, SD_ACTION_WARMREBOOT);

    AROS_LIBFUNC_EXIT
}
