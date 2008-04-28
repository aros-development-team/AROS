#ifndef LOWLEVEL_H_
#define LOWLEVEL_H_

#include <inttypes.h>
#include "timer_intern.h"

void EClockUpdate(struct TimerBase *TimerBase);
void EClockSet(struct TimerBase *TimerBase);
void TimerSetup(struct TimerBase *TimerBase, uint32_t waste);

#endif /*LOWLEVEL_H_*/
