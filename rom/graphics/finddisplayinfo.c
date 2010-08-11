/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function FindDisplayInfo()
    Lang: english
*/
#include <aros/debug.h>
#include <proto/graphics.h>
#include <graphics/displayinfo.h>
#include <hidd/graphics.h>

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

    struct monitor_driverdata *mdd;
    struct DisplayInfoHandle *ret = NULL;
    HIDDT_ModeID hiddmode;

    D(bug("FindDisplayInfo(id=%x)\n", ID));

    /* The database may fail on INVALID_ID, so handle this explicitly */
    if (ID == INVALID_ID)
        return NULL;

    /* Find display driver data */
    for (mdd = CDD(GfxBase)->monitors; mdd; mdd = mdd->next) {
        if (mdd->id == (ID & mdd->mask))
	    break;
    }
    if (!mdd)
        return NULL;

    /* Calculate HIDD part of ModeID */
    hiddmode = ID & (~mdd->mask);

    /* Go through all mode records of this driver */
    for (ret = mdd->modes; ret->id != vHidd_ModeID_Invalid; ret++) {
	if (ret->id == hiddmode)
	    return ret;
    }

    return NULL;

    AROS_LIBFUNC_EXIT
} /* FindDisplayInfo */
