/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function GetVPModeID()
    Lang: english
*/
#include <graphics/view.h>
#include <graphics/modeid.h>

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
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)
    
    return driver_GetVPModeID(vp, GfxBase);


    AROS_LIBFUNC_EXIT
} /* GetVPModeID */
