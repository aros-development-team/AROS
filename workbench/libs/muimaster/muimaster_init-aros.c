/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id: muimaster_init.c 19905 2003-10-04 05:24:00Z falemagn $
*/

#include <proto/exec.h>
#include <clib/alib_protos.h>

#include "muimaster_intern.h"
#include "mui.h"

#include <aros/symbolsets.h>

#include LC_LIBDEFS_FILE

struct Library *MUIMasterBase;

/****************************************************************************************/

AROS_SET_LIBFUNC(MUIMasterInit, LIBBASETYPE, lh)
{
    AROS_SET_LIBFUNC_INIT

    MUIMasterBase = lh;
    
    InitSemaphore(&MUIMB(lh)->ZuneSemaphore);
    
    NewList((struct List *)&MUIMB(lh)->BuiltinClasses);
    NewList((struct List *)&MUIMB(lh)->Applications);
    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT
}

ADD2INITLIB(MUIMasterInit, 0);

/****************************************************************************************/
