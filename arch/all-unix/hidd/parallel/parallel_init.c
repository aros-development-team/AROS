/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Serial hidd initialization code.
    Lang: English.
*/
#include <stddef.h>
#include <exec/types.h>

#include <proto/exec.h>

#include <aros/symbolsets.h>

#include "parallel_intern.h"

#include LC_LIBDEFS_FILE

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 1
#include <aros/debug.h>

AROS_SET_LIBFUNC(UXPar_Init, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    LIBBASE->hdg_csd.utilitybase = OpenLibrary("utility.library", 37);
    if (LIBBASE->hdg_csd.utilitybase)
    {
	D(bug("  Got UtilityBase\n"));
	ReturnInt("ParallelHIDD_Init", ULONG, TRUE);
    }

    ReturnInt("ParallelHIDD_Init", ULONG, FALSE);

    AROS_SET_LIBFUNC_EXIT
}

AROS_SET_LIBFUNC(UXPar_Expunge, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    EnterFunc(bug("ParallelHIDD_Expunge()\n"));

    CloseLibrary(LIBBASE->hdg_csd.utilitybase);

    ReturnInt("ParallelHIDD_Expunge", ULONG, TRUE);
    
    AROS_SET_LIBFUNC_EXIT
}

ADD2INITLIB(UXPar_Init, 0)
ADD2EXPUNGELIB(UXPar_Expunge, 0)
