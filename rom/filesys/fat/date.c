/*
 * fat-handler - FAT12/16/32 filesystem handler
 *
 * Copyright © 2006 Marek Szyprowski
 * Copyright © 2007-2015 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id$
 */

#include <proto/utility.h>

#include "fat_fs.h"
#include "fat_protos.h"

#define DEBUG DEBUG_MISC
#include "debug.h"

void ConvertFATDate(UWORD date, UWORD time, struct DateStamp *ds,
    struct Globals *glob)
{
    ULONG year, month, day, hours, mins, secs;
    struct ClockData clock_data;

    /* Date bits: yyyy yyym mmmd dddd */
    year = (date & 0xfe00) >> 9;    /* Bits 15-9 */
    month = (date & 0x01e0) >> 5;   /* bits 8-5 */
    day = date & 0x001f;            /* Bits 4-0 */

    /* Time bits: hhhh hmmm mmms ssss */
    hours = (time & 0xf800) >> 11;  /* Bits 15-11 */
    mins = (time & 0x07e0) >> 5;    /* Bits 10-5 */
    secs = time & 0x001f;           /* Bits 4-0 */

    D(bug("[fat] converting fat date: year %d month %d day %d hours %d"
        " mins %d secs %d\n", year, month, day, hours, mins, secs));

    if (month < 1 || month > 12 || day < 1 || day > 31 || hours > 23 ||
        mins > 59 || secs > 29)
    {
        D(bug("[fat] invalid fat date: using 01-01-1978 instead\n"));
        secs = 0;
    }
    else
    {
        clock_data.year = 1980 + year;
        clock_data.month = month;
        clock_data.mday = day;
        clock_data.hour = hours;
        clock_data.min = mins;
        clock_data.sec = secs << 1;
        secs = Date2Amiga(&clock_data);
    }

    /* Calculate days since 1978-01-01 (DOS epoch) */
    ds->ds_Days = secs / (60 * 60 * 24);

    /* Minutes since midnight */
    ds->ds_Minute = secs / 60 % (24 * 60);

    /* 1/50 sec ticks since last minute */
    ds->ds_Tick = secs % 60 * TICKS_PER_SECOND;

    D(bug("[fat] converted fat date: days %ld minutes %ld ticks %ld\n",
        ds->ds_Days, ds->ds_Minute, ds->ds_Tick));
}

void ConvertDOSDate(struct DateStamp *ds, UWORD * date, UWORD * time,
    struct Globals *glob)
{
    ULONG year, month, day, hours, mins, secs;
    struct ClockData clock_data;

    /* Convert datestamp to seconds since 1978 */
    secs = ds->ds_Days * 60 * 60 * 24 + ds->ds_Minute * 60
        + ds->ds_Tick / TICKS_PER_SECOND;

    /* Round up to next even second because of FAT's two-second granularity */
    secs = (secs & ~1) + 2;

    /* Convert seconds since 1978 to calendar/time data */
    Amiga2Date(secs, &clock_data);

    /* Get values used in FAT dates */
    year = clock_data.year - 1980;
    month = clock_data.month - 0;
    day = clock_data.mday;
    hours = clock_data.hour;
    mins = clock_data.min;
    secs = clock_data.sec >> 1;

    /* All that remains is to bit-encode the whole lot */

    /* Date bits: yyyy yyym mmmd dddd */
    *date = (((ULONG) year) << 9) | (((ULONG) month) << 5) | day;

    /* Time bits: hhhh hmmm mmms ssss */
    *time = (((ULONG) hours) << 11) | (((ULONG) mins) << 5) | secs;
}

