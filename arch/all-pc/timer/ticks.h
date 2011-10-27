#ifndef _TICKS_H
#define _TICKS_H

#include <exec/types.h>
#include <hardware/pit.h>

#include "timer_intern.h"

static inline void Timer0Start(UWORD delay, struct TimerBase *TimerBase)
{
    outb(CH0|ACCESS_FULL|MODE_SW_STROBE, PIT_CONTROL);  /* Software strobe mode, 16-bit access */
    ch_write(delay, PIT_CH0);

    TimerBase->tb_prev_tick = delay;			/* Remember initial ticks count for incrementing EClock value in EClockUpdate() */
}

void Timer0Setup(struct TimerBase *TimerBase);

#endif
