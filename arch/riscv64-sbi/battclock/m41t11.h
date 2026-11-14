/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: Register map of the ST M41T11 i2c real time clock.

    Every counter is BCD, and each one occupies only the low bits of its
    register - the rest belong to the part's own control bits and must be
    masked off when reading and left alone when writing.

    The map below is Table 10 of the datasheet. This part has no
    fractions-of-a-second register; relatives that do (the M41T56, the
    M48T family) shift everything after address 0 down by one.
*/

#ifndef M41T11_H
#define M41T11_H

#include <exec/types.h>

/* The part answers on one fixed address; the device tree should agree */
#define M41T11_ADDR         0x68

#define M41T11_REG_SECS     0x00    /* ST sits on top of the seconds    */
#define M41T11_REG_MINS     0x01
#define M41T11_REG_HOURS    0x02    /* 24 hour, with the century bits   */
#define M41T11_REG_WDAY     0x03    /* free running 1-7, see below      */
#define M41T11_REG_MDAY     0x04
#define M41T11_REG_MONTH    0x05
#define M41T11_REG_YEAR     0x06
#define M41T11_REG_CONTROL  0x07    /* OUT, FT, S and the calibration   */
/* 0x08 to 0x3f is the 56 bytes of general purpose RAM */

/* The oscillator is held stopped while this is set */
#define M41T11_SECS_ST      0x80

/* The century bits share the hours register. CEB makes CB toggle at the
   turn of the century; both are left clear, since the century is fixed
   below, which keeps the register a plain hours count */
#define M41T11_HOURS_CEB    0x80
#define M41T11_HOURS_CB     0x40

#define M41T11_SECS_MASK    0x7f
#define M41T11_MINS_MASK    0x7f
#define M41T11_HOURS_MASK   0x3f
#define M41T11_WDAY_MASK    0x07
#define M41T11_MDAY_MASK    0x3f
#define M41T11_MONTH_MASK   0x1f

/* Seconds to year, the registers the time of day is spread over */
#define M41T11_CLOCK_REGS   7

/*
 * Two digits of year and no century, so 00-99 is read and written as
 * 2000-2099; the AROS epoch starts in 1978, so 1900 could not be
 * represented anyway. A machine still running in 2100 will read 2000.
 */
#define M41T11_CENTURY      2000

static inline UBYTE m41t11_FromBCD(UBYTE v)
{
    return ((v >> 4) & 0x0f) * 10 + (v & 0x0f);
}

static inline UBYTE m41t11_ToBCD(UBYTE v)
{
    return ((v / 10) << 4) | (v % 10);
}

#endif /* M41T11_H */
