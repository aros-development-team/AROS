/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <proto/dos.h>

#include <stdarg.h>
#include <stdio.h>

#include "ia_log.h"

#define LOGFILE           "T:InstallAROS.log"

static BPTR     logFile;
static BOOL     logTried;
static struct DateStamp logStart;

static void LOG_Open(void)
{
    TEXT date[LEN_DATSTRING], time[LEN_DATSTRING];
    struct DateTime dt;

    logTried = TRUE;

    if (!(logFile = Open(LOGFILE, MODE_NEWFILE)))
        return;

    DateStamp(&logStart);

    dt.dat_Stamp = logStart;
    dt.dat_Format = FORMAT_DOS;
    dt.dat_Flags = 0;
    dt.dat_StrDay = NULL;
    dt.dat_StrDate = date;
    dt.dat_StrTime = time;

    if (DateToStr(&dt))
        FPrintf(logFile, "InstallAROS log, started %s %s\n\n", date, time);
    else
        FPrintf(logFile, "InstallAROS log\n\n");
}

void LOG_Printf(CONST_STRPTR fmt, ...)
{
    TEXT buf[512];
    struct DateStamp now;
    LONG secs;
    va_list ap;

    if (!logTried)
        LOG_Open();
    if (!logFile)
        return;

    DateStamp(&now);
    secs = ((now.ds_Days - logStart.ds_Days) * 24 * 60
            + (now.ds_Minute - logStart.ds_Minute)) * 60
            + (now.ds_Tick - logStart.ds_Tick) / TICKS_PER_SECOND;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), (const char *)fmt, ap);
    va_end(ap);

    FPrintf(logFile, "[%4ld] %s\n", secs, (IPTR)buf);

    /* Flushed per line: an install that dies mid-copy is exactly the one
       whose log matters */
    Flush(logFile);
}

void LOG_Close(void)
{
    if (logFile)
    {
        Close(logFile);
        logFile = BNULL;
    }
}
