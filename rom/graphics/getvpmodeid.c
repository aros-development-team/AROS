/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function GetVPModeID()
    Lang: english
*/
#include <aros/debug.h>
#include <graphics/view.h>
#include <graphics/modeid.h>
#include <hidd/graphics.h>
#include <proto/graphics.h>
#include "dispinfo.h"

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
        ModeNotAvailable() graphics/displayinfo.h

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    ULONG modeid;
    
    D(bug(" GetVPModeID returning %x\n", vp->ColorMap->VPModeID));
    modeid = vp->ColorMap->VPModeID;
    
    D(bug("RETURNING\n"));
    
    return modeid;

    AROS_LIBFUNC_EXIT
} /* GetVPModeID */
