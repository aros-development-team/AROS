/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$
 
    Desc: IBM PC-compatible PIT (8253) specific definitions
    Lang: english
*/

#ifndef HARDWARE_PIT_H
#define HARDWARE_PIT_H

#include <asm/io.h>

/* PIT IO ports */
#define PIT_CH0	        0x40
#define PIT_CH1	        0x41    /* This channel is likely missing       */
#define PIT_CH2	        0x42
#define PIT_CONTROL     0x43

/* PIT control & status byte flags .. */
#define BCD		(1 << 0)	/* BCD flag		                */

#define MODE_TERMINAL	(0 << 1)	/* Channel mode		                */
#define MODE_ONESHOT	(1 << 1)
#define MODE_RATE	(2 << 1)
#define MODE_SQUARE	(3 << 1)
#define MODE_SW_STROBE	(4 << 1)
#define MODE_HW_STROBE	(5 << 1)
#define MODE_RATE2	(6 << 1)
#define MODE_SQUARE2	(7 << 1)

#define ACCESS_LATCH	(0 << 4)	/* 'Latch value' command                */
#define ACCESS_LOW	(1 << 4)	/* Access mode		                */
#define ACCESS_HI	(2 << 4)
#define ACCESS_FULL	(3 << 4)        /* 16bit access                         */

/* PIT control selector flags .. */
#define CH0		(0 << 6)        /* Channel selector	                */
#define CH1		(1 << 6)
#define CH2		(2 << 6)

/* Readback operation flags */
#define RB_CH0		(1 << 1)        /* Channel selector	                */
#define RB_CH1		(1 << 2)
#define RB_CH2		(1 << 3)
#define LATCH_STATUS	(1 << 4)
#define LATCH_COUNT	(1 << 5)
#define READBACK	(3 << 6)

/* PIT status byte flags .. */
#define RBS_NULLCOUNT   (1 << 6)
#define RBS_OUTPUT      (1 << 7)

/* Two useful macros for accessing counter values */
static inline unsigned short ch_read(unsigned short port)
{
    unsigned char val = inb(port);
    return (val | (inb(port) << 8));
}

static inline void ch_write(unsigned short val, unsigned short port)
{
    outb((val) & 0xFF, port);
    outb(((val) >> 8) & 0xFF, port);
}

/*
 * Start up channel 0 in 'terminal count' mode (mode 0).
 * It will count down to zero, and then will issue a single interrupt,
 * but we don't care about it since interrupts are neither used nor enabled.
 * It's safe to use the timer freely here. timer.device starts up long after
 * this code.
 * 
 * The timer is started with initial count of 0xffff and will count down to zero
 * Use pit_wait for a correct delay!
 */
static inline void pit_start(unsigned short start)
{
    (void)start;
    outb(CH0|ACCESS_FULL|MODE_TERMINAL, PIT_CONTROL);
    ch_write(0xffff, PIT_CH0);

    /* Wait until the counter loaded new value and really started to count */
    do {
        outb(READBACK | (LATCH_COUNT | RB_CH0), PIT_CONTROL);
    } while((inb(PIT_CH0) & RBS_NULLCOUNT) != 0);
}

/*
 * In this mode the counter wraps around to 0xFFFF and continues decrementing.
 * It's assumed that the start value is reasonably lower, and the CPU has time
 * to detect the wraparound.
 */
static inline unsigned short pit_wait(unsigned short time)
{
    unsigned short last_tick = 0;
    unsigned short tick = 0;
    unsigned short elapsed = time;
    unsigned short delta = 0;

    outb(CH0|ACCESS_LATCH, PIT_CONTROL);
    last_tick = ch_read(PIT_CH0);

    do
    {
        outb(CH0|ACCESS_LATCH, PIT_CONTROL);
        tick = ch_read(PIT_CH0);
        delta = last_tick - tick;
        last_tick = tick;

        if (delta < elapsed) {
            elapsed -= delta;
            asm volatile("pause"); /* Tell CPU we are spinning! */
        }
        else {
            delta -= elapsed;
            elapsed = 0;
        }
    } while (elapsed != 0);

    return time + delta;
}

#endif /* !HARDWARE_PIT_H */
