#ifndef KERNEL_TIMER_H
#define KERNEL_TIMER_H

#include <resources/pit.h>

/* Kernel clocksource support functions */
void kcs_udelay(unsigned int usec);

#endif /* !KERNEL_TIMER_H */