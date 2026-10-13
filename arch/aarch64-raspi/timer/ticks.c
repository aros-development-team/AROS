/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: On-demand EClock read for the BCM free-running system timer.
*/

#include <proto/exec.h>
#include <proto/execlock.h>

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

/*
 * Arm the tick compare for whichever comes first: the next periodic tick or
 * the head MICROHZ request. Without this a request only completes on the
 * 100Hz tick, so a 1ms delay took 0..10ms (measured 6-9ms on RPi3).
 */
#define TIMER_MIN_DELAY_US 10

void Timer_Reprogram(struct TimerBase *TimerBase)
{
#if defined(__AROSEXEC_SMP__)
    struct ExecLockBase *ExecLockBase = TimerBase->tb_ExecLockBase;
#endif
    ULONG delay = TimerBase->tb_Platform.tbp_TickRate.tv_micro;
    struct timerequest *tr;
    unsigned int clo, target;

    EClockUpdate(TimerBase);

#if defined(__AROSEXEC_SMP__)
    if (ExecLockBase)
        ObtainLock(TimerBase->tb_ListLock, SPINLOCK_MODE_READ, 0);
#endif
    tr = (struct timerequest *)GetHead(&TimerBase->tb_Lists[TL_MICROHZ]);
    if (tr)
    {
        struct timeval left = tr->tr_time;

        if (CMPTIME(&TimerBase->tb_Elapsed, &left) <= 0)
            delay = 0;                      /* already due */
        else
        {
            SUBTIME(&left, &TimerBase->tb_Elapsed);
            if (left.tv_secs == 0 && left.tv_micro < delay)
                delay = left.tv_micro;
        }
    }
#if defined(__AROSEXEC_SMP__)
    if (ExecLockBase)
        ReleaseLock(TimerBase->tb_ListLock, 0);
#endif

    if (delay < TIMER_MIN_DELAY_US)
        delay = TIMER_MIN_DELAY_US;

    /* The compare only matches on equality: never arm a value already behind
     * CLO, or the tick is lost until the counter wraps (~71 min). */
    do
    {
        clo = *((volatile unsigned int *)(SYSTIMER_CLO));
        target = clo + delay;
        *((volatile unsigned int *)(SYSTIMER_C0 + (TICK_TIMER * 4))) = target;
    } while ((LONG)(target - *((volatile unsigned int *)(SYSTIMER_CLO))) < 2);
}
