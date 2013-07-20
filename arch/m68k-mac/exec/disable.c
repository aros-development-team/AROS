/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Disable() - Stop interrupts from occurring.
    Lang: english
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <asm/registers.h>

#undef Exec
#ifdef UseExecstubs
#    define Exec _Exec
#endif

#include <proto/exec.h>

/* See rom/exec/disable.c for documentation */

AROS_LH0(void, Disable,
    struct ExecBase *, SysBase, 20, Exec)
{
#undef Exec

        AROS_LIBFUNC_INIT

	/*
	 * Disable interrupts by masking all interrupts.
	 */
	SetSR(0x700,0x700);

	SysBase->IDNestCnt++;

	AROS_LIBFUNC_EXIT
} /* Disable() */
