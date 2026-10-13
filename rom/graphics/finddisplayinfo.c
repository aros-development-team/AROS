/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Graphics function FindDisplayInfo()
*/
#include <aros/debug.h>
#include <proto/graphics.h>
#include <graphics/displayinfo.h>
#include <graphics/modeid.h>
#include <hidd/gfx.h>

#include "graphics_intern.h"
#include "dispinfo.h"

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

    struct monitor_displaydata *mdd;
    struct DisplayInfoHandle *ret = NULL;
    HIDDT_ModeID hiddmode;

    D(bug("FindDisplayInfo(id=%x)\n", ID));

    /* The database may fail on INVALID_ID, so handle this explicitly */
    if(ID == INVALID_ID)
        return NULL;

    /* Resolve default-monitor IDs consistently with OpenScreenTags(), so
     * display and overscan queries describe the mode that will be opened.
     */
    if ((ID & MONITOR_ID_MASK) == DEFAULT_MONITOR_ID)
    {
        if (GfxBase->DisplayFlags & PAL)
            ID |= PAL_MONITOR_ID;
        else if (GfxBase->DisplayFlags & NTSC)
            ID |= NTSC_MONITOR_ID;
    }

    /* Find display driver data */
    for(mdd = GFXPRIVATE_MONITORFIRST; mdd; mdd = (struct monitor_displaydata *)mdd->mdisplay.display_next) {
        if(mdd->mdisplay.display_idbase == (ID & mdd->mdisplay.display_mask))
            break;
    }
    if(!mdd)
        return NULL;

    /* Calculate HIDD part of ModeID */
    hiddmode = ID & (~mdd->mdisplay.display_mask);

    /* Go through all mode records of this driver */
    for(ret = mdd->modes; ret->id != vHidd_ModeID_Invalid; ret++) {
        if(ret->id == hiddmode)
            return ret;
    }

    return NULL;

    AROS_LIBFUNC_EXIT
} /* FindDisplayInfo */
