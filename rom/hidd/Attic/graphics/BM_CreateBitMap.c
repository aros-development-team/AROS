/*
    (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Create a bitmap for a graphics hidd
    Lang: english
*/

#include <exec/types.h>

#include <proto/exec.h>

#include <exec/memory.h>

#include "gfxhidd_intern.h"
#define DEBUG 1
#include <aros/debug.h>

/*********************************************************************

    NAME */
#include <proto/gfxhidd.h>
#include <hidd/graphics.h>
#include <utility/tagitem.h>

        AROS_LH6(APTR, HIDD_Graphics_CreateBitMap,

/*  SYNOPSIS */
        AROS_LHA(ULONG           , width       , D2),
        AROS_LHA(ULONG           , height      , D3),
        AROS_LHA(ULONG           , depth       , D4),
        AROS_LHA(ULONG           , flags       , D5),
        AROS_LHA(APTR            , friendBitMap, A2),
        AROS_LHA(struct TagItem *, tagList     , A3),

/*  LOCATION */
        struct Library *, GfxHiddBase, 6, GfxHidd)

/*  FUNCTION
        Create a drawing area(bitmap) with the specified attributes.


    INPUTS
        width    - Create a bitmap with this width. The width is in HIDD
                   units. Some graphics HIDDs support text mode, then this
                   is in characters. But most of the time, this will be
                   in screen or printer pixels.
        height   - Create a bitmap with this height. The height is in HIDD
                   units. Some graphics HIDDs support text mode, then
                   this is in characters. But most of the time, this will
                   be in screen or printer pixels.
        depth    - Create a bitmap with this depth. The number of distinct
                   colors is |1L << depth|. Most HIDDs will support depths
                   like 1, 8, 15, 16, 24 and 32 bit.
        flags    -
            \begin{description}
            \item{HIDD_Graphics_Bitmap_Flags_Displayable} create a bitmap
                   which should later be displayed. On some systems, this
                   will also display the bitmap when the function returns.
                   To be safe, always call |HIDD_Graphics_ShowBitmap()|
                   afterwards. A bitmap which a showable may be needs
                   more memory for alignment.

            \item{HIDD_Graphics_Bitmap_Flags_Planar} try to allocate a bitmap
                   in planar format.

            \item{HIDD_Graphics_Bitmap_Flags_Chunky} try to allocate a bitmap
                   in chunky format.

            \end{description}

        friendBitMap - pointer to another bitmap, or NULL. If this pointer
                       is passed, then the bitmap data will be created in
                       the most efficient form for blitting to friend_bitmap.
        tagList  -  for future extension, set this allways to NULL.

    RESULT
        bitmap - pointer to the created bitmap, or NULL if it was not
                 possible to create the desired bitmap.

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

    D(bug("HIDD_Graphics_CreateBitMap\n"));
    D(bug("  sorry, not yet implemented\n"));

    return NULL;

    AROS_LIBFUNC_EXIT
} /* HIDD_Graphics_CreateBitMap */
