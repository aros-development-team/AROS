/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BeginIO - start a timer.device request and re-arm the tick compare
          when the request became the head of the MICROHZ queue.
*/

#include <aros/libcall.h>
#include <proto/exec.h>

#include "timer_intern.h"

AROS_LH1(void, BeginIO,
         AROS_LHA(struct timerequest *, timereq, A1),
         struct TimerBase *, TimerBase, 5, Timer)
{
    AROS_LIBFUNC_INIT

    if (common_BeginIO(timereq, TimerBase))
    {
        Disable();
        Timer_Reprogram(TimerBase);
        Enable();
    }

    AROS_LIBFUNC_EXIT
}
