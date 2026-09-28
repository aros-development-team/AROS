/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: DS3231 RTC on the Pi i2c header bus.
*/

#ifndef BATTCLOCK_DS3231_H
#define BATTCLOCK_DS3231_H

#include <exec/types.h>

/* FALSE when no DS3231 is described or its time is not valid. */
BOOL DS3231_Read(ULONG *secs);
BOOL DS3231_Write(ULONG secs);

#endif /* BATTCLOCK_DS3231_H */
