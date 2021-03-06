/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.

    Desc: default x86 power state handlers
*/

#include <exec/interrupts.h>
#include <asm/io.h>

#include <proto/exec.h>

#define __AROS_KERNEL__

#include "exec_intern.h"

/* Call the kernel to perform a Cold Reset */
AROS_INTH1(Exec_X86ColdResetHandler, struct Interrupt *, handler)
{
    AROS_INTFUNC_INIT

    UBYTE action = handler->is_Node.ln_Type;

    if (action == SD_ACTION_COLDREBOOT)
    {
        krnSysCallChangePMState(0xFF);
    }

    return FALSE;

    AROS_INTFUNC_EXIT
}

/* Call the kernel to perform a Warm Reset */
AROS_INTH1(Exec_X86WarmResetHandler, struct Interrupt *, handler)
{
    AROS_INTFUNC_INIT

    UBYTE action = handler->is_Node.ln_Type;

    if (action == SD_ACTION_WARMREBOOT)
    {
        /* Tell kernel to reboot */
        __asm__ __volatile__ ("int $0xfe"::"a"(0x100));
    }

    /* We really should not return from that */
    return FALSE;

    AROS_INTFUNC_EXIT
}

/* This reset handler is called at the end of the shut down
 * chain (after the power-off screen), and calls the kernel
 * provided routine to power off the hardware if possible.
 */
AROS_INTH1(Exec_X86ShutdownHandler, struct Interrupt *, handler)
{
    AROS_INTFUNC_INIT

    SuperState();
    while (TRUE)
    {
        /*
         * Either we will forever loop looking for a
         * syscall shutdown handler, or call an appropriate one =)
         */

        krnSysCallChangePMState(0);
    };

    /* We really should not return from that */
    return FALSE;

    AROS_INTFUNC_EXIT
}
