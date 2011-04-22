#ifndef LOWLEVEL_H_
#define LOWLEVEL_H_

#include <inttypes.h>
#include "timer_intern.h"

extern uint32_t tbc_expected;
extern uint32_t tbc_achieved;
extern int32_t corr;

void EClockUpdate(struct TimerBase *TimerBase);
void EClockSet(struct TimerBase *TimerBase);
void TimerSetup(struct TimerBase *TimerBase, uint32_t waste);

#endif /*LOWLEVEL_H_*/
