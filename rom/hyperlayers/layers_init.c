/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Layers Resident and initialization.
    Lang: english
*/

#include LC_LIBDEFS_FILE
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <aros/symbolsets.h>

#include "layers_intern.h"

static int LayersInit(LIBBASETYPEPTR LIBBASE)
{
    GfxBase = (APTR)OpenLibrary("graphics.library", 41);
    if (GfxBase == NULL)
        return FALSE;

    UtilityBase = (APTR)OpenLibrary("utility.library", 0);
    if (UtilityBase == NULL) {
        CloseLibrary(GfxBase);
        return FALSE;
    }

    InitSemaphore(&LIBBASE->lb_MemLock);
 
    LIBBASE->lb_ClipRectPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC, sizeof(struct ClipRect) * 50, sizeof(struct ClipRect) * 50);
    if (!LIBBASE->lb_ClipRectPool)
	return FALSE;
  
    return TRUE;    
}

ADD2INITLIB(LayersInit, 0);
