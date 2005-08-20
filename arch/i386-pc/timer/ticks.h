#ifndef _TICKS_H
#define _TICKS_H

#include <exec/types.h>
#include "timer_intern.h"

ULONG tick2usec(ULONG tick);
ULONG usec2tick(ULONG usec);
void EClockUpdate(struct TimerBase *TimerBase);
void EClockSet(struct TimerBase *TimerBase);

void Timer0Setup(struct TimerBase *TimerBase);

#endif
