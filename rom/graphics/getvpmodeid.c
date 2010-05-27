/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function GetVPModeID()
    Lang: english
*/
#include <aros/debug.h>
#include <graphics/view.h>
#include <graphics/modeid.h>
#include <hidd/graphics.h>
#include <proto/graphics.h>

#include "graphics_intern.h"
#include "dispinfo.h"
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH1(ULONG, GetVPModeID,

/*  SYNOPSIS */
        AROS_LHA(struct ViewPort *, vp, A0),

/*  LOCATION */
        struct GfxBase *, GfxBase, 132, Graphics)

/*  FUNCTION
        returns the normal display modeID, if one is currently associated
        with this ViewPort

    INPUTS
        vp - pointer to ViewPort structure

    RESULT
        modeID - a 32 bit DisplayInfoRecord identifier associated
                 with this ViewPort, or INVALID_ID

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        ModeNotAvailable(), graphics/displayinfo.h

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    ULONG modeid;

    if (vp->ColorMap)
        /* If we have a colormap, get ModeID from it */
        modeid = vp->ColorMap->VPModeID;
    else if (IS_HIDD_BM(vp->RasInfo->BitMap))
        /* We also can try to obtain mode ID from HIDD bitmap */
	modeid = HIDD_BM_HIDDMODE(vp->RasInfo->BitMap);
    else
        modeid = INVALID_ID;

    D(bug(" GetVPModeID returning %x\n", modeid));
    return modeid;

    AROS_LIBFUNC_EXIT
} /* GetVPModeID */
