/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function BestModeIDA()
    Lang: english
*/
#include <graphics/modeid.h>

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH1(ULONG, BestModeIDA,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, TagItems, A0),

/*  LOCATION */
        struct GfxBase *, GfxBase, 175, Graphics)

/*  FUNCTION

    INPUTS
        TagItems - pointer to an array of TagItems

    RESULT
        ID - ID of the best mode to use, or INVALID_ID if a match
             could not be found

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        graphics/modeid.h graphics/displayinfo.h

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    return driver_BestModeIDA(TagItems, GfxBase);

    AROS_LIBFUNC_EXIT
} /* BestModeIDA */
