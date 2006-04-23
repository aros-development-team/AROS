/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stddef.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <dos/dosextens.h>
#include <aros/libcall.h>
#include <aros/debug.h>
#include <hidd/unixio.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/alib.h>

#include <aros/symbolsets.h>
#include LC_LIBDEFS_FILE

OOP_Object *unixio;
int audio_fd;

AROS_SET_LIBFUNC(InitData, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT
    
    unixio = OOP_NewObject(NULL, CLID_Hidd_UnixIO, NULL);
    if (!unixio) return FALSE;
    
    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT
}

AROS_SET_LIBFUNC(OpenLib, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT
    
    /* Allow only one opener */

    return ((struct Library *)LIBBASE)->lib_OpenCnt ? FALSE : TRUE;
    
    AROS_SET_LIBFUNC_EXIT
}

AROS_SET_LIBFUNC(CleanUp, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT
    
    if (unixio) OOP_DisposeObject(unixio);
    
    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT
}

ADD2INITLIB(InitData, 0);
ADD2OPENLIB(OpenLib, 0);
ADD2EXPUNGELIB(CleanUp, 0);
