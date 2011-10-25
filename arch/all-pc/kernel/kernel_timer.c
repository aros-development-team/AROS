#include <hardware/pit.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_timer.h"

#define D(x)

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
    unsigned int tick;

    D(bug("[PIT] udelay: %d usec = %d ticks\n", usec, start));

    /*
     * Start up channel 0 in 'terminal count' mode (mode 0).
     * It will count down to zero, and then will issue an interrupt,
     * but we don't care about it since interrupts are neither used nor enabled.
     * It's safe to use the timer freely here. timer.device starts up long after
     * this code.
     */
    outb(CH0|ACCESS_FULL|MODE_TERMINAL, PIT_CONTROL);
    ch_write(start, PIT_CH0);

    /*
     * In this mode the counter wraps around to 0xFFFF.
     * It's assumed that the initial value is reasonably lower,
     * and the CPU has time to detect the wraparound.
     */
    do
    {
        outb(CH0|ACCESS_LATCH, PIT_CONTROL);
        tick = ch_read(PIT_CH0);
    } while ((tick > 0) && (tick < start));

    D(bug("[PIT] udelay done\n"));
}
