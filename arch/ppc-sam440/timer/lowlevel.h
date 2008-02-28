#ifndef LOWLEVEL_H_
#define LOWLEVEL_H_

#include "timer_intern.h"

void EClockUpdate(struct TimerBase *TimerBase);
void EClockSet(struct TimerBase *TimerBase);
void TimerSetup(struct TimerBase *TimerBase);

#endif /*LOWLEVEL_H_*/
