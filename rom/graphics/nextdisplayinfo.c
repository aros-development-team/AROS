/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function NextDisplayInfo()
    Lang: english
*/
#include <graphics/displayinfo.h>

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

    return driver_NextDisplayInfo(last_ID, GfxBase);

    AROS_LIBFUNC_EXIT
} /* NextDisplayInfo */
