/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function FindDisplayInfo()
    Lang: english
*/
#include <aros/debug.h>
#include <proto/graphics.h>
#include <graphics/displayinfo.h>
#include <hidd/graphics.h>
#include "dispinfo.h"
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH1(DisplayInfoHandle, FindDisplayInfo,

/*  SYNOPSIS */
        AROS_LHA(ULONG, ID, D0),

/*  LOCATION */
        struct GfxBase *, GfxBase, 121, Graphics)

/*  FUNCTION
	Search for a DisplayInfo which matches the ID key.

    INPUTS
        ID - identifier

    RESULT
        handle - handle to a displayinfo record with that key
                 or NULL if no match

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        graphics/displayinfo.h

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    DisplayInfoHandle ret = NULL;
    HIDDT_ModeID hiddmode;
    OOP_Object *sync, *pixfmt;
    
    D(bug("FindDisplayInfo(id=%x)\n", ID));
    
    /* Check for the NOTNULLMASK */
    if ((ID & NOTNULLMASK) != NOTNULLMASK) {
    	D(bug("!!! NO AROS MODEID IN FindDisplayInfo() !!!\n"));
    	return NULL;
    }
    
    hiddmode = AMIGA_TO_HIDD_MODEID(ID);
    
    /* Try to get mode info for the mode */
    if (!HIDD_Gfx_GetMode(SDD(GfxBase)->gfxhidd, hiddmode, &sync, &pixfmt)) {
	D(bug("!!! NO AROS MODEID IN FindDisplayInfo() !!!\n"));
	return NULL;
    }
    
    ret = (DisplayInfoHandle)ID;

    return ret;

    AROS_LIBFUNC_EXIT
} /* FindDisplayInfo */
