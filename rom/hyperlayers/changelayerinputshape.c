/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Where a layer takes input. AROS extension.
*/
#include <aros/libcall.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <graphics/clip.h>
#include <graphics/layers.h>
#include "layers_intern.h"
#include "basicfuncs.h"

/*****************************************************************************

    NAME */

        AROS_LH2(BOOL, ChangeLayerInputShape,

/*  SYNOPSIS */
        AROS_LHA(struct Layer  *, l     , A0),
        AROS_LHA(struct Region *, region, A1),

/*  LOCATION */
        struct LayersBase *, LayersBase, 53, Layers)

/*  FUNCTION
        Sets where the layer takes input, apart from where it is seen: the
        region WhichLayer() and IsLayerHitAt() use for the layer from now
        on (see LA_InputShape).

        A point hits the layer when the layer is visible, the point is
        inside the layer's visible shape and inside this region. The
        pixels' alpha of a composited layer is not looked at; a point
        outside the region goes to the layers behind, whatever is drawn
        there - a translucent shadow, for example, that should not catch
        clicks meant for the window under it.

    INPUTS
        l      - the layer
        region - layer relative (0,0 is the layer's top left corner, as for
                 LA_ShapeRegion), or NULL for the default: input wherever
                 the layer is seen (its visible shape and, for an alpha
                 surface, every pixel whose alpha is not 0). An empty
                 region makes the layer take no input at all.

    RESULT
        TRUE when the new input shape is in place. FALSE when no memory
        was left for the copy; the layer keeps its previous input shape.

    NOTES
        layers.library keeps a copy of its own; the region may be disposed
        or changed after the call. The input shape is not moved, scaled or
        clipped when the layer is moved or sized; call this function again
        after a size change when the input area depends on the size.

        Works the same for composited and ordinary layers.

        For an Intuition window, pass window->WLayer, or give
        WA_InputRegion to OpenWindowTagList().

        This function is an AROS extension.

    EXAMPLE

    BUGS

    SEE ALSO
        IsLayerHitAt(), WhichLayer(), CreateUpfrontLayerTagList()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Region *copy = NULL, *old;

    if (NULL == l)
        return FALSE;

    if (NULL != region)
    {
        if (NULL == (copy = NewRegion()))
            return FALSE;
        if (!OrRegionRegion(region, copy))
        {
            DisposeRegion(copy);
            return FALSE;
        }
    }

    /* The hit test runs under the Layer_Info lock (WhichLayer() callers) */
    LockLayerInfo(l->LayerInfo);
    old = IL(l)->inputshape;
    IL(l)->inputshape = copy;
    UnlockLayerInfo(l->LayerInfo);

    if (NULL != old)
        DisposeRegion(old);

    return TRUE;

    AROS_LIBFUNC_EXIT
} /* ChangeLayerInputShape */
