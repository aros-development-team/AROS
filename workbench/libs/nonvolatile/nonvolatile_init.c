/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Nonvolatile library initialization code.
    Lang: English
*/


#define DEBUG 1
#include <aros/debug.h>

#include <exec/types.h>
#include <exec/resident.h>
#include <proto/exec.h>
#include <devices/timer.h>
#include <aros/symbolsets.h>

#include <exec/libraries.h>
#include <exec/alerts.h>
#include LC_LIBDEFS_FILE

#include "nonvolatile_intern.h"

AROS_SET_LIBFUNC(Init, LIBBASETYPE, LIBBASE)
{
    /* This function is single-threaded by exec by calling Forbid. */

    D(bug("Opening implementation library (NVDisk)\n"));

    // Should be able to select this one...
    nvBase->nv_ImplementationLib = OpenLibrary("nvdisk.library", 41);

    if(nvBase->nv_ImplementationLib == NULL)
    {
	return FALSE;
    }

    D(bug("Nonvolatile library successfully inialized\n"));

    return TRUE;
}


AROS_SET_LIBFUNC(Expunge, LIBBASETYPE, LIBBASE)
{
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    CloseLibrary(nvBase->nv_ImplementationLib);

    return TRUE;
}

ADD2INITLIB(Init, 0);
ADD2EXPUNGELIB(Expunge, 0);
