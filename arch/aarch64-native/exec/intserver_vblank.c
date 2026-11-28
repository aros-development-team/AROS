/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: aarch64-native VBlankServer.

          On SMP every core, the boot core included, expires the quantum
          of its running task from its own ARM generic timer (CNTP)
          heartbeat in the kernel (bcm27xx_sched_tick()). The VBlank is
          raised on the boot core from the same tick period, so counting
          the quantum here as well would take it off that core twice per
          period. The VBlank only runs the registered VERTB interrupt
          servers here, as on arm-native.

          On non-SMP there is no per-core heartbeat, so the VBlank remains
          the scheduling heartbeat and expires the quantum here, exactly
          like generic Exec.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <aros/asmcall.h>
#include <exec/execbase.h>
#include <exec/lists.h>

#define AROS_NO_ATOMIC_OPERATIONS
#include <exec_platform.h>

#include "intservers.h"

/* VBlankServer. The same as general purpose IntServer, and on non-SMP it
 * also counts the task's quantum. */
AROS_INTH3(VBlankServer, struct List *, intList, intMask, custom)
{
    AROS_INTFUNC_INIT

    D(bug("[Exec] %s()\n", __func__));

#if !defined(__AROSEXEC_SMP__)
    /* Check if it is time for the running task to be switched away */
    {
        UWORD current = SCHEDELAPSED_GET;
        if (current)
            SCHEDELAPSED_SET(--current);

        if (current == 0)
        {
            FLAG_SCHEDQUANTUM_SET;
            FLAG_SCHEDSWITCH_SET;
        }
    }
#endif

    /* Chain to the generic routine */
    return AROS_INTC3(IntServer, intList, intMask, custom);

    AROS_INTFUNC_EXIT
}
