/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Layers Resident and initialization.
    Lang: english
*/

#include "layers_intern.h"
#include LC_LIBDEFS_FILE
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <aros/symbolsets.h>

AROS_SET_LIBFUNC(LayersInit, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    InitSemaphore(&LIBBASE->lb_MemLock);
 
    LIBBASE->lb_ClipRectPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC, sizeof(struct ClipRect) * 50, sizeof(struct ClipRect) * 50);
    if (!LIBBASE->lb_ClipRectPool)
	return FALSE;
  
    return TRUE;    
    AROS_SET_LIBFUNC_EXIT
}

ADD2INITLIB(LayersInit, 0);
