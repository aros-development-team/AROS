/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
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

static int Init(LIBBASETYPEPTR LIBBASE)
{
    MathIeeeSingBasBase = OpenLibrary ("mathieeesingbas.library", 39);
    if (!MathIeeeSingBasBase)
	return FALSE;

    return TRUE;
}


static int Expunge(LIBBASETYPEPTR LIBBASE)
{
    if (MathIeeeSingBasBase)
	CloseLibrary ((struct Library *)MathIeeeSingBasBase);
    
    return TRUE;
}

ADD2INITLIB(Init, 0);
ADD2EXPUNGELIB(Expunge, 0);
