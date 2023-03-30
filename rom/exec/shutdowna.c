/*
    Copyright (C) 1995-2023, The AROS Development Team. All rights reserved.

    Desc: ShutdownA() - Shut down the operating system.
*/

#include <aros/debug.h>
#include <proto/exec.h>

#include "exec_util.h"
#include "exec_debug.h"

static void ShutdownHandler(struct ExecBase *SysBase, IPTR action)
{
    DSHUTDOWN("Shutdown Handler started..");

    Exec_DoResetCallbacks((struct IntExecBase *)SysBase, action);

    /* We shouldn't get here. If a reset handler has failed to shut down
     * the system, the system may still be unstable as a result of
     * peripheral device resets */
}

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

    NewCreateTask(TASKTAG_NAME       , "Shutdown",
                    TASKTAG_PRI        , 127,
                    TASKTAG_PC         , ShutdownHandler,
                    TASKTAG_ARG1       , SysBase,
                    TASKTAG_ARG2       , action,
                    TAG_DONE);

    return 0;

    AROS_LIBFUNC_EXIT
}
