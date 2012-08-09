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

void Exec_DoResetCallbacks(struct IntExecBase *SysBase)
{
    struct Interrupt *i;

    Disable();

    for (i = (struct Interrupt *)SysBase->ResetHandlers.lh_Head; i->is_Node.ln_Succ;
         i = (struct Interrupt *)i->is_Node.ln_Succ)
	AROS_UFC3NR(void, i->is_Code,
		  AROS_UFCA(APTR, i->is_Data, A1),
		  AROS_UFCA(APTR, i->is_Code, A5),
		  AROS_UFCA(struct ExecBase *, &SysBase->pub, A6));
}
