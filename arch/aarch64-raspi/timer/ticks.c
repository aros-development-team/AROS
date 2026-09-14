/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: On-demand EClock read for the BCM free-running system timer.
*/

#include <proto/exec.h>

#include "timer_intern.h"
#include "timer_macros.h"

/*
 * Bring the clock up to date from the hardware, so time sources are not
 * limited to the periodic tick's resolution.
 *
 * Disable() only masks the local core, so several cores can be in here at
 * once and the booked-up-to point must be claimed atomically - as two
 * separate words it could be read torn, giving a 'last' ahead of 'now'
 * and underflowing the subtraction below into a delta of centuries. One
 * 64-bit word claimed by CAS: only the winner books its own interval.
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

    now = ((UQUAD)chi << 32) | clo;

    do
    {
        last = TimerBase->tb_Platform.tbp_EClockLast;

        /* Someone else booked past here - nothing of ours to add. */
        if (now <= last)
            return;
    } while (!__sync_bool_compare_and_swap(&TimerBase->tb_Platform.tbp_EClockLast,
                                          last, now));

    delta = now - last;

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
