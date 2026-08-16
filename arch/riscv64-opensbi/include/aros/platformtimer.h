#ifndef AROS_PLATFORMTIMER_H
#define AROS_PLATFORMTIMER_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: The S-mode timer on the opensbi-riscv64 target, as shared between
          kernel.resource and timer.device (KrnGetSystemAttr(KATTR_PlatformTimer)).
*/

#include <exec/types.h>

/*
 * The kernel keeps a periodic tick on the timer. timer.device may ask
 * for one earlier wakeup: it stores the absolute time (in timebase
 * ticks, 0 = none) in kpt_Deadline and re-arms the hardware with
 * KrnPlatformTimerArm(); when that time comes the kernel runs the
 * handlers of source kpt_IRQ. Once timer.device has claimed the timer
 * (KPTF_CLAIMED) it drives INTB_VERTB itself and the kernel tick no
 * longer does. All fields are touched with interrupts disabled.
 */
struct KrnPlatformTimer
{
    volatile UQUAD  kpt_Deadline;
    volatile UQUAD  kpt_NextTick;
    ULONG           kpt_IRQ;
    ULONG           kpt_Frequency;
    volatile ULONG  kpt_Flags;
};

#define KPTF_CLAIMED    (1 << 0)

static inline UQUAD KrnPlatformTimerRead(void)
{
    UQUAD v;
    asm volatile("rdtime %0" : "=r"(v));
    return v;
}

/* Program the hardware for the earlier of the tick and the deadline */
static inline void KrnPlatformTimerArm(struct KrnPlatformTimer *kpt)
{
    register unsigned long a0 asm("a0") = kpt->kpt_NextTick;
    register unsigned long a6 asm("a6") = 0;
    register unsigned long a7 asm("a7") = 0x54494D45;   /* SBI TIME extension */

    if (kpt->kpt_Deadline && (QUAD)(kpt->kpt_Deadline - a0) < 0)
        a0 = kpt->kpt_Deadline;
    asm volatile("ecall" : "+r"(a0) : "r"(a6), "r"(a7) : "memory", "a1");
}

#endif /* !AROS_PLATFORMTIMER_H */
