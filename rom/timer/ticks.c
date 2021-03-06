/*
    Copyright (C) 1995-2011, The AROS Development Team. All rights reserved.

    Desc: Hardware management routines, template
*/

#include "timer_intern.h"

void EClockUpdate(struct TimerBase *TimerBase)
{
    /* This is called whenever timer.device wants to read EClock value from the hardware */
}

void EClockSet(struct TimerBase *TimerBase)
{
    /* This is called from SetSysTime() in order to set the new EClock value in hardware */
}
