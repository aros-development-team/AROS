/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "sdcard_intern.h"

ULONG sdcard_CurrentTime()
{
    return *((volatile ULONG *)(SYSTIMER_CLO));
}

void sdcard_Udelay(ULONG usec)
{
    ULONG now = sdcard_CurrentTime();
    do
    {
        asm volatile("mov r0, r0\n");
    } while (sdcard_CurrentTime() < (now + usec));
}

void sdcard_WaitNano(register ULONG ns, struct SDCardBase *SDCardBase)
{
    while (ns > 0)
    {
	asm volatile("mov r0, r0\n");
	--ns;
    }
}
