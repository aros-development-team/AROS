/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: 64-bit read of the BCM free-running system timer.
*/

#ifndef SYSTIMER_H
#define SYSTIMER_H

#include "timer_intern.h"

/* Counter rate: its ticks are microseconds. */
#define SYSTIMER_RATE 1000000

static inline UQUAD SysTimer_Read(struct TimerBase *TimerBase)
{
    unsigned int chi, clo;

    /* CLO wraps every ~71 minutes, so CHI is re-read to be sure it did
     * not carry between the two loads. */
    do
    {
        chi = *((volatile unsigned int *)(SYSTIMER_CHI));
        clo = *((volatile unsigned int *)(SYSTIMER_CLO));
    } while (chi != *((volatile unsigned int *)(SYSTIMER_CHI)));

    return ((UQUAD)chi << 32) | clo;
}

#endif /* SYSTIMER_H */
