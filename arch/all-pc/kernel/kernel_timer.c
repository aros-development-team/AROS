/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <hardware/pit.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_timer.h"

#define D(x)

/* See timer/ticks.c for the explanation */
static const unsigned int timer_rpr = 3599597124UL;

static inline unsigned int usec2tick(unsigned int usec)
{
    unsigned int ret = 0;

    asm volatile("divl %2":"+a"(ret),"+d"(usec):"m"(timer_rpr));
    return ret;
}

void krnClockSourceInit(void)
{
    D(bug("[Krn] %s()\n", __func__));

    if (KernelBase->kb_ClockSource)
    {
#if (0)
#endif
    }
}

void krnClockSourceUdelay(unsigned int usec)
{
    unsigned int start = usec2tick(usec);

    D(bug("[Krn] %s: %d usec = %d ticks\n", __func__, usec, start));

    if (KernelBase->kb_ClockSource)
    {
        APTR CSBase  = KernelBase->kb_ClockSource;
    }
#if (1)
    pit_wait(start);
#endif
}
