/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Remove an interrupt handler.
    Lang:
*/

#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <hardware/intbits.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <aros/libcall.h>

#include "exec_debug.h"
#include "exec_intern.h"
#include "chipset.h"

/*****************************************************************************

    NAME */

	AROS_LH2(void, RemIntServer,

/*  SYNOPSIS */
	AROS_LHA(ULONG,              intNumber, D0),
	AROS_LHA(struct Interrupt *, interrupt, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 29, Exec)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    ExecLog(SysBase, EXECDEBUGF_EXCEPTHANDLER, "RemIntServer: Int %d, Interrupt %p\n", intNumber, interrupt);

    if (intNumber >= INTB_KERNEL) {
        KrnRemIRQHandler(interrupt->is_Node.ln_Succ);
        return;
    }

    Disable();

    Remove((struct Node *)interrupt);
    CUSTOM_DISABLE(intNumber, SysBase->IntVects[intNumber].iv_Data);

    Enable();

    AROS_LIBFUNC_EXIT
} /* RemIntServer */
