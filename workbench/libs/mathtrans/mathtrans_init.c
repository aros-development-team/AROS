/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
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

struct MathBase * MathBase;

AROS_SET_LIBFUNC(Init, LIBBASETYPE, LIBBASE)
{
    MathBase = (struct MathBase *)OpenLibrary ("mathffp.library", 0);
    if (!MathBase)
	return FALSE;

    return TRUE;
}

AROS_SET_LIBFUNC(Expunge, LIBBASETYPE, LIBBASE)
{
    if (MathBase)
	CloseLibrary ((struct Library *)MathBase);
}

ADD2INITLIB(Init, 0);
ADD2EXPUNGELIB(Expunge, 0);

