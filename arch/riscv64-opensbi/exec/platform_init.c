/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: Default power state handlers for the opensbi-riscv64 target.
*/

#include <aros/debug.h>

#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <exec/pm.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>

#include "exec_intern.h"

/* kernel and exec are linked into one core.elf, so this reaches the
   kernel directly (arch/riscv64-opensbi/kernel/kernel_startup.c) */
void krnWarmKick(void) __attribute__((noreturn));

/*
 * A warm reboot restarts AROS itself - back into the kickstart, the
 * platform left running (Amiga semantics). This sits at SD_PRI_REBOOT
 * so every device and resource above it has had its say; on an EFI
 * machine efi.resource (pri -56) declines warm resets on purpose so the
 * request falls through to here - the same division of labour as the
 * x86_64 EFI systems, where the equivalent handler re-kicks the kernel.
 */
AROS_INTH1(Exec_WarmResetHandler, struct Interrupt *, handler)
{
    AROS_INTFUNC_INIT

    UBYTE action = handler->is_Node.ln_Type & SD_ACTION_MASK;

    /* Bitwise: SD_ACTION_REBOOT lands here when no cold handler took it */
    if (action & SD_ACTION_WARMREBOOT)
        krnWarmKick();

    return FALSE;

    AROS_INTFUNC_EXIT
}

static int PlatformInit(struct ExecBase *SysBase)
{
    struct IntExecBase *sysBase = (struct IntExecBase *)SysBase;

    sysBase->WarmResetHandler.is_Node.ln_Pri = SD_PRI_REBOOT;
    sysBase->WarmResetHandler.is_Node.ln_Name = "System Reset";
    sysBase->WarmResetHandler.is_Code = (VOID_FUNC)Exec_WarmResetHandler;
    sysBase->WarmResetHandler.is_Data = &sysBase->WarmResetHandler;
    AddResetCallback(&sysBase->WarmResetHandler);

    return TRUE;
}

ADD2INITLIB(PlatformInit, 0);
