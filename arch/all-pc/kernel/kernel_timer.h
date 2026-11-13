#ifndef KERNEL_TIMER_H
#define KERNEL_TIMER_H

#include <resources/clocksource.h>

/* Kernel clocksource support functions */
void krnClockSourceInit(void);
void krnClockSourceUdelay(unsigned int usec);

#endif /* !KERNEL_TIMER_H */