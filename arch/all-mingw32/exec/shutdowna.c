/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ShutdownA() - Shut down the operating system, Windows-hosted implementation.
    Lang: english
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

    switch(action)
    {
    case SD_ACTION_POWEROFF:
	PD(SysBase).ExitProcess(0);
	break;

    case SD_ACTION_COLDREBOOT:
	PD(SysBase).Reboot(FALSE);
	break;
    }

    Enable();
    return 0;

    AROS_LIBFUNC_EXIT
}
