/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
    $Id$    $Log

    Desc: Graphics function PolyDraw()
*/
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH3(void, PolyDraw,

/*  SYNOPSIS */
        AROS_LHA(struct RastPort *, rp, A1),
        AROS_LHA(WORD             , count, D0),
        AROS_LHA(WORD            *, polyTable, A0),

/*  LOCATION */
        struct GfxBase *, GfxBase, 56, Graphics)

/*  FUNCTION
        Draw connected lines from an array. The first line is drawn
        from the current pen position to the first entry in the array.

    INPUTS
        rp        - RastPort
        count     - number of x,y pairs
        polyTable - array of x,y pairs

    RESULT

    NOTES
        count is a WORD (D0:16) as in the AmigaOS autodoc; it is used
        unsigned here, as the original ROM code does.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
        29-10-95    digulla automatically created from
                            graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    UWORD i, cnt;
    WORD x, y;

    cnt = (UWORD)count; /* see NOTES */
    for(i = 0; i < cnt; i++) {
        x = *polyTable++;
        y = *polyTable++;

        Draw(rp, x, y);
    }

    AROS_LIBFUNC_EXIT

} /* PolyDraw */
