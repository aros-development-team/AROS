/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id: supervisor.c 31451 2009-06-20 21:21:46Z neil $

    Desc: Supervisor() - Execute some code in a priviledged environment.
    Lang: english
*/

#include <aros/libcall.h>
#include <exec/execbase.h>

#include "kernel_syscall.h"

AROS_LH1I(IPTR, Supervisor,
	 AROS_LHA(void *, userFunction, A5),
	 struct ExecBase *, SysBase, 5, Exec)
{
    AROS_LIBFUNC_INIT

    IPTR retval;

    /* Put function pointer into e(r)dx because on x86-64 it doesn't require additional reload */
    __asm__ __volatile__ ("int $0x80":"=a"(retval):"a"(SC_SUPERVISOR),"D"(userFunction));
    return retval;

    AROS_LIBFUNC_EXIT
} /* Supervisor() */
