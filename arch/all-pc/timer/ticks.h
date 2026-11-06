#ifndef _PC_TIMER_TICKS_H
#define _PC_TIMER_TICKS_H

#include <exec/types.h>
#include <hardware/pit.h>

#include "timer_intern.h"

static inline void Timer0Start(UWORD delay, struct TimerBase *TimerBase)
{
    outb(CH0|ACCESS_FULL|MODE_TERMINAL, PIT_CONTROL);   /* Terminal count mode, 16-bit access */
    ch_write(delay, PIT_CH0);

    TimerBase->tb_prev_tick = delay;			/* Remember initial ticks count for incrementing EClock value in EClockUpdate() */
}

void Timer0Setup(struct TimerBase *TimerBase);

#endif /* _PC_TIMER_TICKS_H */
