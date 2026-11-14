/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: The only thing battclock.resource needs from an i2c bus.

    These three calls are everything the resource asks of a bus, and
    battclock_i2c.c is the only place that knows which driver answers
    them: nothing that knows about the RTC knows about i2c at all.
*/

#ifndef BATTCLOCK_I2C_H
#define BATTCLOCK_I2C_H

#include "battclock_intern.h"

/*
 * Claim the controller the device tree said the RTC hangs off, which
 * bb_BusBase names. Sets bb_Bus, or returns FALSE and leaves it NULL -
 * the "no RTC" path everything else already copes with. bb_Bus is also
 * cleared later if the part turns out not to answer, so a caller has to
 * keep checking it rather than only asking once.
 */
BOOL BattClock_I2COpen(struct BattClockBase *BattClockBase);

/*
 * Write wlen bytes, then a repeated START, then read rlen bytes - how
 * the RTC is told which register a read is to begin at.
 */
BOOL BattClock_I2CWriteRead(struct BattClockBase *BattClockBase,
                            const UBYTE *wbuf, ULONG wlen,
                            UBYTE *rbuf, ULONG rlen);

/* Write wlen bytes and STOP */
BOOL BattClock_I2CWrite(struct BattClockBase *BattClockBase,
                        const UBYTE *wbuf, ULONG wlen);

#endif /* BATTCLOCK_I2C_H */
