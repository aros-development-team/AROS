/*
    Copyright © 2002-2006, The AROS Development Team. 
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

static int MUIMasterInit(LIBBASETYPEPTR lh)
{
    MUIMasterBase = lh;
    
    InitSemaphore(&MUIMB(lh)->ZuneSemaphore);
    
    NewList((struct List *)&MUIMB(lh)->BuiltinClasses);
    NewList((struct List *)&MUIMB(lh)->Applications);
    return TRUE;
}

ADD2INITLIB(MUIMasterInit, 0);

/****************************************************************************************/
