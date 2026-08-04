/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: Register map of the ST M41T11 i2c real time clock.

    Every counter is BCD, and each one occupies only the low bits of its
    register - the rest belong to the part's own control bits and must be
    masked off when reading and left alone when writing.
*/

#ifndef M41T11_H
#define M41T11_H

#include <exec/types.h>

/* The part answers on one fixed address; the device tree should agree */
#define M41T11_ADDR         0x68

#define M41T11_REG_HSECS    0x00    /* tenths/hundredths of a second    */
#define M41T11_REG_SECS     0x01
#define M41T11_REG_MINS     0x02
#define M41T11_REG_HOURS    0x03    /* 24 hour, there is no am/pm bit   */
#define M41T11_REG_WDAY     0x04    /* free running 1-7, see below      */
#define M41T11_REG_MDAY     0x05
#define M41T11_REG_MONTH    0x06
#define M41T11_REG_YEAR     0x07
#define M41T11_REG_CONTROL  0x08    /* OUT, FT, S and the calibration   */

/* The oscillator is held stopped while this is set */
#define M41T11_SECS_ST      0x80

#define M41T11_SECS_MASK    0x7f
#define M41T11_MINS_MASK    0x7f
#define M41T11_HOURS_MASK   0x3f
#define M41T11_WDAY_MASK    0x07
#define M41T11_MDAY_MASK    0x3f
#define M41T11_MONTH_MASK   0x1f

/* Seconds to year, the registers the time of day is spread over */
#define M41T11_CLOCK_REGS   7

/*
 * The part stores two digits of year and no century, so something outside
 * it has to say which hundred years those digits mean. Everything this
 * port runs on shipped after 2000, and the AROS epoch starts in 1978 so a
 * wrap back to 1900 could not be represented anyway: 00-99 is read and
 * written as 2000-2099. A machine still running in 2100 will read 2000.
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
