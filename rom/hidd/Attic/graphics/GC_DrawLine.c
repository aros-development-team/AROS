/*
    (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Draw a line
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

        AROS_LH5(ULONG, HIDD_Graphics_DrawLine,

/*  SYNOPSIS */
        AROS_LHA(APTR            , gc          , A2),
        AROS_LHA(WORD            , x1          , D2),
        AROS_LHA(WORD            , y1          , D3),
        AROS_LHA(WORD            , x2          , D4),
        AROS_LHA(WORD            , y2          , D5),

/*  LOCATION */
        struct Library *, GfxHiddBase, 20, GfxHidd)

/*  FUNCTION

    INPUTS
        gc    - valid pointer to a graphics context that was created with
                HIDD_Graphics_CreateGC()

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        GROUP=HIDD_Pixel

    INTERNALS

    HISTORY
        06-04-98    drieling created
***************************************************************************/

{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxHiddBase_intern *,GfxHiddBase)

    D(bug("HIDD_Graphics_DrawLine\n"));
    D(bug("  sorry, not yet implemented\n"));

    return FALSE;

    AROS_LIBFUNC_EXIT

} /* HIDD_Graphics_DrawLine */
