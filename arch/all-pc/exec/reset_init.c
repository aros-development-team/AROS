/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Install default reset handlers
    Lang: english
*/

#include <exec/interrupts.h>
#include <aros/symbolsets.h>
#include <asm/io.h>

#include <proto/exec.h>

#include "exec_intern.h"

/* This reset handler is called if more modern system reset mechanisms fail
 * or are not available */
AROS_INTH1(static ColdResetHandler, struct Interrupt *, handler)
{
    AROS_INTFUNC_INIT

    UBYTE action = handler->is_Node.ln_Type;
    if (action == SD_ACTION_COLDREBOOT)
    {
        outb(0xFE, 0x64);

        /*
         * On some machines (new PCs without a PS/2 controller), this might
         * not work. So we need to be able to return cleanly.
         */
    }

    return FALSE;

    AROS_INTFUNC_EXIT
}

/* This reset handler is called for ColdReboot(), or if all cold reset
 * handlers fail */
AROS_INTH1(static WarmResetHandler, struct Interrupt *, handler)
{
    AROS_INTFUNC_INIT

    /* Tell kernel to reboot */
    __asm__ __volatile__ ("int $0x80"::"a"(0x100));

    /* We really should not return from that */
    return FALSE;

    AROS_INTFUNC_EXIT
}

/* Install handlers */
int Exec_ResetInit(struct IntExecBase *SysBase)
{
    SysBase->ColdResetHandler.is_Node.ln_Pri = -64;
    SysBase->ColdResetHandler.is_Code = (VOID_FUNC)ColdResetHandler;
    SysBase->ColdResetHandler.is_Data = &SysBase->ColdResetHandler;
    AddResetCallback(&SysBase->ColdResetHandler);

    SysBase->WarmResetHandler.is_Node.ln_Pri = -68;
    SysBase->WarmResetHandler.is_Code = (VOID_FUNC)WarmResetHandler;
    SysBase->WarmResetHandler.is_Data = &SysBase->WarmResetHandler;
    AddResetCallback(&SysBase->WarmResetHandler);

    return 1;
}

ADD2INITLIB(Exec_ResetInit, 0)
