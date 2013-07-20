/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ColdReboot() - Reboot the computer.
    Lang: english
*/

#include <aros/debug.h>

#include "exec_intern.h"
#include "exec_util.h"

extern void Exec_MagicResetCode(void);
    /* Reset everything but the CPU, then restart
     * at the ROM exception vector
     */
asm (
	"	.text\n"
	"	.align 4\n"
	"	.globl Exec_MagicResetCode\n"
	"Exec_MagicResetCode:\n"
	"	nop\n"
        "	move.l #2,%a0\n"
        "	reset\n"
        "	jmp    (%a0)\n"
     );


#include <proto/exec.h>

/* See rom/exec/coldreboot.c for documentation */

AROS_LH0(void, ColdReboot,
    struct ExecBase *, SysBase, 121, Exec)
{
    AROS_LIBFUNC_INIT

    /* Disable interrupts, and do all the reset callbacks
     */
    Exec_DoResetCallbacks((struct IntExecBase *)SysBase, SD_ACTION_WARMREBOOT);

    Supervisor((ULONG_FUNC)Exec_MagicResetCode);

    AROS_LIBFUNC_EXIT
} /* ColdReboot() */
