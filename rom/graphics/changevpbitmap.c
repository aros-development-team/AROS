/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function ChangeVPBitMap()
    Lang: english
*/
#include <aros/debug.h>
#include <graphics/gfx.h>
#include <graphics/view.h>

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH3(void, ChangeVPBitMap,

/*  SYNOPSIS */
        AROS_LHA(struct ViewPort *, vp, A0),
        AROS_LHA(struct BitMap *, bm, A1),
        AROS_LHA(struct DBufInfo *, db, A2),

/*  LOCATION */
        struct GfxBase *, GfxBase, 157, Graphics)

/*  FUNCTION

    INPUTS
        vp - pointer to a viewport
        bm - pointer to a BitMap structure. This BitMap structure must be of
             the same layout as the one attached to the viewport
             (same depth, alignment and BytesPerRow)
        db - pointer to a DBufInfo

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT

#warning TODO: Write graphics/ChangeVPBitMap()
    aros_print_not_implemented ("ChangeVPBitMap");

    AROS_LIBFUNC_EXIT
} /* ChangeVPBitMap */
