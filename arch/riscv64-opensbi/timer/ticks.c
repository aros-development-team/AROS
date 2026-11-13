/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Hardware management routines for the opensbi-riscv64 timer.

    The time CSR counts at the timebase frequency and is the EClock. The
    S-mode timer is one-shot and belongs to kernel.resource, which keeps
    its tick on it; timer.device places the next MICROHZ expiry in the
    shared descriptor and re-arms the hardware for the earlier of the two.
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/execlock.h>

#include "ticks.h"
#include "timer_macros.h"

static inline ULONG tick2usec(struct TimerBase *TimerBase, ULONG tick)
{
    return (ULONG)(((UQUAD)tick * 1000000ULL) / TimerBase->tb_eclock_rate);
}

static inline UQUAD usec2tick(struct TimerBase *TimerBase, UQUAD usec)
{
    return (usec * TimerBase->tb_eclock_rate) / 1000000ULL;
}

/* Add 'diff' EClock ticks to a timeval whose fraction of second is kept in 'frac' */
static void addticks(struct TimerBase *TimerBase, struct timeval *time, ULONG *frac, UQUAD diff)
{
    UQUAD f = *frac + diff;

    if (f >= TimerBase->tb_eclock_rate)
    {
        time->tv_secs += (ULONG)(f / TimerBase->tb_eclock_rate);
        f %= TimerBase->tb_eclock_rate;
    }
    *frac = (ULONG)f;
    time->tv_micro = tick2usec(TimerBase, (ULONG)f);
}

void EClockUpdate(struct TimerBase *TimerBase)
{
    UQUAD now = KrnPlatformTimerRead();
    UQUAD diff = now - TimerBase->tb_Platform.tb_LastTime;

    TimerBase->tb_Platform.tb_LastTime = now;
    TimerBase->tb_ticks_total += diff;
    addticks(TimerBase, &TimerBase->tb_CurrentTime, &TimerBase->tb_ticks_sec, diff);
    addticks(TimerBase, &TimerBase->tb_Elapsed, &TimerBase->tb_ticks_elapsed, diff);
}

void EClockSet(struct TimerBase *TimerBase)
{
    TimerBase->tb_ticks_sec   = (ULONG)usec2tick(TimerBase, TimerBase->tb_CurrentTime.tv_micro);
    TimerBase->tb_ticks_total = TimerBase->tb_ticks_sec +
                                (UQUAD)TimerBase->tb_CurrentTime.tv_secs * TimerBase->tb_eclock_rate;
    TimerBase->tb_Platform.tb_LastTime = KrnPlatformTimerRead();
}

/*
 * Arm the hardware for the first MICROHZ request. Called with
 * interrupts disabled, from BeginIO and from the timer interrupt.
 */
void Timer0Setup(struct TimerBase *TimerBase)
{
#if defined(__AROSEXEC_SMP__)
    struct ExecLockBase *ExecLockBase = TimerBase->tb_ExecLockBase;
#endif
    struct KrnPlatformTimer *kpt = TimerBase->tb_Platform.tb_KPT;
    struct timerequest *tr;
    UQUAD deadline = 0;

    if (!kpt)
        return;

#if defined(__AROSEXEC_SMP__)
    if (ExecLockBase)
        ObtainLock(TimerBase->tb_ListLock, SPINLOCK_MODE_READ, 0);
#endif
    if ((tr = (struct timerequest *)GetHead(&TimerBase->tb_Lists[TL_MICROHZ])) != NULL)
    {
        struct timeval time;

        time.tv_micro = tr->tr_time.tv_micro;
        time.tv_secs  = tr->tr_time.tv_secs;

        EClockUpdate(TimerBase);
        SUBTIME(&time, &TimerBase->tb_Elapsed);

        if ((LONG)time.tv_secs < 0)
            deadline = TimerBase->tb_Platform.tb_LastTime + 1;
        else
            deadline = TimerBase->tb_Platform.tb_LastTime +
                       usec2tick(TimerBase, (UQUAD)time.tv_secs * 1000000ULL + time.tv_micro);

        /* 0 means "nothing pending" to the kernel */
        if (!deadline)
            deadline = 1;
    }
#if defined(__AROSEXEC_SMP__)
    if (ExecLockBase)
        ReleaseLock(TimerBase->tb_ListLock, 0);
#endif

    kpt->kpt_Deadline = deadline;
    KrnPlatformTimerArm(kpt);
}
