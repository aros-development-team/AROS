/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/libcall.h>
#include <graphics/clip.h>
#include <graphics/layers.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include "basicfuncs.h"

/*****************************************************************************

    NAME */
#include <proto/layers.h>
	AROS_LH8(struct Layer *, CreateBehindLayerTagList,

/*  SYNOPSIS */
	AROS_LHA(struct Layer_Info *, li, A0),
	AROS_LHA(struct BitMap     *, bm, A1),
        AROS_LHA(LONG               , x0, D0),
        AROS_LHA(LONG               , y0, D1),
        AROS_LHA(LONG               , x1, D2),
        AROS_LHA(LONG               , y1, D3),
	AROS_LHA(LONG               , flags, D4),
	AROS_LHA(struct TagItem    *, tagList, A2),

/*  LOCATION */
	struct LayersBase *, LayersBase, 40, Layers)

/*  FUNCTION
        Create a new layer according to the tags given.

    INPUTS
        li    - pointer to LayerInfo structure
        bm    - pointer to common bitmap
	x0,y0 - upper left corner of the layer (in parent layer coords)
	x1,y1 - lower right corner of the layer (in parent layer coords)
        flags - choose the type of layer by setting some flags
                If it is to be a super bitmap layer then the tag
                LA_SUPERBITMAP must be provided along with a 
                pointer to a valid super bitmap.
        tagList - a list of tags that specify the properties of the
                  layer. The following tags are currently supported:
                  LA_PRIORITY : priority class of the layer. The
                                higher the number the further the
                                layer will be in front of everything
                                else.
                                Default value is UPFRONTPRIORITY.
                  LA_HOOK     : Backfill hook
                  LA_SUPERBITMAP : pointer to a superbitmap. The flags
                                  must also represent that this
                                  layer is supposed to be a superbitmap
                                  layer.
                  LA_CHILDOF  : pointer to parent layer. If NULL then
                                this layer will be created as a old-style
                                layer.
                  LA_INFRONTOF : pointer to a layer in front of which
                                 this layer is to be created.
                  LA_BEHIND : pointer to a layer behind which this layer
                             is to be created. Must not give both LA_INFRONTOF
                             and LA_BEHIND.
                  LA_VISIBLE : FALSE if this layer is to be invisible.
                               Default value is TRUE
                  LA_SHAPE : The region of the layer that comprises its shape.
                             This value is optional. The region must be relative to the layer.
                  
        
    RESULT
        Pointer to the newly created layer. NULL if layer could not be 
        created (Probably out of memory).
        If the layer is created successfully you must not free its shape.
        The shape is automatically freed when the layer is deleted.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /*
     * *** !!! FIXME !!! ***
     * This condition was copied from CreateBehindHookLayer() and
     * it is 100% identical to one in CreateUpfrontHookLayer().
     * This urgently needs to be fixed.
     */
    int priority = (LAYERBACKDROP == (flags & LAYERBACKDROP)) ? BACKDROPPRIORITY: UPFRONTPRIORITY;

    return CreateLayerTagList(li, bm, x0, y0, x1, y1, flags, priority, tagList, LayersBase);

    AROS_LIBFUNC_EXIT
} /* CreateBehindHookLayer */
