/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function NextDisplayInfo()
    Lang: english
*/
#include <graphics/displayinfo.h>
#include <hidd/graphics.h>
#include "graphics_intern.h"
#include "dispinfo.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH1(ULONG, NextDisplayInfo,

/*  SYNOPSIS */
        AROS_LHA(ULONG, last_ID, D0),

/*  LOCATION */
        struct GfxBase *, GfxBase, 122, Graphics)

/*  FUNCTION

    INPUTS
        last_ID - previous displayinfo identifier
                  or INVALID_ID if beginning iteration

    RESULT
        next_ID - subsequent displayinfo identifier
                  or INVALID_ID if no more records

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        FindDisplayInfo() GetDisplayInfoData() graphics/displayinfo.h

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    OOP_Object *sync, *pixfmt;
    
    HIDDT_ModeID hiddmode;
    ULONG id;
    
    hiddmode = (HIDDT_ModeID)AMIGA_TO_HIDD_MODEID(last_ID);
    
    /* Get the next modeid */
    hiddmode = HIDD_Gfx_NextModeID(SDD(GfxBase)->gfxhidd, hiddmode, &sync, &pixfmt);
    
    id = HIDD_TO_AMIGA_MODEID(hiddmode);
    
    return id;

    AROS_LIBFUNC_EXIT
} /* NextDisplayInfo */
