/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Init of mathtrans.library
    Lang: english
*/

#include <aros/symbolsets.h>
#include <libcore/base.h>

#include "mathtrans_intern.h"
#include LC_LIBDEFS_FILE

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

struct Library * MathBase;

AROS_SET_LIBFUNC(Init, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT;
    
    MathBase = OpenLibrary ("mathffp.library", 0);
    if (!MathBase)
	return FALSE;

    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT;
}

AROS_SET_LIBFUNC(Expunge, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT;
    
    if (MathBase)
	CloseLibrary ((struct Library *)MathBase);
    
    AROS_SET_LIBFUNC_EXIT;
}

ADD2INITLIB(Init, 0);
ADD2EXPUNGELIB(Expunge, 0);

