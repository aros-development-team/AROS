/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.

    Desc: ColdReboot() - Reboot the computer (AArch64).
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <aros/libcall.h>

#include <proto/exec.h>

#include "exec_intern.h"
#include "exec_util.h"

#include "kernel_cpu.h"
#include "kernel_syscall.h"

/* See rom/exec/coldreboot.c for documentation */

AROS_LH0(void, ColdReboot,
    struct ExecBase *, SysBase, 121, Exec)
{
    AROS_LIBFUNC_INIT

    Exec_DoResetCallbacks((struct IntExecBase *)SysBase, SD_ACTION_WARMREBOOT);

    /* The kernel performs the actual reset via the BCM283x PM watchdog */
    krnSysCall(SC_REBOOT);

    AROS_LIBFUNC_EXIT
}
