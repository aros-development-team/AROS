/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/muimaster.h>

#include "support.h"

#include "mui.h"
#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
        AROS_LH5(APTR, MUI_AddClipping,

/*  SYNOPSIS */
        AROS_LHA(struct MUI_RenderInfo *, mri, A0),
        AROS_LHA(WORD, left, D0),
        AROS_LHA(WORD, top, D1),
        AROS_LHA(WORD, width, D2),
        AROS_LHA(WORD, height, D3),

/*  LOCATION */
        struct Library *, MUIMasterBase, 28, MUIMaster)

/*  FUNCTION
        Restrict drawing to a rectangle by creating a region and passing it
        to MUI_AddClipRegion(). Existing MUI clipping remains in effect.

    INPUTS
        mri - Valid rendering context for the target layer.
        left, top - Rectangle origin in the layer's drawing coordinates.
        width, height - Positive rectangle dimensions in pixels. Values
                        greater than or equal to MUI_MAXMAX skip clipping.

    RESULT
        A removal token, which can be NULL, or (APTR)-1 if no region was
        pushed. The latter includes allocation failure, a rejected region,
        a skipped dimension, and a rectangle containing the current clip
        region's entire bounds.

    NOTES
        Pair this call with MUI_RemoveClipping() using the same context
        and the exact token. Remove nested clips in reverse order. NULL
        is a valid success token; do not test the token as a Boolean.
        Failure and an unnecessary clip share the same sentinel, so this
        interface cannot guarantee that a new restriction was installed.

    BUGS
        The result of OrRectRegion() is not checked before the region is
        passed to MUI_AddClipRegion().

    SEE ALSO
        MUI_RemoveClipping(), MUI_AddClipRegion(), MUI_RemoveClipRegion()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Region *r;
    struct Rectangle rect;
    APTR handle;

    if ((width >= MUI_MAXMAX) || (height >= MUI_MAXMAX))
        return (APTR)-1;

    if (mri->mri_rCount > 0)
    {
        if (isRegionWithinBounds(mri->mri_rArray[mri->mri_rCount-1],
            left, top, width, height))
            return (APTR)-1;
    }

    if ((r = NewRegion()) == NULL)
        return (APTR)-1;

    rect.MinX = left;
    rect.MinY = top;
    rect.MaxX = left + width  - 1;
    rect.MaxY = top  + height - 1;
    OrRectRegion(r, &rect);

    handle = MUI_AddClipRegion(mri, r);

#if 0 /* MUI_AddClipRegion frees region itself upon failure */
    if (handle == (APTR)-1)
    {
        DisposeRegion(r);
    }
#endif
    return handle;

    AROS_LIBFUNC_EXIT

} /* MUIA_AddClipping */
