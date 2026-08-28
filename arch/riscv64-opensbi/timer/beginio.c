/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BeginIO - Start up a timer.device request.
*/

#include <aros/debug.h>

#include <aros/libcall.h>

#include "ticks.h"

AROS_LH1(void, BeginIO,
         AROS_LHA(struct timerequest *, timereq, A1),
         struct TimerBase *, TimerBase, 5, Timer)
{
    AROS_LIBFUNC_INIT

    if (common_BeginIO(timereq, TimerBase))
    {
        Disable();
        Timer0Setup(TimerBase);
        Enable();
    }

    AROS_USERFUNC_EXIT
}
