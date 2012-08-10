/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Add interrupt client to chain of interrupt server
    Lang: english
*/

#include <aros/asmcall.h>
#include <exec/interrupts.h>

#include "exec_intern.h"
#include "exec_util.h"

/* Call this function from within your ColdReboot() implementation
   in order to execute installed reset handlers.
   For improved safety callbacks are called in a Disable()d state.
   This function does not need to Enable(). */

void Exec_DoResetCallbacks(struct IntExecBase *IntSysBase)
{
    struct Interrupt *i;

    Disable();

    for (i = (struct Interrupt *)IntSysBase->ResetHandlers.lh_Head; i->is_Node.ln_Succ;
         i = (struct Interrupt *)i->is_Node.ln_Succ)
        AROS_UFIC1(i->is_Code, i->is_Data);
}
