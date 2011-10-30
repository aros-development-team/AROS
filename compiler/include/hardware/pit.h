/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
 
    Desc: IBM PC-compatible PIT (8253) specific definitions
    Lang: english
*/

#ifndef HARDWARE_PIT_H
#define HARDWARE_PIT_H

#include <asm/io.h>

#define PIT_CH0	    0x40
#define PIT_CH1	    0x41 /* This channel is likely missing */
#define PIT_CH2	    0x42
#define PIT_CONTROL 0x43

#define BCD		0x01	/* BCD flag		 */
#define MODE_TERMINAL	0x00	/* Channel mode		 */
#define MODE_ONESHOT	0x02
#define MODE_RATE	0x04
#define MODE_SQUARE	0x06
#define MODE_SW_STROBE	0x08
#define MODE_HW_STROBE	0x0A
#define ACCESS_LATCH	0x00	/* 'Latch value' command */
#define ACCESS_LOW	0x10	/* Access mode		 */
#define ACCESS_HI	0x20
#define ACCESS_FULL	0x30
#define CH0		0x00	/* Channel selector	 */
#define CH1		0x40
#define CH2		0x80
#define READBACK	0xC0

/* Two useful macros for accessing counter values */
#define ch_read(port) inb(port) | (inb(port) << 8)
#define ch_write(val, port) outb((val) & 0xff, port); outb(((val) >> 8) & 0xff, port)

#endif
