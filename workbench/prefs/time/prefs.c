/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"
#include <aros/macros.h>

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

BOOL SavePrefs(void)
{
 
}

/*********************************************************************************************/

void RestorePrefs(void)
{
}

/*********************************************************************************************/
