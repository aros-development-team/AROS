/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id: supervisor.c 31451 2009-06-20 21:21:46Z neil $

    Desc: Supervisor() - Execute some code in a priviledged environment.
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
