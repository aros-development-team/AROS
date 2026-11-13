/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Elapsed E-clock time in unsigned 16.16 seconds
*/

#include <proto/timer.h>

#include "lowlevel_intern.h"

#include <aros/libcall.h>
#include <exec/types.h>
#include <devices/timer.h>
#include <libraries/lowlevel.h>

/*****************************************************************************

    NAME */

      AROS_LH1(ULONG, ElapsedTime,

/*  SYNOPSIS */
      AROS_LHA(struct EClockVal *, context, A0),

/*  LOCATION */
      struct LowLevelBase *, LowLevelBase, 17, LowLevel)

/*  FUNCTION
        Return the time since the previous call using this context, as
        unsigned 16.16 seconds. The first call establishes the context.

    INPUTS
        context - An EClockVal, initially cleared, reused for subsequent calls.

    RESULT
        Seconds in the upper sixteen bits and fractional seconds in the lower
        sixteen bits. Intervals of 65536 seconds or more saturate at 0xffffffff.

    SEE ALSO
        timer.device/ReadEClock()

    INTERNALS

*****************************************************************************/
{
  AROS_LIBFUNC_INIT

    struct Library *TimerBase = LowLevelBase->ll_TimerBase;
    struct EClockVal now;
    ULONG rate, high, low, remainder, result = 0;
    unsigned int bits = 48;

    rate = ReadEClock(&now);
    if (!rate)
        return 0;
    low = now.ev_lo - context->ev_lo;
    high = now.ev_hi - context->ev_hi - (now.ev_lo < context->ev_lo);
    *context = now;
    /* Compare against rate * 65536 before scaling, without overflowing. */
    if (high > (rate >> 16) ||
        (high == (rate >> 16) && low >= (rate << 16)))
        return ~0UL;

    remainder = high;
    if (!high && low < rate)
    {
        /* Frame timers need only the sixteen fractional quotient bits. */
        remainder = low;
        low = 0;
        bits = 16;
    }

    /* Divide the tick difference followed by sixteen zero bits by the clock
     * rate. The saturation check guarantees that the quotient fits in ULONG.
     * Using word arithmetic also avoids a 64-bit divide helper in small ROMs. */
    while (bits--)
    {
        ULONG carry = remainder >> 31;
        remainder = (remainder << 1) | (low >> 31);
        low <<= 1;
        result <<= 1;
        if (carry || remainder >= rate)
        {
            remainder -= rate;
            result |= 1;
        }
    }
    return result;

    AROS_LIBFUNC_EXIT
} /* ElapsedTime */
