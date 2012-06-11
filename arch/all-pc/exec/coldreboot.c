/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ColdReboot() - Reboot the computer.
    Lang: english
*/

#include <aros/libcall.h>

#include "exec_util.h"
#include "kernel_syscall.h" /* Comes from rom/kernel */

AROS_LH0(void, ColdReboot,
	 struct ExecBase *, SysBase, 121, Exec)
{
    AROS_LIBFUNC_INIT

    Exec_DoResetCallbacks((struct IntExecBase *)SysBase);

    __asm__ __volatile__ ("int $0x80"::"a"(SC_REBOOT));

    AROS_LIBFUNC_EXIT
} /* Supervisor() */
