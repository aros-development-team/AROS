/*
    Copyright (C) 1995-2023, The AROS Development Team. All rights reserved.

    Desc: Execute installed reset handlers.
*/

#include <aros/asmcall.h>
#include <exec/interrupts.h>

#include "exec_intern.h"
#include "exec_util.h"
#include "exec_debug.h"

/*
   This function executes installed reset handlers.
   It stores the supplied shutdown action type (SD_ACTION_#?) in the
   ln_Type field of each reset interrupt structure. Typically this
   information is needed by system reset handlers (EFI, ACPI etc.), but
   not by peripheral-device reset handlers (USB HCs, NICs etc.).
   ln_Type field also contains information on whethere the code is
   executed in supervisor mode.
   For improved safety callbacks are called in a Disable()d state.
   This function does not need to Enable().
*/

void Exec_DoResetCallbacks(struct IntExecBase *IntSysBase, UBYTE action)
{
    struct Interrupt *i;

    Disable();

    for ( i = (struct Interrupt *)IntSysBase->ResetHandlers.lh_Head; i->is_Node.ln_Succ;
            i = (struct Interrupt *)i->is_Node.ln_Succ)
    {
        DSHUTDOWN("Calling handler: %d '%s'", i->is_Node.ln_Pri, i->is_Node.ln_Name);

        i->is_Node.ln_Type = action;
        if (KrnIsSuper())
            i->is_Node.ln_Type |= 0x80; /* Set the "supervisor" flag */

        AROS_INTC1(i->is_Code, i->is_Data);
    }
}
