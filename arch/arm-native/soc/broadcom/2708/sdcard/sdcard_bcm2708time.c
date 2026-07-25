/*
    Copyright (C) 2013-2019, The AROS Development Team. All rights reserved.
*/

#include "sdcard_intern.h"

/* Shared by whichever controller drivers are built into this device. */
APTR            MBoxBase;
APTR            DMABase;
IPTR            __arm_periiobase __attribute__((used)) = 0;

#if defined(__aarch64__)
#define NOP() asm volatile("yield\n")
#else
#define NOP() asm volatile("mov r0, r0\n")
#endif

ULONG sdcard_CurrentTime()
{
    return AROS_LE2LONG(*((volatile ULONG *)(SYSTIMER_CLO)));
}

void sdcard_Udelay(ULONG usec)
{
    ULONG now = sdcard_CurrentTime();
    do
    {
        NOP();
    } while (sdcard_CurrentTime() < (now + usec));
}

void sdcard_WaitNano(register ULONG ns, struct SDCardBase *SDCardBase)
{
    while (ns > 0)
    {
        NOP();
        --ns;
    }
}
