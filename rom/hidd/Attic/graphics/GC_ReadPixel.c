/*
    (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Write pixel direct
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

        AROS_LH4(ULONG, HIDD_Graphics_ReadPixel,

/*  SYNOPSIS */
        AROS_LHA(APTR            , gc          , A2),
        AROS_LHA(WORD            , x           , D2),
        AROS_LHA(WORD            , y           , D3),
        AROS_LHA(ULONG           , val         , D4),

/*  LOCATION */
        struct Library *, GfxHiddBase, 15, GfxHidd)

/*  FUNCTION

    INPUTS
        gc    - valid pointer to a graphics context that was created with
                HIDD_Graphics_CreateGC(). Passing a NULL-pointer
                (meaning "do nothing") is OK.
        (x,y) - coordinates of the pixel in hidd units

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

#define BM ((struct hGfx_bitMapInt *) bitMap)
#define GC ((struct hGfx_gc *) gc)
#define GCINT ((struct hGfx_gcInt *) gc)

{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxHiddBase_intern *,GfxHiddBase)

    D(bug("HIDD_Graphics_ReadPixel\n"));
    D(bug("  sorry, not yet implemented\n"));

    return(-1);

    AROS_LIBFUNC_EXIT

} /* HIDD_Graphics_ReadPixel */
