/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics hidd initialization code.
    Lang: English.
*/
#include <stddef.h>
#include <exec/types.h>

#include <proto/exec.h>

#include <aros/symbolsets.h>

#include "graphics_intern.h"

#include LC_LIBDEFS_FILE

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

#undef csd

static int GFX_Init(LIBBASETYPEPTR LIBBASE)
{
    struct class_static_data *csd = &LIBBASE->hdg_csd;
    
    EnterFunc(bug("GfxHIDD_Init()\n"));

    csd->cs_GfxBase = NULL;
    NEWLIST(&csd->pflist);
    InitSemaphore(&csd->sema);
    InitSemaphore(&csd->pfsema);
    InitSemaphore(&csd->rgbconvertfuncs_sem);

    ReturnInt("GfxHIDD_Init", ULONG, TRUE);
}

ADD2INITLIB(GFX_Init, -2)

