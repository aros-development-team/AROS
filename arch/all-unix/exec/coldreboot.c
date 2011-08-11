/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id: coldreboot.c 34578 2010-10-04 07:19:30Z sonic $

    Desc: ColdReboot() - Reboot the computer, Windows-hosted implementation.
    Lang: english
*/

#include <aros/debug.h>

#include "exec_intern.h"

AROS_LH0(void, ColdReboot,
	 struct ExecBase *, SysBase, 121, Exec)
{
    AROS_LIBFUNC_INIT

    PD(SysBase).SysIFace->exit(0x8F); /* Another magic value for the bootstrap */
    AROS_HOST_BARRIER

    AROS_LIBFUNC_EXIT
} /* ColdReboot() */
