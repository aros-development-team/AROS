/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/graphics.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <graphics/layersext.h>
#include <graphics/layers.h>
#include <graphics/clip.h>
#include <utility/tagitem.h>
#include <aros/libcall.h>

#include "layers_intern.h"
#include "basicfuncs.h"


/*****************************************************************************

    NAME */
#include <proto/layers.h>

        AROS_LH7(struct Layer *, CreateLayerTagList,

/*  SYNOPSIS */
        AROS_LHA(struct Layer_Info *, li, A0),
        AROS_LHA(struct BitMap     *, bm, A1),
        AROS_LHA(LONG               , x0, D0),
        AROS_LHA(LONG               , y0, D1),
        AROS_LHA(LONG               , x1, D2),
        AROS_LHA(LONG               , y1, D3),
        AROS_LHA(struct TagItem    *, taglist, A2),

/*  LOCATION */
        struct LayersBase *, LayersBase, 37, Layers)

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
	taglist - pointer to taglist (defined in graphics/layersext.h)

    RESULT
        pointer to layer if successful, NULL otherwise

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

    struct Layer 	*lay, *behindlay, *frontlay;
    struct Hook		*layerbackfill;
    struct BitMap	*layersuperbm;
    struct Region	*layershape, *damagelist;
    struct RastPort	*rp;
    WORD 		layerpri;
    WORD		layertype;
    BOOL		layerbehind;
    BOOL		layerinvisible;
    
    if (!li || !bm) return NULL;
    
    layertype      = GetTagData(LA_Type, LAYERSMART, taglist);
    layerpri       = GetTagData(LA_Priority, LPRI_NORMAL, taglist);
    layerbehind    = GetTagData(LA_Behind, FALSE, taglist);
    layerinvisible = GetTagData(LA_Invisible, FALSE, taglist);
    layerbackfill  = (struct Hook *)GetTagData(LA_BackFill, (IPTR)LAYERS_BACKFILL, taglist);
    layersuperbm   = (struct BitMap *)GetTagData(LA_SuperBitMap, 0, taglist);
    layershape     = (struct Region *)GetTagData(LA_Shape, 0, taglist);
    
    if ((layertype == LAYERSUPER) && (layersuperbm == NULL)) return NULL;
    
    lay        = AllocMem(sizeof(struct IntLayer), MEMF_PUBLIC | MEMF_CLEAR);
    rp         = CreateRastPort();
    damagelist = NewRegion();
    
    if (lay && rp && damagelist)
    {
        lay->rp           = rp;
	lay->bounds.MinX  = x0;
	lay->bounds.MinY  = y0;
	lay->bounds.MaxX  = x1;
	lay->bounds.MaxY  = y1;
	
	lay->Flags        = LAYERTYPE_TO_FLAG(layertype) + LAYERPRI_TO_FLAG(layerpri);
	lay->LayerInfo    = li;
	lay->Width	  = x1 - x0 + 1;
	lay->Height       = y1 - y0 + 1;
	lay->SuperBitMap  = layersuperbm;
	lay->BackFill     = layerbackfill;
	
	lay->DamageList   = damagelist;
	
	IL(lay)->pri	  = layerpri;
	IL(lay)->shape	  = layershape;
	IL(lay)->intflags = layerinvisible ? LFLG_INVISIBLE : 0;
	
	InitSemaphore(&lay->Lock);
	
	rp->Layer  = lay;
	rp->BitMap = bm;
	
	LockLayers(li);
	
	behindlay = FindBehindLayer(li, layerpri, layerbehind);
	if (behindlay)
	{
	    frontlay = behindlay->front;
	} else {
	    frontlay = li->top_layer;
	    if (frontlay)
	    {
	        while(frontlay->back) frontlay = frontlay->back;
	    }
	}
	
	if (behindlay) behindlay->front = lay;
	lay->back  = behindlay;
	lay->front = frontlay;
	if (frontlay) frontlay->back = lay;
	
	if (!frontlay) li->top_layer = lay;
	
	
	UnlockLayers(li);
	
    } /* if (lay && rp && damagelist) */
    else
    {
        if (lay) FreeMem(lay, sizeof(struct IntLayer));
	if (rp) FreeRastPort(rp);
	if (damagelist) DisposeRegion(damagelist);
    }
    
    return lay;

    AROS_LIBFUNC_EXIT
    
} /* CreateLayerTagList */

