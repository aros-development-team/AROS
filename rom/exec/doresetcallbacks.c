/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.

    Desc: Execute installed reset handlers.
*/

#include <aros/asmcall.h>
#include <exec/interrupts.h>

#include "exec_intern.h"
#include "exec_util.h"
#include "exec_debug.h"

/*
   This function executes all installed reset handlers in sequence.
   It stores the supplied shutdown action type (SD_ACTION_#?) in the
   ln_Type field of each reset interrupt structure before invoking it.
   Typically, this information is used by system-level reset handlers
   (EFI, ACPI, etc.), but not by peripheral-device handlers (USB HCs,
   NICs, etc.). The ln_Type field also encodes whether the code is
   executing in supervisor mode.

   For improved safety, all callbacks are executed in a Disable()d state.
   This function itself does not need to call Enable().

   NOTE: This function can fail if any installed reset handler crashes
   or hangs. In such cases, the shutdown sequence may not complete,
   leaving the system in an undefined or partially reset state. This
   behavior needs to be corrected so that failure in one handler cannot
   prevent the rest of the shutdown process from completing.
*/

void Exec_DoResetCallbacks(struct IntExecBase *IntSysBase, UBYTE action)
{
    struct Interrupt *i, *tmp;

    Disable();

    ForeachNodeSafe(&IntSysBase->ResetHandlers, i, tmp) {
        DSHUTDOWN("Calling handler: %d '%s'", i->is_Node.ln_Pri, i->is_Node.ln_Name);

        i->is_Node.ln_Type = action;
        if (KrnIsSuper())
            i->is_Node.ln_Type |= 0x80; /* Set the "supervisor" flag */

        AROS_INTC1(i->is_Code, i->is_Data);
    }
}
