/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Nonvolatile library initialization code.
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
    AROS_SET_LIBFUNC_INIT
    
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
    
    AROS_SET_LIBFUNC_EXIT
}


AROS_SET_LIBFUNC(Expunge, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT
    
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    CloseLibrary(nvBase->nv_ImplementationLib);

    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT
}

ADD2INITLIB(Init, 0);
ADD2EXPUNGELIB(Expunge, 0);
