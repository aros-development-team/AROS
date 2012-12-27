/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
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


/*****************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH0(void, ColdReboot,

/*  LOCATION */
	struct ExecBase *, SysBase, 121, Exec)

/*  FUNCTION
	This function will reboot the computer.

    INPUTS
	None.

    RESULT
	This function does not return.

    NOTES
	It can be quite harmful to call this function. It may be possible that
	you will lose data from other tasks not having saved, or disk buffers
	not being flushed. Plus you could annoy the (other) users.

    EXAMPLE

    BUGS

    SEE ALSO

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Disable interrupts, and do all the reset callbacks
     */
    Exec_DoResetCallbacks((struct IntExecBase *)SysBase, SD_ACTION_WARMREBOOT);

    Supervisor((ULONG_FUNC)Exec_MagicResetCode);

    AROS_LIBFUNC_EXIT
} /* ColdReboot() */
