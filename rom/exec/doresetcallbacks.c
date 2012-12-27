/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Execute installed reset handlers.
    Lang: english
*/

#include <aros/asmcall.h>
#include <exec/interrupts.h>

#include "exec_intern.h"
#include "exec_util.h"

/*
   This function executes installed reset handlers.
   It stores the supplied shutdown action type (SD_ACTION_#?) in the
   ln_Type field of each reset interrupt structure. Typically this
   information is needed by system reset handlers (EFI, ACPI etc.), but
   not by peripheral-device reset handlers (USB HCs, NICs etc.).
   For improved safety callbacks are called in a Disable()d state.
   This function does not need to Enable().
*/

void Exec_DoResetCallbacks(struct IntExecBase *IntSysBase, UBYTE action)
{
    struct Interrupt *i;

    Disable();

    for (i = (struct Interrupt *)IntSysBase->ResetHandlers.lh_Head; i->is_Node.ln_Succ;
         i = (struct Interrupt *)i->is_Node.ln_Succ)
    {
        i->is_Node.ln_Type = action;
        AROS_INTC1(i->is_Code, i->is_Data);
    }
}
