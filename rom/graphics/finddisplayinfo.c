/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function FindDisplayInfo()
    Lang: english
*/
#include <graphics/displayinfo.h>

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH1(DisplayInfoHandle, FindDisplayInfo,

/*  SYNOPSIS */
        AROS_LHA(ULONG, ID, D0),

/*  LOCATION */
        struct GfxBase *, GfxBase, 121, Graphics)

/*  FUNCTION

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

    return driver_FindDisplayInfo(ID, GfxBase);

    AROS_LIBFUNC_EXIT
} /* FindDisplayInfo */
