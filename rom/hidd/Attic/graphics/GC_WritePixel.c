/*
    (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Write pixel
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

        AROS_LH3(BOOL, HIDD_Graphics_WritePixel,

/*  SYNOPSIS */
        AROS_LHA(APTR            , gc          , A2),
        AROS_LHA(WORD            , x           , D2),
        AROS_LHA(WORD            , y           , D3),

/*  LOCATION */
        struct Library *, GfxHiddBase, 14, GfxHidd)

/*  FUNCTION
        Changes the pixel at (x,y). The color of the pixel depends on the
        attributes of gc, eg. colors, drawmode, colormask etc.
        This function checks the gc pointer and the coordinates and writes
        the pixel only if it is inside the gc.

    INPUTS
        gc    - valid pointer to a graphics context that was created with
                HIDD_Graphics_CreateGC(). Passing a NULL-pointer
                (meaning "do nothing") is OK.
        (x,y) - coordinates of the pixel in hidd units

    RESULT
        error - TRUE if the pixel is written
                FALSE if gc is NULL or the pixel is outside the gc

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

    D(bug("HIDD_Graphics_WritePixel\n"));
    /*D(bug("  sorry, not yet implemented\n"));*/

    return HIDD_Graphics_WritePixelDirect(gc, x, y, GC->fg);

    AROS_LIBFUNC_EXIT

} /* HIDD_Graphics_WritePixel */
