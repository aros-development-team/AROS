/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CGFX Library
    Lang: english
*/

#include <proto/exec.h>
#include <aros/symbolsets.h>
#include <utility/utility.h>

#include "cybergraphics_intern.h"
#include LC_LIBDEFS_FILE

AROS_SET_LIBFUNC(CGFXInit, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    UtilityBase = OpenLibrary (UTILITYNAME, 0);
    if (UtilityBase)
    {
    	OOPBase = OpenLibrary("oop.library", 0);
	if (OOPBase)
	{
#undef GfxBase
	    GetCGFXBase(LIBBASE)->gfxbase = (struct GfxBase *)OpenLibrary("graphics.library", 37);
	    if (GetCGFXBase(LIBBASE)->gfxbase)
	    {
		return TRUE;
	    }
	    CloseLibrary(OOPBase);
	}
	CloseLibrary(UtilityBase);
    }

    return (FALSE);

    AROS_SET_LIBFUNC_EXIT
}

ADD2INITLIB(CGFXInit, 0);
