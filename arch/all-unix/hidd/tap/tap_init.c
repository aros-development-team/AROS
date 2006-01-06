/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id: tap_init.c 23803 2005-12-11 11:58:09Z verhaegs $

    Desc: Serial hidd initialization code.
    Lang: English.
*/
#include <stddef.h>
#include <exec/types.h>

#include <proto/exec.h>

#include <aros/symbolsets.h>

#include "tap_intern.h"

#include LC_LIBDEFS_FILE

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 1
#include <aros/debug.h>

AROS_SET_LIBFUNC(UXTap_Init, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    LIBBASE->hdg_csd.utilitybase = OpenLibrary("utility.library", 37);
    if (LIBBASE->hdg_csd.utilitybase)
    {
	D(bug("  Got UtilityBase\n"));
	ReturnInt("TapHIDD_Init", ULONG, TRUE);
    }

    ReturnInt("TapHIDD_Init", ULONG, FALSE);

    AROS_SET_LIBFUNC_EXIT
}

AROS_SET_LIBFUNC(UXTap_Expunge, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    EnterFunc(bug("TapHIDD_Expunge()\n"));

    CloseLibrary(LIBBASE->hdg_csd.utilitybase);

    ReturnInt("TapHIDD_Expunge", ULONG, TRUE);
    
    AROS_SET_LIBFUNC_EXIT
}

ADD2INITLIB(UXTap_Init, 0)
ADD2EXPUNGELIB(UXTap_Expunge, 0)
