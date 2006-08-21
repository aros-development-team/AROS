/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
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

    csd->utilitybase = OpenLibrary("utility.library", 37);
    
    if (csd->utilitybase)
    {
	D(bug("  Got UtilityBase\n"));

	ReturnInt("GfxHIDD_Init", ULONG, TRUE);
    }

    ReturnInt("GfxHIDD_Init", ULONG, FALSE);
}


static int GFX_Expunge(LIBBASETYPEPTR LIBBASE)
{
    struct class_static_data *csd = &LIBBASE->hdg_csd;
    
    EnterFunc(bug("GfxHIDD_Expunge()\n"));

    CloseLibrary(csd->utilitybase);
    
    return TRUE;
}

ADD2INITLIB(GFX_Init, -1)
ADD2EXPUNGELIB(GFX_Expunge, -1)
