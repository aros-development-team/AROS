/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: ShutdownA() - Shut down the operating system (AArch64).
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <aros/libcall.h>

#include <proto/exec.h>

#include "exec_intern.h"
#include "exec_util.h"

#include "kernel_cpu.h"
#include "kernel_syscall.h"

/* See rom/exec/shutdowna.c for documentation */

AROS_LH1(ULONG, ShutdownA,
    AROS_LHA(ULONG, action, D0),
    struct ExecBase *, SysBase, 173, Exec)
{
    AROS_LIBFUNC_INIT

    /* The SVC immediate has to be a compile-time constant, so the call
     * cannot be shared between the two actions. */
    switch (action & SD_ACTION_MASK)
    {
    case SD_ACTION_POWEROFF:
        Exec_DoResetCallbacks((struct IntExecBase *)SysBase, action);
        krnSysCall(SC_POWEROFF);
        break;

    case SD_ACTION_COLDREBOOT:
        Exec_DoResetCallbacks((struct IntExecBase *)SysBase, action);
        krnSysCall(SC_REBOOT);
        break;
    }

    return 0;       /* Unknown action code, or the kernel declined */

    AROS_LIBFUNC_EXIT
}
