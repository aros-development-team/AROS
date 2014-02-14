/*
    Copyright � 1995-2014, The AROS Development Team. All rights reserved.
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

/* This reset handler is called for ColdReboot() */
AROS_INTH1(static WarmResetHandler, struct Interrupt *, handler)
{
    AROS_INTFUNC_INIT

    UBYTE action = handler->is_Node.ln_Type;

    if (action == SD_ACTION_WARMREBOOT)
    {
        /* Tell kernel to reboot */
        __asm__ __volatile__ ("int $0x80"::"a"(0x100));
    }

    /* We really should not return from that */
    return FALSE;

    AROS_INTFUNC_EXIT
}

/* This reset handler is called if software power-off or reboot has not
 * occurred. It is called after the shutdown screen is shown so that the
 * system isn't still alive in the background */
AROS_INTH1(static ShutdownHandler, struct Interrupt *, handler)
{
    AROS_INTFUNC_INIT

    SuperState();
    while (TRUE) asm volatile("hlt");

    /* We really should not return from that */
    return FALSE;

    AROS_INTFUNC_EXIT
}

/* Install handlers */
int Exec_ResetInit(struct IntExecBase *SysBase)
{
    SysBase->ColdResetHandler.is_Node.ln_Pri = -64;
    SysBase->ColdResetHandler.is_Node.ln_Name = "keyboard controller reset";
    SysBase->ColdResetHandler.is_Code = (VOID_FUNC)ColdResetHandler;
    SysBase->ColdResetHandler.is_Data = &SysBase->ColdResetHandler;
    AddResetCallback(&SysBase->ColdResetHandler);

    SysBase->WarmResetHandler.is_Node.ln_Pri = -64;
    SysBase->WarmResetHandler.is_Node.ln_Name = "kernel reset";
    SysBase->WarmResetHandler.is_Code = (VOID_FUNC)WarmResetHandler;
    SysBase->WarmResetHandler.is_Data = &SysBase->WarmResetHandler;
    AddResetCallback(&SysBase->WarmResetHandler);

    SysBase->ShutdownHandler.is_Node.ln_Pri = -128;
    SysBase->ShutdownHandler.is_Node.ln_Name = "HLT shutdown";
    SysBase->ShutdownHandler.is_Code = (VOID_FUNC)ShutdownHandler;
    SysBase->ShutdownHandler.is_Data = &SysBase->ShutdownHandler;
    AddResetCallback(&SysBase->ShutdownHandler);

    return 1;
}

ADD2INITLIB(Exec_ResetInit, 0)
