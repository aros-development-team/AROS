/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ColdReboot() - Reboot the computer, Unix-hosted implementation.
    Lang: english
*/

#include <aros/debug.h>

#include "exec_intern.h"
#include "exec_util.h"

AROS_LH0(void, ColdReboot,
	 struct ExecBase *, SysBase, 121, Exec)
{
    AROS_LIBFUNC_INIT

    Exec_DoResetCallbacks((struct IntExecBase *)SysBase, SD_ACTION_WARMREBOOT);

    PD(SysBase).SysIFace->exit(0x8F); /* Another magic value for the bootstrap */
    AROS_HOST_BARRIER

    AROS_LIBFUNC_EXIT
} /* ColdReboot() */
