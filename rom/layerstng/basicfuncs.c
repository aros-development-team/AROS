/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/config.h>
#include <aros/asmcall.h>
#include <exec/memory.h>
#include <graphics/rastport.h>
#include <graphics/clip.h>
#include <graphics/regions.h>
#include <graphics/layers.h>
#include <graphics/gfx.h>
#include <utility/hooks.h>

#include <proto/exec.h>
#include <proto/alib.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/utility.h>

#include "layers_intern.h"
#include "basicfuncs.h"

/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/

/******************************************************************************************
*                                                                                         *
* Allocate LayerInfo_extra and initialize its resource list. Layers uses                  *
* this resource list to keep track of various memory allocations it makes                 *
* for the layers. See ResourceNode and ResData in layers_intern.h for the                 *
* node structure. See AddLayersResource for more information on the basic                 *
* operation.                                                                              *
*                                                                                         *
******************************************************************************************/
 
BOOL AllocExtLayerInfo(struct Layer_Info *li, struct LayersBase *LayersBase)
{
    if(++li->fatten_count != 0) return TRUE;
    if(!(li->LayerInfo_extra = AllocMem(sizeof(struct LayerInfo_extra),MEMF_PUBLIC|MEMF_CLEAR))) return FALSE;

    NewList((struct List *)&((struct LayerInfo_extra *)li->LayerInfo_extra)->lie_ResourceList);

    return TRUE;
}

/*****************************************************************************************/

void FreeExtLayerInfo(struct Layer_Info *li, struct LayersBase *LayersBase)
{
    if(--li->fatten_count >= 0) return;
    if(li->LayerInfo_extra == NULL) return;

    FreeMem(li->LayerInfo_extra, sizeof(struct LayerInfo_extra));

    li->LayerInfo_extra = NULL;
}

/*****************************************************************************************/

ULONG InitLIExtra(struct Layer_Info *li, struct LayersBase * LayersBase)
{
    struct LayerInfo_extra *lie = li->LayerInfo_extra;

    LockLayerInfo(li);

    /*
     * Initialize the ResourceList contained in the LayerInfo_extra.
     * This list is used to keep track of Layers' resource (memory/
     * bitmaps/etc.) allocations.
     */
    NewList((struct List *)&lie->lie_ResourceList);

    return 0;
}

/******************************************************************************************
*                                                                                         *
* Find the Layer in front of which a new Layer must be created depending on               *
* it's pri and the behind flag. Can return NULL which means the layer must                *
* be created behind all other layers                                                      *
*                                                                                         *
******************************************************************************************/

struct Layer *FindBehindLayer(struct Layer_Info *li, WORD pri, BOOL behind)
{
    struct Layer *lay;
    
    lay = li->top_layer;
    if (lay)
    {
        if (behind)
	{
	    /* It's to create a layer behind layers with the same pri */
	    
	    while(lay)
	    {
	        if (IL(lay)->pri < pri) break;
		lay = lay->back;
	    }
	    
	} else {
	    /* It's to create a layer in front of layers with the same pri */
	    
	    while(lay)
	    {
	        if (IL(lay)->pri <= pri) break;
	        lay = lay->back;
	    }
	}
	
    } /* if (lay) */
    
    return lay;    
}

/*****************************************************************************************/

struct ClipRect *AllocClipRect(struct Layer *L, struct LayersBase *LayersBase)
{
    struct ClipRect *CR;

    if (L)
    {
	CR =  L->SuperSaveClipRects;

	if (CR)
	{
	    /* I want to access the list of free ClipRects alone */
	    
	    L->SuperSaveClipRects = CR->Next;

	    CR->Flags  = 0;
	    CR->Next   = NULL; 
	    CR->lobs   = NULL;
	    CR->BitMap = NULL;

	    return CR;
	}
    }
    
    CR = (struct ClipRect *)AllocMem(sizeof(struct ClipRect), MEMF_PUBLIC | MEMF_CLEAR);
    
    return CR;
}

/*****************************************************************************************/

void FreeClipRect(struct ClipRect *CR, struct Layer *L, struct LayersBase *LayersBase)
{
    /* Add the ClipRect to the front of the list */
    
    if (L)
    {
        CR ->Next             = L->SuperSaveClipRects;
        L->SuperSaveClipRects = CR; 
    } else {
        FreeMem(CR, sizeof(struct ClipRect));
    }
}

/*****************************************************************************************/

