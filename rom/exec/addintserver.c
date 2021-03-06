/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.

    Desc: Add interrupt client to chain of interrupt servers
*/

#include <aros/debug.h>
#include <aros/libcall.h>
#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <hardware/intbits.h>
#include <proto/exec.h>

#include "exec_intern.h"
#include "exec_debug.h"
#include "chipset.h"
#include "exec_locks.h"

static void krnIRQwrapper(void *data1, void *data2)
{
    struct Interrupt *irq = (struct Interrupt *)data1;
    struct ExecBase *SysBase = (struct ExecBase *)data2;

    AROS_INTC1(irq->is_Code, irq->is_Data);
}

/*****************************************************************************

    NAME */

        AROS_LH2(void, AddIntServer,

/*  SYNOPSIS */
        AROS_LHA(ULONG,              intNumber, D0),
        AROS_LHA(struct Interrupt *, interrupt, A1),

/*  LOCATION */
        struct ExecBase *, SysBase, 28, Exec)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES
        This function also enables the corresponding chipset interrupt if
        run on a native Amiga.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    ExecLog(SysBase, EXECDEBUGF_EXCEPTHANDLER, "AddIntServer: Int %d, Interrupt %p\n", intNumber, interrupt);

    if (intNumber >= INTB_KERNEL) {
        /* N.B. ln_Succ is being re-purposed/abused here */
        interrupt->is_Node.ln_Succ = KrnAddIRQHandler(intNumber - INTB_KERNEL, krnIRQwrapper, interrupt, SysBase);
        return;
    }

    EXEC_LOCK_LIST_WRITE_AND_DISABLE(&SysBase->IntrList);

    Enqueue((struct List *)SysBase->IntVects[intNumber].iv_Data, &interrupt->is_Node);
    CUSTOM_ENABLE(intNumber);
    
    EXEC_UNLOCK_LIST_AND_ENABLE(&SysBase->IntrList);

    AROS_LIBFUNC_EXIT
} /* AddIntServer */
