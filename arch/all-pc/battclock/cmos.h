/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CMOS offsets.
    Lang: English
*/

#ifndef CMOS_H
#define CMOS_H

#define CENTURY		0x32
#define YEAR		0x09
#define MONTH		0x08
#define MDAY		0x07
#define HOUR		0x04
#define MIN		0x02
#define SEC		0x00
#define STATUS_A	0x0A
#define STATUS_B	0x0B
#define HEALTH		0x0E

static inline UBYTE ReadCMOSByte(UBYTE port)
{
    UBYTE value;

    asm volatile("outb	%0,$0x70" :: "a"(port));
    asm volatile("inb	$0x71,%0" : "=a"(value));

    return value;
}

static inline VOID WriteCMOSByte(UBYTE port, UBYTE value)
{
    asm volatile("outb	%0,$0x70" :: "a"(port));
    asm volatile("outb	%0,$0x71" :: "a"(value));

    return;
}

#endif /* CMOS_H */
