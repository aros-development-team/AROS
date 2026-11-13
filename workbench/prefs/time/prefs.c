/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc:
*/

/*********************************************************************************************/

#include "global.h"
#include <aros/macros.h>

#define DEBUG 0
#include <aros/debug.h>

/*********************************************************************************************/

void InitPrefs(BOOL use, BOOL save)
{
    struct timeval tv;
    
    GetSysTime(&tv);
    Amiga2Date(tv.tv_secs, &clockdata);
    
    if (use || save) Cleanup(NULL);
}

/*********************************************************************************************/

BOOL UsePrefs(void)
{
    ULONG secs;
    
    secs = Date2Amiga(&clockdata);
    
    TimerIO->tr_node.io_Command = TR_SETSYSTIME;
    TimerIO->tr_time.tv_secs    = secs;
    TimerIO->tr_time.tv_micro   = 0;
    
    DoIO(&TimerIO->tr_node);
    
    return TRUE;
}

/*********************************************************************************************/

BOOL SavePrefs(void)
{
    ULONG secs;

    secs = Date2Amiga(&clockdata);

    if (LocaleBase)
    {
        struct Locale *locale = OpenLocale(NULL);

        if (locale)
        {
            if (locale->loc_Flags & LOCF_GMT_CLOCK)
                secs += locale->loc_GMTOffset * 60;

            CloseLocale(locale);
        }
    }

    WriteBattClock(secs);
    UsePrefs();

    return TRUE;
}

/*********************************************************************************************/

void RestorePrefs(void)
{
    InitPrefs(FALSE, FALSE);
}

/*********************************************************************************************/
