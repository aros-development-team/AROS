/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Init of mathieeesingtrans.library
    Lang: english
*/

#include <aros/symbolsets.h>
#include <libcore/base.h>

#include "mathieeesingtrans_intern.h"
#include LC_LIBDEFS_FILE

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

struct MathIeeeSingBasBase * MathIeeeSingBasBase;

AROS_SET_LIBFUNC(Init, LIBBASETYPE, LIBBASE)
{
    MathIeeeSingBasBase = (struct MathIeeeSingBasBase *) OpenLibrary ("mathieeesingbas.library", 39);
    if (!MathIeeeSingBasBase)
	return FALSE;

    return TRUE;
}


AROS_SET_LIBFUNC(Expunge, LIBBASETYPE, LIBBASE)
{
    if (MathIeeeSingBasBase)
	CloseLibrary ((struct Library *)MathIeeeSingBasBase);
    
    return TRUE;
}

ADD2INITLIB(Init, 0);
ADD2EXPUNGELIB(Expunge, 0);
