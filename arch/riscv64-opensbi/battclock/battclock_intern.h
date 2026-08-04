/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: Internal data structures for the opensbi battclock.resource
*/

#ifndef BATTCLOCK_INTERN_H
#define BATTCLOCK_INTERN_H

/*
 * rom/battclock builds its own functions against this header as well, and
 * it does so without the include paths this directory adds, so nothing
 * beyond the base includes may be pulled in here.
 */
#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/semaphores.h>

struct BattClockBase
{
    struct Library          bb_LibNode;
    struct SignalSemaphore  bb_Lock;    /* one i2c transaction at a time  */

    /*
     * What the device tree said about the RTC. bb_Bus is the i2c
     * controller driver, held open for as long as we mean to use its
     * class; it stays NULL when the machine describes no RTC we can
     * reach, and every function then falls back to bb_Time.
     */
    APTR                    bb_Bus;
    UQUAD                   bb_BusBase; /* the controller's own registers */
    UWORD                   bb_Addr;    /* the RTC's slave address        */

    /*
     * The i2c HIDD objects, made on first use rather than at init -
     * see battclock_i2c.c for why they cannot exist any earlier.
     * Really OOP_Object *, kept as APTR so this header stays free of
     * anything rom/battclock cannot include.
     */
    APTR                    bb_I2CBus;  /* the controller as a hidd.i2c   */
    APTR                    bb_I2CDev;  /* the RTC on it                  */

    /*
     * The last value written. Without an RTC this is all the resource has
     * to answer with, which at least keeps the time consistent within one
     * session - the behaviour this port had before the RTC was supported.
     */
    ULONG                   bb_Time;
};

#endif /* BATTCLOCK_INTERN_H */
