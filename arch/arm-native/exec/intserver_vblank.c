/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: arm-native VBlankServer.

          On SMP the scheduler quantum is expired per-core by each core's
          own ARM generic timer (CNTP) heartbeat in the kernel, so the
          VBlank only runs the registered VERTB interrupt servers here.

          On non-SMP arm-native there is no per-core timer, so the VBlank
          remains the scheduling heartbeat and expires the quantum here,
          exactly like generic Exec.
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
