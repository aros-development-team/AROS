/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"
#include <aros/macros.h>
#include <libraries/locale.h>
#include <proto/locale.h>

#define DEBUG 0
#include <aros/debug.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
    struct Locale *l;

    secs = Date2Amiga(&clockdata);

    l = OpenLocale(NULL);
    if (l)
    {
	if (l->loc_Flags & LOCF_GMT_CLOCK)
	    secs += l->loc_GMTOffset * 60;

	CloseLocale(l);
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
