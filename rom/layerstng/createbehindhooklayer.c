/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <proto/exec.h>
#include <utility/tagitem.h>
#include <graphics/layersext.h>
#include <aros/libcall.h>
#include "layers_intern.h"

/*****************************************************************************

    NAME */
#include <proto/layers.h>
	AROS_LH9(struct Layer *, CreateBehindHookLayer,

/*  SYNOPSIS */
	AROS_LHA(struct Layer_Info *, li, A0),
	AROS_LHA(struct BitMap     *, bm, A1),
	AROS_LHA(LONG               , x0, D0),
	AROS_LHA(LONG               , y0, D1),
	AROS_LHA(LONG               , x1, D2),
	AROS_LHA(LONG               , y1, D3),
	AROS_LHA(LONG               , flags, D4),
	AROS_LHA(struct Hook       *, hook, A3),
	AROS_LHA(struct BitMap     *, bm2, A2),

/*  LOCATION */
	struct LayersBase *, LayersBase, 32, Layers)

/*  FUNCTION
        Create a new layer at the given position and with the
        given size. The new layer will be in front of all other
        layers. If it is a backdrop layer it will be created
        in front of all other backdrop layers and behind all
        non backdrop layers.
        Install the given hook as a backfill hook. This hook will
        be called whenever a part of the layer is supposed to be
        filled with a certain pattern. The backfill hook has to
        do that.
        If a super bitmap layer is wanted the flags LAYERSUPER and
        the flag LAYERSMART have to be set and a pointer to a 
        bitmap must also be passed to this function. 

    INPUTS
        li    - pointer to LayerInfo structure
        bm    - pointer to common bitmap
        x0, y0- upper left corner of the layer
        x1, y1- lower right corner of the layer
        flags - choose the type of layer by setting some flags
        hook  - pointer to the backfill hook of this layer
                The backfill hook will be called with
                     object = (struct RastPort *) result->RastPort
                and message = [ (struct Layer *) layer,
                                (struct Rectangle) bounds,
                                (WORD) offsetx,
                                (WORD) offsety ]
        bm2   - pointer to optional super bitmap. 
        
    RESULT
        Pointer to the newly created layer. NULL if layer could not be 
        created (Probably out of memory).
        
    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    layers_lib.fd and clib/layers_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct LayersBase *,LayersBase)

    struct TagItem layer_tags[] =
    {
	{LA_Type	, 0		},
	{LA_Priority	, 0		},
	{LA_SuperBitMap , (IPTR)bm2	},
	{LA_Behind	, TRUE		},
	{LA_BackFill	, (IPTR)hook	},
	{TAG_DONE			}
    };

    layer_tags[0].ti_Data = IS_SIMPLELAYERFLAG(flags) ? LAYERSIMPLE :
    						      (IS_SMARTLAYERFLAG(flags) ? LAYERSMART : LAYERSUPER);
    layer_tags[1].ti_Data = IS_BACKDROPFLAG(flags) ? LPRI_BACKDROP : LPRI_NORMAL;
     
    return CreateLayerTagList(li, bm, x0, y0, x1, y1, layer_tags);
  
    AROS_LIBFUNC_EXIT
} /* CreateBehindHookLayer */
