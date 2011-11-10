/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
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
    TimerIO->tr_time.tv_secs 	= secs;
    TimerIO->tr_time.tv_micro 	= 0;
    
    DoIO(&TimerIO->tr_node);
    
    return TRUE;
}

/*********************************************************************************************/

BOOL SavePrefs(void)
{
    ULONG secs;

    secs = Date2Amiga(&clockdata);
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
