/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc: aros.library Resident and initialization.
    Lang: english
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <aros/symbolsets.h>

#include "aros_intern.h"

#include <aros/debug.h>

AROS_SET_LIBFUNC(ArosInit, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    if(!(LIBBASE->aros_utilityBase = OpenLibrary("utility.library", 37)))
	return FALSE;

    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT
}

AROS_SET_LIBFUNC(ArosExpunge, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    if(LIBBASE->aros_utilityBase != NULL)
    {
	CloseLibrary(LIBBASE->aros_utilityBase);
	LIBBASE->aros_utilityBase = NULL;
    }

    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT
}

ADD2INITLIB(ArosInit, 0);
ADD2EXPUNGELIB(ArosExpunge, 0);
