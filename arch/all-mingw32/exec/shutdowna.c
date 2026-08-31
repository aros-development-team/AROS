/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: ShutdownA() - Shut down the operating system, Windows-hosted implementation.
*/

#include <aros/debug.h>

#include "exec_intern.h"

AROS_LH1(ULONG, ShutdownA,
         AROS_LHA(ULONG, action, D0),
         struct ExecBase *, SysBase, 173, Exec)
{
    AROS_LIBFUNC_INIT

    /* WinAPI CreateProcess() call may silently abort if scheduler attempts task switching
       while it's running. There's no sense in this beyond this point, so we simply Disable() */
    Disable();

    switch(action & SD_ACTION_MASK)
    {
    case SD_ACTION_POWEROFF:
        PD(SysBase).ExitProcess(0);
        break;

    case SD_ACTION_COLDREBOOT:
    case SD_ACTION_REBOOT:      /* hosted restart serves either flavour */
        PD(SysBase).Reboot(FALSE);
        break;
    }

    Enable();
    return 0;

    AROS_LIBFUNC_EXIT
}
