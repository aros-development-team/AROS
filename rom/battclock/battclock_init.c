/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Battery-backed up clock initialisation.
    Lang: english
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <utility/utility.h>
#include <proto/exec.h>
#include <proto/battclock.h>

#include <aros/symbolsets.h>
#include "battclock_intern.h"
#include LC_LIBDEFS_FILE

AROS_SET_LIBFUNC(GM_UNIQUENAME(Init), LIBBASETYPE, BattClockBase)
{
    AROS_SET_LIBFUNC_INIT

    BattClockBase->bb_UtilBase = OpenLibrary("utility.library", 0);

    return TRUE;

    AROS_SET_LIBFUNC_EXIT
}

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
