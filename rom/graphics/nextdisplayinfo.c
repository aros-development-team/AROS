/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function NextDisplayInfo()
    Lang: english
*/
#include <graphics/displayinfo.h>
#include <hidd/graphics.h>
#include <proto/oop.h>
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
	Go to next entry in the DisplayInfo database.

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
        FindDisplayInfo(), GetDisplayInfoData(), graphics/displayinfo.h

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct monitor_driverdata *mdd = NULL;
    struct DisplayInfoHandle *dh = NULL;

    if (last_ID == INVALID_ID)
        /* Start from the beginning */
        mdd = CDD(GfxBase)->monitors;
    else {
        /* Find handle and driver of current mode */
        dh = FindDisplayInfo(last_ID);
	if (dh)
	    mdd = dh->drv;
    }

    while (mdd) {
        /* Take next (or first) mode handle */
        if (dh)
	    dh++;
	else
	    dh = mdd->modes;

	/* If it's not the last handle, return it */
        if (dh->id != vHidd_ModeID_Invalid)
            return dh->drv->id | dh->id;

	/* Next driver, first handle */
        mdd = mdd->next;
	dh = NULL;
    }

    return INVALID_ID;

    AROS_LIBFUNC_EXIT
} /* NextDisplayInfo */
