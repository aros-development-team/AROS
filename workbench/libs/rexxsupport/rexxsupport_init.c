/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: RexxSupport initialization code.
    Lang: English
*/

#include <exec/types.h>
#include <exec/libraries.h>
#include <dos/dosextens.h>
#include <aros/symbolsets.h>
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/alib.h>

#include "rexxsupport_intern.h"
#include LC_LIBDEFS_FILE

struct RxsLib *RexxSysBase;
struct DosLibrary *DOSBase;
int errno;

AROS_SET_LIBFUNC(Init, LIBBASETYPE, LIBBASE)
{
    NewList(&RSBI(LIBBASE)->openports);
    DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 36);
    if (DOSBase == NULL)
	return FALSE;
    RexxSysBase = (struct RxsLib *)OpenLibrary("rexxsyslib.library", 0);
    if (RexxSysBase == NULL)
    {
	CloseLibrary((struct Library *)DOSBase);
        return FALSE;
    }
    else
        return TRUE;
}

AROS_SET_LIBFUNC(Expunge, LIBBASETYPE, LIBBASE)
{
    CloseLibrary((struct Library *) RexxSysBase);
}

ADD2INITLIB(Init, 0);
ADD2EXPUNGELIB(Expunge, 0);
