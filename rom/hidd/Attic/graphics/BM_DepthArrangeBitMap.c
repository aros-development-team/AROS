/*
    (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Move a bitmap of a graphics hidd to front or back
    Lang: english
*/

#include <exec/types.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include "gfxhidd_intern.h"
#define DEBUG 1
#include <aros/debug.h>

/*********************************************************************

    NAME */
#include <proto/gfxhidd.h>
#include <utility/tagitem.h>

        AROS_LH4(VOID, HIDD_Graphics_DepthArrangeBitMap,

/*  SYNOPSIS */
        AROS_LHA(APTR            , bitMap      , A2),
        AROS_LHA(ULONG           , mode        , D2),
        AROS_LHA(APTR            , otherBitMap , A3),
        AROS_LHA(struct TagItem *, tagList     , A4),

/*  LOCATION */
        struct Library *, GfxHiddBase, 10, GfxHidd)

/*  FUNCTION
        Move a bitmap to the front or back of all bitmaps or a specific
        bitmap. If |other| is |!= NULL|, then the bitmap will be moved just
        in front or behind that bitmap. The |otherBitMap| is |NULL|, then
        bitmap will be moved in front or behind all other bitmaps. If the
        bitmap was invsible, then |HIDD_Graphics_ShowBitMap()| will
        implicitly be called.

    INPUTS
        bitMap  - valid pointer to a bitmap that was created with
                  HIDD_Graphics_CreateBitMap(). Passing a NULL-pointer
                  (meaning "do nothing") is OK.
        mode    -
            \begin{description}
            \item{HIDDV_Graphics_DepthArrange_ToFront} move bitmap in front
            of all other bitmaps or if the |otherBitMap| is not |NULL|
            before this bitmap.

            \item{HIDDV_Graphics_DepthArrange_ToBack} move bitmap behind
            all other bitmaps or if the |otherBitMap| is not |NULL|
            behind this bitmap.
            \end{description}

        otherBitMap - valid pointer to an other bitmap that was created
                      with HIDDV_Graphics_CreateBitMap() or NULL.
        tagList - for future extensions, set this always to NULL.

    RESULT

    NOTES

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

    D(bug("HIDD_Graphics_DepthArrangeBitMap\n"));
    D(bug("  sorry, not yet implemented\n"));

    AROS_LIBFUNC_EXIT

} /* HIDD_Graphics_DepthArrangeBitMap */
