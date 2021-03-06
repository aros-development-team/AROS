/*
    Copyright (C) 1995-2010, The AROS Development Team. All rights reserved.

    Desc: ColdReboot() - Reboot the computer, Windows-hosted implementation.
*/

#include <aros/debug.h>

#include "exec_intern.h"

AROS_LH0(void, ColdReboot,
         struct ExecBase *, SysBase, 121, Exec)
{
    AROS_LIBFUNC_INIT

    Disable();
    PD(SysBase).Reboot(TRUE);
    Enable();

    AROS_LIBFUNC_EXIT
} /* ColdReboot() */
