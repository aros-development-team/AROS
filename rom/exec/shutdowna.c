/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.

    Desc: ShutdownA() - Shut down the operating system.
*/

#include <aros/debug.h>
#include <proto/exec.h>

#include "exec_util.h"
#include "exec_debug.h"

/*****************************************************************************

    NAME */

        AROS_NTLH1(ULONG, ShutdownA,

/*  SYNOPSIS */
        AROS_LHA(ULONG, action, D0),

/*  LOCATION */
        struct ExecBase *, SysBase, 173, Exec)

/*  FUNCTION
        This function attempts to shut down registered handlers
        before rebooting the system, or entering a powered off state.

    INPUTS
        action - which process to perform:
         * SD_ACTION_POWEROFF   - power off/halt the hardware.
         * SD_ACTION_COLDREBOOT - cold reboot the hardware.
         * SD_ACTION_WARMREBOOT - soft reboot the operating system.

    RESULT
        This function does not return in case of success. Otherwise it returns
        zero.

    NOTES
        It can be quite harmful to call this function. It may be possible that
        you will lose data from other tasks not having saved, or disk buffers
        not being flushed. Plus you could annoy the (other) users.

    EXAMPLE

    BUGS

    SEE ALSO
        ColdReboot()

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    Exec_DoResetCallbacks((struct IntExecBase *)SysBase, action);

    return 0;

    AROS_LIBFUNC_EXIT
}
