/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ColdReboot() - Reboot the computer, Unix-hosted implementation.
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
