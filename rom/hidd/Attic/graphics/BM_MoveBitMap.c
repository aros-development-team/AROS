/*
    (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Move a bitmap of a graphics hidd
    Lang: english
*/

#include <exec/types.h>
#include <proto/intuition.h>

#include "gfxhidd_intern.h"
#define DEBUG 1
#include <aros/debug.h>

/*********************************************************************

    NAME */
#include <proto/gfxhidd.h>
#include <utility/tagitem.h>

        AROS_LH4(VOID, HIDD_Graphics_MoveBitMap,

/*  SYNOPSIS */
        AROS_LHA(APTR            , bitMap      , A2),
        AROS_LHA(WORD            , horizontal  , D2),
        AROS_LHA(WORD            , vertical    , D3),
        AROS_LHA(struct TagItem *, tagList     , A3),

/*  LOCATION */
        struct Library *, GfxHiddBase, 9, GfxHidd)

/*  FUNCTION
        Move a visible bitmap around the screen. Positive values mean move
        to the left or down, negative values means right and up.
        If the horizontal and vertical variables you specify would move
        the bitmap beyond any restrictions then the bitmap is move
        only as far as possible. You can get the real position to which
        the bitmap was move with the HIDDA_BitMap_LeftEdge and
        HIDDA_BitMap_TopEdge attributes.

    INPUTS
        bitMap  - valid pointer to a bitmap that was created with
                  HIDD_Graphics_CreateBitMap(). Passing a NULL-pointer
                  (meaning "do nothing") is OK.
        horizontal - if negative move screen to the right otherwise
                     to the left.
        vertical  - if negative move screen up otherwise down.
        tagList - for future extensions, set this always to NULL.

    RESULT
        position

    NOTES
        The horizontal and vertical values are in HIDD units. Some
        graphics HIDDs support text mode, then this is in characters.
        But most of the time, this will be in screen or printer pixels.

    EXAMPLE

    BUGS

    SEE ALSO
        GROUP=HIDD_Graphics_Bitmap

    INTERNALS

    HISTORY
        05-04-98    drieling created
***************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxHiddBase_intern *,GfxHiddBase)

    D(bug("HIDD_Graphics_MoveBitMap\n"));
    D(bug("  sorry, not yet implemented\n"));

    AROS_LIBFUNC_EXIT

} /* HIDD_Graphics_MoveBitMap */
