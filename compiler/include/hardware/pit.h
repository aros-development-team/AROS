/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$
 
    Desc: IBM PC-compatible PIT (8253) specific definitions
    Lang: english
*/

#ifndef HARDWARE_PIT_H
#define HARDWARE_PIT_H

#include <asm/io.h>

#define PIT_CH0	        0x40
#define PIT_CH1	        0x41    /* This channel is likely missing       */
#define PIT_CH2	        0x42
#define PIT_CONTROL     0x43

#define BCD		0x01	/* BCD flag		                */
#define MODE_TERMINAL	0x00	/* Channel mode		                */
#define MODE_ONESHOT	0x02
#define MODE_RATE	0x04
#define MODE_SQUARE	0x06
#define MODE_SW_STROBE	0x08
#define MODE_HW_STROBE	0x0A
#define ACCESS_LATCH	0x00	/* 'Latch value' command                */
#define ACCESS_LOW	0x10	/* Access mode		                */
#define ACCESS_HI	0x20
#define ACCESS_FULL	0x30    /* 16bit access                         */
#define CH0		0x00	/* Channel selector	                */
#define CH1		0x40
#define CH2		0x80
#define READBACK	0xC0

/* Two useful macros for accessing counter values */
#define ch_read(port) inb(port) | (inb(port) << 8)
#define ch_write(val, port) outb((val) & 0xff, port); outb(((val) >> 8) & 0xff, port)

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
        outb(READBACK | 0x22, PIT_CONTROL);
    } while((inb(PIT_CH0) & 0x40) != 0);
}

/*
 * In this mode the counter wraps around to 0xFFFF and continues decrementing.
 * It's assumed that the start value is reasonably lower, and the CPU has time
 * to detect the wraparound.
 */
static inline unsigned short pit_wait(unsigned short start)
{
    unsigned short end = 0xffff - start;
    unsigned short tick;

    do
    {
        outb(CH0|ACCESS_LATCH, PIT_CONTROL);
        tick = ch_read(PIT_CH0);
    } while (tick > end);

    return 0xffff - tick;
}

void pit_udelay(unsigned int usec);

#endif
