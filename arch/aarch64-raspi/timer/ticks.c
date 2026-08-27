/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: On-demand EClock read for the BCM free-running system timer.
*/

#include <proto/exec.h>

#include "timer_intern.h"
#include "timer_macros.h"

/*
 * Bring the clock up to date from the hardware. Called under Disable() from
 * GetSysTime(), GetUpTime() and ReadEClock(). Without it the clock only moved
 * on the periodic tick, so every time source had that resolution - 10ms at
 * the default 100Hz.
 *
 * The tick handler reads the same counter, and both paths advance
 * tbp_CHI/tbp_CLO past what they consumed, so no interval is counted twice.
 */
void EClockUpdate(struct TimerBase *TimerBase)
{
    unsigned int chi, clo;
    UQUAD now, last, delta;
    struct timeval tv;

    /* CLO wraps every ~71 minutes, so CHI is re-read to be sure it did
     * not carry between the two loads. */
    do
    {
        chi = *((volatile unsigned int *)(SYSTIMER_CHI));
        clo = *((volatile unsigned int *)(SYSTIMER_CLO));
    } while (chi != *((volatile unsigned int *)(SYSTIMER_CHI)));

    now  = ((UQUAD)chi << 32) | clo;
    last = ((UQUAD)TimerBase->tb_Platform.tbp_CHI << 32)
         | TimerBase->tb_Platform.tbp_CLO;

    delta = now - last;
    if (!delta)
        return;

    TimerBase->tb_Platform.tbp_CHI = chi;
    TimerBase->tb_Platform.tbp_CLO = clo;

    /* The counter runs at 1MHz, so its ticks are microseconds. */
    tv.tv_secs  = delta / 1000000;
    tv.tv_micro = delta % 1000000;

    ADDTIME(&TimerBase->tb_CurrentTime, &tv);
    ADDTIME(&TimerBase->tb_Elapsed, &tv);
}

void EClockSet(struct TimerBase *TimerBase)
{
    /* Nothing to program: the counter is read-only and free-running, and
       SetSysTime()'s value lives in tb_CurrentTime. */
}