struct Region *CalcVisibleArea(struct Layer *lay, struct LayersBase *LayersBase)
{
    struct Layer_Info 	*li = lay->LayerInfo;
    struct Region 	*visreg;
    
    visreg = NewRegion();
    
    if (visreg)
    {
        struct Rectangle screenrect;
	struct Layer *checklay;
	
	screenrect.MinX = 0;
	screenrect.MinY = 0;
	screenrect.MaxX = GetBitMapAttr(lay->rp->BitMap, BMA_WIDTH)  - 1;
	screenrect.MaxY = GetBitMapAttr(lay->rp->BitMap, BMA_HEIGHT) - 1;
	
	/* visreg = layer box */
        OrRectRegion(visreg, &lay->bounds);
	
	/* clip away areas which are out of screen */
	AndRectRegion(visreg, &screenrect);
	
	if (IL(lay)->shape)
	{
	    /* if the layer has a (irregular) shape AND visreg with shape */
	    /* Important: Shape Region is in layer coords */
	    
	    TranslateRegion(IL(lay)->shape, lay->bounds.MinX, lay->bounds.MinY);
	    AndRegionRegion(IL(lay)->shape, visreg);
	    TranslateRegion(IL(lay)->shape, -lay->bounds.MinX, -lay->bounds.MinY);
	}
	
	/* Now remove areas from visreg which are hidden by layers in front of lay */
	for(checklay = lay->front; checklay; checklay = checklay->front)
	{
	    if (IS_INVISIBLE(checklay)) continue;
	    if (!DO_INTERSECT(checklay, lay)) continue;
	    
	    if (IL(checklay)->shape)
	    {
	        struct Region *tempreg = NewRegion();

		if (tempreg)
		{
		    struct RegionRectangle *rr;
		    struct Rectangle clearrect;
		    
		    /* layershape could extend outside layerbounds, so
		       we must clip layershape to layerbounds */
		       
		    /* Important: Shape Region is in layer coords */

		    TranslateRegion(IL(checklay)->shape, checklay->bounds.MinX, checklay->bounds.MinY);
		    OrRegionRegion(IL(checklay)->shape, tempreg);
		    TranslateRegion(IL(checklay)->shape, -checklay->bounds.MinX, -checklay->bounds.MinY);

		    AndRectRegion(tempreg, &checklay->bounds);
		    
		    /* Now clear aways tempreg from visreg */
		    rr = tempreg->RegionRectangle;
		    while (rr)
		    {
		        clearrect.MinX = tempreg->bounds.MinX + rr->bounds.MinX;
			clearrect.MinY = tempreg->bounds.MinY + rr->bounds.MinY;
			clearrect.MaxX = tempreg->bounds.MinX + rr->bounds.MaxX;
			clearrect.MaxY = tempreg->bounds.MinY + rr->bounds.MaxY;
			
			ClearRectRegion(visreg, &clearrect);
			
			rr = rr->Next;
		    }
		    
		    DisposeRegion(tempreg);

		} /* if (tempreg) */
		
	    } else {
	        /* Easy case: layer has a normal rectangular shape */
	        ClearRectRegion(visreg, &checklay->bounds);
	    }
	    
	} /* for(checklay = lay->front; checklay; checklay = checklay->front) */
	
    } /* if (visreg) */
    
    return visreg;
    
}

/*****************************************************************************************/

BOOL PointInRegion(struct Region *region, WORD x, WORD y)
{
    struct RegionRectangle *rr;
    
    BOOL rc = FALSE;
    
    if ((x >= region->bounds.MinX) &&
        (y >= region->bounds.MinY) &&
	(x <= region->bounds.MaxX) &&
	(y <= region->bounds.MaxY))
    {
	/* by doing this subtractions here, the if below is more simple */

	x -= region->bounds.MinX;
	y -= region->bounds.MinY;

	for(rr = region->RegionRectangle; rr; rr = rr->Next)
	{
            if ((x >= rr->bounds.MinX) &&
		(y >= rr->bounds.MinY) &&
		(x <= rr->bounds.MaxX) &&
		(y <= rr->bounds.MaxY))
	    {
		rc = TRUE;
		break;
	    }
	}
    }
    
    return rc;
};

/*****************************************************************************************/

void TranslateRegion(struct Region *region, WORD dx, WORD dy)
{
    region->bounds.MinX += dx;
    region->bounds.MinY += dy;
    region->bounds.MaxX += dx;
    region->bounds.MaxY += dy;
}

/*****************************************************************************************/
