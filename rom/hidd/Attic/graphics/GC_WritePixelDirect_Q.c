/*
    (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Write pixel direct quick
    Lang: english
*/

#include <exec/types.h>
#include <proto/graphics.h>

#include "gfxhidd_intern.h"
#define DEBUG 1
#include <aros/debug.h>

/*********************************************************************

    NAME */
#include <proto/gfxhidd.h>

        AROS_LH4(VOID, HIDD_Graphics_WritePixelDirect_Q,

/*  SYNOPSIS */
        AROS_LHA(APTR            , gc          , A2),
        AROS_LHA(WORD            , x           , D2),
        AROS_LHA(WORD            , y           , D3),
        AROS_LHA(ULONG           , val         , D4),

/*  LOCATION */
        struct Library *, GfxHiddBase, 17, GfxHidd)

/*  FUNCTION
        Set the pixel at (x,y) direct to val without making use of the gc
        attributes like colors, drawmode, colormask etc. The function does
        not check gc != NULL and the coordinates.

    INPUTS
        gc    - valid pointer to a graphics context that was created with
                HIDD_Graphics_CreateGC().
        (x,y) - coordinates of the pixel in hidd units
        val   - set pixel direct to this value

    RESULT

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

    D(bug("HIDD_Graphics_WritePixelDirect_Q:\n"));
    D(bug("  sorry, not yet implemented\n"));

    AROS_LIBFUNC_EXIT
} /* HIDD_Graphics_WritePixelDirect_Q */
