#ifndef _PC_TIMER_TICKS_H
#define _PC_TIMER_TICKS_H
/*
    Copyright (C) 2011-2026, The AROS Development Team. All rights reserved.
*/

#include <exec/types.h>
#include <hardware/pit.h>

#include "timer_intern.h"

#if defined(__AROSEXEC_SMP__)
static inline ULONG PITLock(struct TimerBase *TimerBase)
{
#if defined(__x86_64__)
    ULONG flags = (ULONG)__builtin_ia32_readeflags_u64();
#else
    ULONG flags = __builtin_ia32_readeflags_u32();
#endif

    asm volatile("cli" ::: "memory");
    while (__atomic_exchange_n(&TimerBase->tb_Platform.tb_PITLock, 1, __ATOMIC_ACQUIRE))
        while (__atomic_load_n(&TimerBase->tb_Platform.tb_PITLock, __ATOMIC_RELAXED))
            asm volatile("pause");

    return flags;
}

static inline void PITUnlock(ULONG flags, struct TimerBase *TimerBase)
{
    __atomic_store_n(&TimerBase->tb_Platform.tb_PITLock, 0, __ATOMIC_RELEASE);
    if (flags & 0x200)
        asm volatile("sti" ::: "memory");
}
#else
static inline ULONG PITLock(struct TimerBase *TimerBase)
{
    return 0;
}

static inline void PITUnlock(ULONG flags, struct TimerBase *TimerBase)
{
}
#endif

static inline void Timer0Start(UWORD delay, struct TimerBase *TimerBase)
{
    ULONG flags = PITLock(TimerBase);

    outb(CH0|ACCESS_FULL|MODE_TERMINAL, PIT_CONTROL);   /* Terminal count mode, 16-bit access */
    ch_write(delay, PIT_CH0);

    TimerBase->tb_prev_tick = delay;			/* Remember initial ticks count for incrementing EClock value in EClockUpdate() */
    PITUnlock(flags, TimerBase);
}

void Timer0Setup(struct TimerBase *TimerBase);

#endif /* _PC_TIMER_TICKS_H */
