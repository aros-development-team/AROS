/*
 * Copyright (C) 2012, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <exec/execbase.h>
#include <hardware/intbits.h>
#include <proto/exec.h>
 
#include <kernel_base.h>
#include <kernel_intr.h>
#include <kernel_scheduler.h>
#include <kernel_syscall.h>

/* Called on leaveing the interrupt.
 * This function returns TRUE if the task scheduler is needed.
 */
BOOL core_ExitIntr(VOID)
{
    /* Soft interrupt requested? It's high time to do it */
    if (SysBase->SysFlags & SFF_SoftInt)
        core_Cause(INTB_SOFTINT, 1L << INTB_SOFTINT);

    /* If task switching is disabled, do nothing */
    if (SysBase->TDNestCnt < 0)
    {
        /*
         * Do not disturb task if it's not necessary. 
         * Reschedule only if switch pending flag is set. Exit otherwise.
         */
        if (SysBase->AttnResched & ARF_AttnSwitch)
        {
            /* Run task scheduling sequence */
            return TRUE;
        }
    }

    return FALSE;
}


