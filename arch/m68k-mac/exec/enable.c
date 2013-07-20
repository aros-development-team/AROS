/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Enable() - Allow interrupts to occur after Disable().
    Lang: english
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <asm/registers.h>

#define DEBUG 1
#include <aros/debug.h>

#undef Exec
#ifdef UseExecstubs
#    define Exec _Exec
#endif

#include <proto/exec.h>

/* See rom/exec/enable.c for documentation */

AROS_LH0(void, Enable,
    struct ExecBase *, SysBase, 21, Exec)
{
#undef Exec

        AROS_LIBFUNC_INIT

	if( --SysBase->IDNestCnt < 0) {
		/*
		 * Enable interrupt by allowing all of them.
		 */
		SetSR(0,0x700);
	}

	AROS_LIBFUNC_EXIT
} /* Enable() */
