/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/

/*  HISTORY:  23.7.98  SDuvan  Implemented based on BattClock code. */

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <proto/exec.h>
#include <proto/misc.h>
#include <resources/misc.h>

#include <aros/symbolsets.h>
#include "misc_intern.h"

#include LC_LIBDEFS_FILE

AROS_SET_LIBFUNC(GM_UNIQUENAME(Init), LIBBASETYPE, MiscBase)
{
    AROS_SET_LIBFUNC_INIT

    InitSemaphore(&MiscBase->mb_Lock);

    return TRUE;

    AROS_SET_LIBFUNC_EXIT
}

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
