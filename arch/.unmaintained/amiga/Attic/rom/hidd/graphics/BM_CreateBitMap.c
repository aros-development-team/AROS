/*
    (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Create a bitmap for a graphics hidd
    Lang: english
*/

#include <exec/types.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include <exec/memory.h>
#include <graphics/gfx.h>
#include <intuition/screens.h>

#include "gfxhidd_internIntuition.h"
#define DEBUG 0
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
        TODO Support for HIDD_Graphics_Bitmap_Flags_Planar
                         HIDD_Graphics_Bitmap_Flags_Chunky
                         friend_bitmap
             Init bitMap structure correct

    HISTORY
        05-04-98    drieling created
***************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxHiddBase_intern *,GfxHiddBase)

    BOOL   ok = FALSE;
    struct hGfx_bitMapInt *bm;
    struct TagItem tags[] =
    {
        {SA_Left     , 0},
        {SA_Top      , 0},
        {SA_Width    , width},
        {SA_Height   , height},
        {SA_Depth    , depth},
        {TAG_SKIP    , 1},      /* Use SA_DisplayID only wenn a display mode */
        {SA_DisplayID, 0},      /* is specified                              */
        {SA_Title    , (ULONG) "AROS graphics hidd"},
        {SA_Type     , CUSTOMSCREEN},

        /* Will be use later   */
        /* SA_Quiet    , TRUE, */
        /* SA_BitMap   , 0     On V37 double buffering could only
                               implemented with a custom bitmap */
        {TAG_DONE,   0}
    };

    /* alloc and initialize bitmap structure */
    bm = AllocVec(sizeof(struct hGfx_bitMapInt), MEMF_CLEAR | MEMF_PUBLIC);
    bm->bm.width  = width;
    bm->bm.height = height;
    bm->bm.depth  = depth;

    if(bm)
    {
        if(flags & HIDD_Graphics_BitMap_Flags_Displayable)
        /* bitmap is displayable use an intuition screen */
        {
            bm->screen = OpenScreenTagList(NULL, tags);
            if(bm->screen)
            {
                ok = TRUE;
            }
        }
        else
        /* bitmap is not displayable use AllocBitMap() */
        {
            bm->bitMap = AllocBitMap(width, height, depth, BMF_CLEAR, NULL);
            if(bm->bitMap)
            {
                ok = TRUE;
            }
        }
    } /* if(bm) */


    /*
       if OpenScreenTagList() or AllocBitMap() fails then free bitMap
       structure.
    */
    if(!ok)
    {
        FreeVec(bm);
        bm = NULL;
    }

    return (APTR) bm;
    AROS_LIBFUNC_EXIT
} /* HIDD_Graphics_CreateBitMap */
