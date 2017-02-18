/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Remove an interrupt handler.
    Lang:
*/

#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <hardware/intbits.h>
#include <proto/exec.h>
#include <aros/libcall.h>

#include "exec_intern.h"
#include "exec_debug.h"
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
#if defined(__AROSEXEC_SMP__)
    EXEC_SPINLOCK_LOCK(&PrivExecBase(SysBase)->IntrListSpinLock, SPINLOCK_MODE_WRITE);
#endif
    Remove((struct Node *)interrupt);
    CUSTOM_DISABLE(intNumber, SysBase->IntVects[intNumber].iv_Data);
#if defined(__AROSEXEC_SMP__)
    EXEC_SPINLOCK_UNLOCK(&PrivExecBase(SysBase)->IntrListSpinLock);
#endif
    Enable();

    AROS_LIBFUNC_EXIT
} /* RemIntServer */
