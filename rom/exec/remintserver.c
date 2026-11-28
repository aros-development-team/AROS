/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Remove an interrupt handler.
*/

#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <hardware/intbits.h>
#include <proto/exec.h>
#include <aros/libcall.h>

#include "exec_intern.h"
#include "exec_debug.h"
#include "chipset.h"
#include "exec_locks.h"

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
        On SMP the chain is locked while its servers run. This function
        returns once the server is off the chain and is not running on
        any other CPU. It must not be called from an interrupt server:
        that would wait for the chain the caller itself keeps locked.

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

    EXEC_LOCK_LIST_WRITE_AND_DISABLE(&SysBase->IntrList);
    
    Remove((struct Node *)interrupt);
    CUSTOM_DISABLE(intNumber, SysBase->IntVects[intNumber].iv_Data);
    
    EXEC_UNLOCK_LIST_AND_ENABLE(&SysBase->IntrList);

    AROS_LIBFUNC_EXIT
} /* RemIntServer */
