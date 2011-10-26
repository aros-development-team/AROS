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

void pit_udelay(unsigned int usec)
{
    unsigned int start = usec2tick(usec);

    D(bug("[PIT] udelay: %d usec = %d ticks\n", usec, start));

    pit_start(start);
    pit_wait(start);
}
