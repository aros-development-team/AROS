/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    Desc: Internal definitions for battclock.resource, Raspberry Pi version.
*/

#ifndef BATTCLOCK_INTERN_H
#define BATTCLOCK_INTERN_H

#include <exec/libraries.h>

/* File on the boot volume that persists the clock across reboots. */
#define BATTCLOCK_FILE "DEVS:battclock"

/* VideoCore Mailbox RTC Property Tags (Raspberry Pi 5 PMIC Hardware RTC) */
#define PROPTAG_GET_RTC                 0x00030080UL
#define PROPTAG_SET_RTC                 0x00038080UL
#define VCMB_PROPCHAN                   8

/* Seconds difference between Amiga Epoch (1978-01-01) and POSIX Epoch (1970-01-01) */
#define AMIGA_POSIX_EPOCH_DIFF          252460800UL

struct BattClockBase
{
    struct Library bb_LibNode;
};

#endif /* BATTCLOCK_INTERN_H */
