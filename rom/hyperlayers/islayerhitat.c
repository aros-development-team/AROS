/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Alpha aware hit test of one layer. AROS extension.
*/
#include <aros/libcall.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <graphics/clip.h>
#include <graphics/layers.h>
#include "layers_intern.h"
#include "basicfuncs.h"

/*****************************************************************************

    NAME */

        AROS_LH3(BOOL, IsLayerHitAt,

/*  SYNOPSIS */
        AROS_LHA(struct Layer *, l, A0),
        AROS_LHA(LONG          , x, D0),
        AROS_LHA(LONG          , y, D1),

/*  LOCATION */
        struct LayersBase *, LayersBase, 52, Layers)

/*  FUNCTION
        Tells whether the point (x,y), in the coordinates of the layer's
        bitmap (screen coordinates), hits the layer: the layer is visible,
        the point is inside its shape and, for a composited layer with a
        per pixel alpha surface (LA_Composited + LA_AlphaSurface), the
        surface pixel there is not fully transparent (alpha 0).

        A layer with an input shape (LA_InputShape,
        ChangeLayerInputShape()) is hit where the point is inside both
        its visible shape and its input shape; alpha does not count then.

        This is the policy WhichLayer() applies to every layer front to
        back; it lets Intuition and applications apply the same one, for
        example to decide whether a click on a shaped, translucent window
        belongs to it or to what is seen through its transparent parts.

    INPUTS
        l    - the layer
        x, y - the point, in the coordinates of the layer's bitmap

    RESULT
        TRUE when the point hits the layer, FALSE when it does not.

        Layers in front of l are not taken into account: a point inside l
        that is covered by another layer still hits l. Use WhichLayer()
        to find the frontmost layer at a point.

    NOTES
        The function does not lock the Layer_Info. It is the
        responsibility of the caller to hold the lock (LockLayerInfo() /
        UnlockLayerInfo()) as for WhichLayer().

        A composited layer with a constant opacity (LA_Opacity) but
        without an alpha surface is hit everywhere inside its shape, as
        are all other layer kinds, whatever is drawn in them.

        This function is an AROS extension. It is not MorphOS's
        IsLayerHitable(layer), which takes the layer only, answers a
        different question (is the layer hitable at all) and has its own,
        earlier LVO, which stays reserved for it.

    EXAMPLE

    BUGS

    SEE ALSO
        WhichLayer(), CreateUpfrontLayerTagList(), ChangeLayerInputShape()

    INTERNALS
        The surface pixel's alpha comes from graphics.library's private
        LayerSurfaceAlpha(), which knows the surface's pixel format.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return _IsLayerHitAt(l, x, y, LayersBase);

    AROS_LIBFUNC_EXIT
} /* IsLayerHitAt */
