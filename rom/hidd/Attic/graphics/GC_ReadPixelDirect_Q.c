/*
    (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Read pixel direct quick
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

        AROS_LH3(ULONG, HIDD_Graphics_ReadPixelDirect_Q,

/*  SYNOPSIS */
        AROS_LHA(APTR            , gc          , A2),
        AROS_LHA(WORD            , x           , D2),
        AROS_LHA(WORD            , y           , D3),

/*  LOCATION */
        struct Library *, GfxHiddBase, 19, GfxHidd)

/*  FUNCTION
        Queries the color of the pixel at (x,y) direct. The return
        code is the physical value for that color in the format used by
        the HIDD.
        The function does not check gc != NULL and the coordinates.

    INPUTS
        gc    - valid pointer to a graphics context that was created with
                HIDD_Graphics_GC()
        (x,y) - coordinates of the pixel in hidd units

    RESULT
        color - physical value at (x,y)

    NOTES
        The function always returns something but if gc == NULL or the
        coordinates are outside the valid coordinates, this can crash your
        machine or return random values.

    EXAMPLE

    BUGS

    SEE ALSO
        GROUP=HIDD_Pixel

    INTERNALS

    HISTORY
        05-04-98    drieling created
***************************************************************************/

#define GCINT ((struct hGfx_gcInt *) gc)

{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxHiddBase_intern *,GfxHiddBase)

    D(bug("HIDD_Graphics_ReadPixelDirect \n"));
    D(bug("  sorry, not yet implemented\n"));

    return -1;

    AROS_LIBFUNC_EXIT

} /* HIDD_Graphics_ReadPixelDirect_Q */
