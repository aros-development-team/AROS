/*
    (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Write pixel direct
    Lang: english
*/

#include <exec/types.h>
#include <proto/gfxhidd.h>

#include "gfxhidd_intern.h"
#define DEBUG 0
#include <aros/debug.h>

/*********************************************************************

    NAME */
#include <proto/gfxhidd.h>

        AROS_LH4(BOOL, HIDD_Graphics_WritePixelDirect,

/*  SYNOPSIS */
        AROS_LHA(APTR            , gc          , A2),
        AROS_LHA(WORD            , x           , D2),
        AROS_LHA(WORD            , y           , D3),
        AROS_LHA(ULONG           , val         , D4),

/*  LOCATION */
        struct Library *, GfxHiddBase, 13, GfxHidd)

/*  FUNCTION
        Set the pixel at (x,y) direct to val without making use of the gc
        attributes like colors, drawmode, colormask etc. This function
        checks the gc pointer and the coordinates and writes the pixel only
        if it is inside the gc.

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
        GROUP=HIDD_Pixel

    INTERNALS

    HISTORY
        05-04-98    drieling created
***************************************************************************/

#define GC ((struct hGfx_gc *) gc)

{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxHiddBase_intern *,GfxHiddBase)

    BOOL ok = FALSE;

    D(bug("HIDD_Graphics_WritePixelDirect\n"));
    /*D(bug("  sorry, not yet implemented\n"));*/

    if(gc)
    {
        if((x >= 0) &&
           (y >= 0) &&
           (x < ((struct hGfx_bitMap *) GC->bitMap)->width) &&
           (y < ((struct hGfx_bitMap *) GC->bitMap)->height)
          )
        {
            HIDD_Graphics_WritePixelDirect_Q(gc, x, y, val);
            ok = TRUE;
        }
    }

    return(ok);

    AROS_LIBFUNC_EXIT

} /* HIDD_Graphics_WritePixelDirect */
