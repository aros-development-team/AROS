/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Init of mathieeesingtrans.library
    Lang: english
*/

#include <aros/symbolsets.h>

#include "mathieeesingtrans_intern.h"
#include LC_LIBDEFS_FILE

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

struct Library * MathIeeeSingBasBase;

AROS_SET_LIBFUNC(Init, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT;
    
    MathIeeeSingBasBase = OpenLibrary ("mathieeesingbas.library", 39);
    if (!MathIeeeSingBasBase)
	return FALSE;

    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT;
}


AROS_SET_LIBFUNC(Expunge, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT;
    
    if (MathIeeeSingBasBase)
	CloseLibrary ((struct Library *)MathIeeeSingBasBase);
    
    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT;
}

ADD2INITLIB(Init, 0);
ADD2EXPUNGELIB(Expunge, 0);
