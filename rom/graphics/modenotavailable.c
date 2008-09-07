/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function ModeNotAvailable()
    Lang: english
*/
#include <graphics/displayinfo.h>
#include <graphics/modeid.h>

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH1(ULONG, ModeNotAvailable,

/*  SYNOPSIS */
        AROS_LHA(ULONG, modeID, D0),

/*  LOCATION */
        struct GfxBase *, GfxBase, 133, Graphics)

/*  FUNCTION
        returns an error code, indicating why this modeID is not available,
        or 0 if there is no reason known why this mode should not be there

    INPUTS
        modeID - a 32 bit DisplayInfoRecord identifier

    RESULT
        error - a general indication of why this modeID is not available,
                or 0 if there is no reason why it should not be available

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        GetVPModeID(), graphics/displayinfo.h

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    
#warning Do a real check here    

    return 0;


    AROS_LIBFUNC_EXIT
} /* ModeNotAvailable */
