/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Layers Resident and initialization.
    Lang: english
*/

#include "layers_intern.h"
#include LC_LIBDEFS_FILE
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <aros/symbolsets.h>

#undef GfxBase
#undef UtilityBase

#define SysBase LIBBASE->lb_SysBase

AROS_SET_LIBFUNC(LayersInit, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    InitSemaphore(&LIBBASE->lb_MemLock);
 
    LIBBASE->lb_ClipRectPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC, sizeof(struct ClipRect) * 50, sizeof(struct ClipRect) * 50);
    LIBBASE->lb_GfxBase = (struct GfxBase *) OpenLibrary("graphics.library",0);
    LIBBASE->lb_UtilityBase = (struct UtilityBase *) OpenLibrary("utility.library",0);
  
    if (!LIBBASE->lb_GfxBase || !LIBBASE->lb_UtilityBase || !LIBBASE->lb_ClipRectPool)
    {
	if (LIBBASE->lb_GfxBase)
	{
	    CloseLibrary((struct Library *)LIBBASE->lb_GfxBase);
	    LIBBASE->lb_GfxBase = NULL;
	}
	if (LIBBASE->lb_UtilityBase)
	{
	    CloseLibrary((struct Library *)LIBBASE->lb_UtilityBase);
	    LIBBASE->lb_UtilityBase = NULL;
	}
	if (LIBBASE->lb_ClipRectPool)
	{
	    DeletePool(LIBBASE->lb_ClipRectPool);
	    LIBBASE->lb_ClipRectPool = NULL;
	}
	return FALSE;
    }
  
    return TRUE;    
    AROS_SET_LIBFUNC_EXIT
}

ADD2INITLIB(LayersInit, 0);
