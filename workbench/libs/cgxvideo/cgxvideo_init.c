/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CGXVideo Library
    Lang: english
*/

#include <proto/exec.h>
#include <aros/symbolsets.h>
#include <utility/utility.h>

#include "cgxvideo_intern.h"
#include LC_LIBDEFS_FILE

#define DEBUG 1
#include <aros/debug.h>

AROS_SET_LIBFUNC(CGXVInit, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    UtilityBase = OpenLibrary (UTILITYNAME, 0);
    if (UtilityBase)
    {
    	OOPBase = OpenLibrary("oop.library", 0);
	if (OOPBase)
	{
#undef GfxBase
	    GetCGXVBase(LIBBASE)->gfxbase = (struct GfxBase *)OpenLibrary("graphics.library", 37);
	    if (GetCGXVBase(LIBBASE)->gfxbase)
	    {
		aros_print_not_implemented("Most of this library");
		return TRUE;
	    }
	    CloseLibrary(OOPBase);
	}
	CloseLibrary(UtilityBase);
    }

    return (FALSE);

    AROS_SET_LIBFUNC_EXIT
}

ADD2INITLIB(CGXVInit, 0);
