/*
    (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Write pixel quick
    Lang: english
*/

#include <exec/types.h>
#include <proto/graphics.h>

#include "gfxhidd_intern.h"
#define DEBUG 0
#include <aros/debug.h>

/*********************************************************************

    NAME */
#include <proto/gfxhidd.h>

        AROS_LH3(VOID, HIDD_Graphics_WritePixel_Q,

/*  SYNOPSIS */
        AROS_LHA(APTR            , gc          , A2),
        AROS_LHA(WORD            , x           , D2),
        AROS_LHA(WORD            , y           , D3),

/*  LOCATION */
        struct Library *, GfxHiddBase, 22, GfxHidd)

/*  FUNCTION
        Changes the pixel at (x,y). The color of the pixel depends on the
        attributes of gc, eg. colors, drawmode, colormask etc.
        This function does not check the gc pointer != NULL and the
        coordinates.

    INPUTS
        gc    - valid pointer to a graphics context that was created with
                HIDD_Graphics_CreateGC().
        (x,y) - coordinates of the pixel in hidd units

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        GROUP=HIDD_Gfx_Pixel, GROUP=HIDD_Gfx_SetAttributes

    INTERNALS
        TODO Support for drawing modes etc.

    HISTORY
        05-04-98    drieling created
***************************************************************************/

#define BM ((struct hGfx_bitMapInt *) bitMap)
#define GC ((struct hGfx_gc *) gc)
#define GCINT ((struct hGfx_gcInt *) gc)

{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxHiddBase_intern *,GfxHiddBase)

    D(bug("HIDD_Graphics_WritePixel_Q\n"));
    /*D(bug("  sorry, not yet implemented\n"));*/

    return HIDD_Graphics_WritePixelDirect_Q(gc, x, y, GC->fg);

    AROS_LIBFUNC_EXIT

} /* HIDD_Graphics_WritePixel_Q */
