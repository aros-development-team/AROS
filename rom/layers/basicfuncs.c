/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Basic support functions for layers.library.
    Lang: English.
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
#include <setjmp.h>

#include <proto/exec.h>
#include <proto/alib.h>
#include <proto/graphics.h>
#include <proto/layers.h>

#include "layers_intern.h"
#include "basicfuncs.h"

#define DEBUG 1
#include <aros/debug.h>
#undef kprintf

/*
 *  Sections:
 *
 *  + Blitter
 *  + Hook
 *  + Layer
 *  + LayerInfo
 *  + Rectangle
 *  + Resource Handling
 *  + Miscellaneous
 *
 */

/***************************************************************************/
/*                                 BLITTER                                 */
/***************************************************************************/

#define CR2NR_NOBITMAP 0
#define CR2NR_BITMAP   1

#if !(AROS_FLAVOUR & AROS_FLAVOUR_NATIVE)
/*
 * These functions cause the infamous "fixed or forbidden register was spilled"
 * bug/feature in m68k gcc, so these were written straight in asm. They can be
 * found in config/m68k-native/layers, for the m68k AROSfA target. Other targets,
 * that use stack passing, can use these versions.
 */

AROS_UFH4(void, BltRPtoCR,
    AROS_UFHA(struct RastPort *,   rp,         A0),
    AROS_UFHA(struct ClipRect *,   cr,         A1),
    AROS_UFHA(ULONG,               Mode,       D0),
    AROS_UFHA(struct LayersBase *, LayersBase, A6))
{
    BltBitMap(rp->BitMap, cr->bounds.MinX, cr->bounds.MinY,
	      cr->BitMap, cr->bounds.MinX & 0xf, 0,
	      cr->bounds.MaxX - cr->bounds.MinX + 1,
	      cr->bounds.MaxY - cr->bounds.MinY + 1,
	      Mode, ~0, NULL);
}

AROS_UFH4(void, BltCRtoRP,
    AROS_UFHA(struct RastPort *,   rp,         A0),
    AROS_UFHA(struct ClipRect *,   cr,         A1),
    AROS_UFHA(ULONG,               Mode,       D0),
    AROS_UFHA(struct LayersBase *, LayersBase, A6))
{
    BltBitMap(cr->BitMap, cr->bounds.MinX & 0xf, 0,
	      rp->BitMap, cr->bounds.MinX, cr->bounds.MinY,
	      cr->bounds.MaxX - cr->bounds.MinX + 1,
	      cr->bounds.MaxY - cr->bounds.MinY + 1,
	      Mode, ~0, NULL);
}

#endif /* if !native */

/***************************************************************************/
/*                                  HOOK                                   */
/***************************************************************************/

struct LayerHookMsg
{
    struct Layer *l;
/*  struct Rectangle rect; (replaced by the next line!) */
    WORD MinX, MinY, MaxX, MaxY;
    LONG OffsetX, OffsetY;
};

AROS_UFH8(void, CallLayerHook,
    AROS_UFHA(struct Hook *,       h,          A2),
    AROS_UFHA(struct Layer *,      l,          D0),
    AROS_UFHA(struct RastPort *,   rp,         A0),
    AROS_UFHA(struct Rectangle *,  r1,         A1),
    AROS_UFHA(struct Rectangle *,  r2,         A3),
    AROS_UFHA(WORD,                BaseX,      D1),
    AROS_UFHA(WORD,                BaseY,      D2),
    AROS_UFHA(struct LayersBase *, LayersBase, A6))
{
    DB2(bug("CallLayerHook(h@$%lx l@$%lx rp@$%lx r1@$%lx r2@$%lx BaseX=%d BaseY=%d\n",
	h, l, rp, r1, r2, BaseX, BaseY));

    if(h == LAYERS_BACKFILL)
    {
	DB2(bug("ClearRect(F %d-%d T %d-%d)\n", r1->MinX, r1->MinY, r1->MaxX, r1->MaxY));

	ClearRect(rp, r1, LayersBase);
    }

    else if(h != LAYERS_NOBACKFILL)
    {
	struct LayerHookMsg lhm =
	{
	    l,
	    r1->MinX,
	    r1->MinY,
	    r1->MaxX,
	    r1->MaxY,
	    r2->MinX - BaseX,
	    r2->MinY - BaseY,
	};

	DB2(bug("Hook(F %d-%d T %d-%d O %d-%d) entry $%lx sub $%lx\n", r1->MinX, r1->MinY, r1->MaxX, r1->MaxY,
	    r2->MinX - BaseX, r2->MinY - BaseY, h->h_Entry, h->h_SubEntry));

	AROS_UFC3(void, h->h_Entry,
	    AROS_UFCA(struct Hook *,         h,    A0),
	    AROS_UFCA(struct RastPort *,     rp,   A2),
	    AROS_UFCA(struct LayerHookMsg *, &lhm, A1)
	);
    }
}

/***************************************************************************/
/*                                 LAYER                                   */
/***************************************************************************/

void SetLayerPriorities(struct Layer_Info * li)
{
  struct Layer * L = li -> top_layer;
  UWORD pri = 10000;
  while (NULL != L)
  {
    L -> priority = pri;
    pri--;
    L = L->back;
  }
}

/***************************************************************************/
/*                               LAYERINFO                                 */
/***************************************************************************/

/*-------------------------------------------------------------------------*/
/*
 * Allocate LayerInfo_extra and initialize its resource list. Layers uses
 * this resource list to keep track of various memory allocations it makes
 * for the layers. See ResourceNode and ResData in layers_intern.h for the
 * node structure. See AddLayersResource for more information on the basic
 * operation.
 */
AROS_UFH2(BOOL, _AllocExtLayerInfo,
    AROS_UFHA(struct Layer_Info *, li,         A0),
    AROS_UFHA(struct LayersBase *, LayersBase, A6))
{
    if(++li->fatten_count != 0)
	return TRUE;

    if(!(li->LayerInfo_extra = AllocMem(sizeof(struct LayerInfo_extra),MEMF_PUBLIC|MEMF_CLEAR)))
	return FALSE;

    NewList((struct List *)&((struct LayerInfo_extra *)li->LayerInfo_extra)->lie_ResourceList);

    return TRUE;
}

/*
 * Free LayerInfo_extra.
 */
AROS_UFH2(void, _FreeExtLayerInfo,
    AROS_UFHA(struct Layer_Info *, li,         A0),
    AROS_UFHA(struct LayersBase *, LayersBase, A6))
{
    if(--li->fatten_count >= 0)
	return;

    if(li->LayerInfo_extra == NULL)
	return;

    FreeMem(li->LayerInfo_extra, sizeof(struct LayerInfo_extra));

    li->LayerInfo_extra = NULL;
}

/*
 * Initialize LayerInfo_extra and save the current environment.
 */
AROS_UFH2(ULONG, _InitLIExtra,
    AROS_UFHA(struct Layer_Info *, li,         A0),
    AROS_UFHA(struct LayersBase *, LayersBase, A6))
{
    struct LayerInfo_extra *lie = li->LayerInfo_extra;

    LockLayerInfo(li);

    /*
     * Initialize the ResourceList contained in the LayerInfo_extra.
     * This list is used to keep track of Layers' resource (memory/
     * bitmaps/etc.) allocations.
     */
    NewList((struct List *)&lie->lie_ResourceList);

    /*
     * Save the current environment, so we can drop back in case of
     * an error.
     */
    // return setjmp(lie->lie_JumpBuf);
    return 0;
}

AROS_UFH2(void, ExitLIExtra,
    AROS_UFHA(struct Layer_Info *, li,         A0),
    AROS_UFHA(struct LayersBase *, LayersBase, A6))
{
    struct LayerInfo_extra *lie = li->LayerInfo_extra;

    DB2(bug("ExitLIEExtra($%lx)\n", li));

    /* Free all resources associated with the layers. */
    FreeLayerResources(li, TRUE, LayersBase);

    UnlockLayerInfo(li);

    DB2(bug("ExitLIEExtra: longjmp ahead\n"));
    longjmp(lie->lie_JumpBuf, 1);
}

/*
 * Dynamically allocate LayerInfo_extra if it isn't already there.
 */
AROS_UFH2(BOOL, SafeAllocExtLI,
    AROS_UFHA(struct Layer_Info *, li,         A0),
    AROS_UFHA(struct LayersBase *, LayersBase, A6))
{
    LockLayerInfo(li);

    /* Check to see if we can ignore the rest of this call. :-) */
    if(li->Flags & NEWLAYERINFO_CALLED)
	return TRUE;

    if(_AllocExtLayerInfo(li, LayersBase))
	return TRUE;

    UnlockLayerInfo(li);

    return FALSE;
}

/*
 * Free LayerInfo_extra if it was dynamically allocated, and unlock the LI.
 */
AROS_UFH2(void, SafeFreeExtLI,
    AROS_UFHA(struct Layer_Info *, li,         A0),
    AROS_UFHA(struct LayersBase *, LayersBase, A6))
{
    if(!(li->Flags & NEWLAYERINFO_CALLED))
	_FreeExtLayerInfo(li, LayersBase);

    UnlockLayerInfo(li);
}

/***************************************************************************/
/*                                RECTANGLE                                */
/***************************************************************************/

/*
  Within the linked list of rectangles search for the rectangle that
  contains the given coordinates.
 */
struct ClipRect * internal_WhichClipRect(struct Layer * L, WORD x, WORD y)
{
  struct ClipRect * CR = L->ClipRect;
  while (NULL != CR)
  {
    if (x >= CR->bounds.MinX &&
        x <= CR->bounds.MaxX &&
        y >= CR->bounds.MinY &&
        y <= CR->bounds.MaxY)
      return CR;
    CR = CR->Next;
  }
  return NULL;
}

/*
 * Clear a Rectangle
 */
AROS_UFH3(void, ClearRect,
    AROS_UFHA(struct RastPort *,   rp,         A0),
    AROS_UFHA(struct Rectangle *,  r,          A1),
    AROS_UFHA(struct LayersBase *, LayersBase, A6))
{
    BltBitMap(rp->BitMap, r->MinX, r->MinY,
	      rp->BitMap, r->MinX, r->MinY,
	      r->MaxX - r->MinX + 1,
	      r->MaxY - r->MinY + 1,
	      0, ~0, NULL);
}

#define MAX(a,b)    ((a) > (b) ? (a) : (b))
#define MIN(a,b)    ((a) < (b) ? (a) : (b))

/*
 * Fill the Result Rectangle with the coordinates of the area that the two
 * input Rectangles have in common, i.e. calculate a cross-section.
 */
AROS_UFH3(void, IntersectRects,
    AROS_UFHA(struct Rectangle *, r1,          A0),
    AROS_UFHA(struct Rectangle *, r2,          A1),
    AROS_UFHA(struct Rectangle *, Result,      A2))
{
    Result->MinX = MAX(r1->MinX,r2->MinX);
    Result->MinY = MAX(r1->MinY,r2->MinY);
    Result->MaxX = MIN(r1->MaxX,r2->MaxX);
    Result->MaxY = MIN(r1->MaxY,r2->MaxY);
}

/*
 * Test if some area of one Rectangle is within the other.
 */
AROS_UFH2(BOOL, Overlap,
    AROS_UFHA(struct Rectangle *, r1, A0),
    AROS_UFHA(struct Rectangle *, r2, A1))
{
    if(r1->MaxX < r2->MinX ||
       r1->MinX > r2->MaxX ||
       r1->MaxY < r2->MinY ||
       r1->MinY > r2->MaxY)
	return FALSE;
    else
	return TRUE;
}

/*
 * Test if one ClipRect is completely inside another.
 */
AROS_UFH2(BOOL, ContainsRect,
    AROS_UFHA(struct Rectangle *, Bound,     A0),
    AROS_UFHA(struct Rectangle *, InnerRect, A1))
{
    if(Bound->MinX <= InnerRect->MinX &&
       Bound->MinY <= InnerRect->MinY &&
       Bound->MaxX >= InnerRect->MaxX &&
       Bound->MaxY >= InnerRect->MaxY)
	return TRUE;
   else
	return FALSE;
}

/*
 * Add a ClipRect to the Layer's ClipRect list.
 */
AROS_UFH3(void, AddClipRect,
    AROS_UFHA(struct Layer *,      l,          A0),
    AROS_UFHA(struct ClipRect *,   cr,         A1),
    AROS_UFHA(struct LayersBase *, LayersBase, A6))
{
    cr->Next = l->ClipRect;

    l->ClipRect = cr;
}

/*
 * Copy an area from one ClipRect to another. Only copy the area that both
 * ClipRects have in common.
 */
AROS_UFH3(void, CopyCR,
    AROS_UFHA(struct ClipRect *,   source,     A0),
    AROS_UFHA(struct ClipRect *,   dest,       A1),
    AROS_UFHA(struct LayersBase *, LayersBase, A6))
{
    struct Rectangle r;

    DB2(bug("CopyCR($%lx, $%lx)\n", source, dest));

    if(!source->BitMap || !dest->BitMap)
	return;

    if(!Overlap(&source->bounds, &dest->bounds))
	return;

    IntersectRects(&source->bounds, &dest->bounds, &r);

    BltBitMap(source->BitMap, r.MinX - (source->bounds.MinX & 0xfff0),
                              r.MinY -  source->bounds.MinY,
              dest->BitMap,   r.MinX - (dest->bounds.MinX & 0xfff0),
                              r.MinY -  dest->bounds.MinY,
                              r.MaxX - r.MinX + 1, r.MaxY - r.MinY + 1,
              0xCA, ~0, NULL);

    DB2(bug("CopyCR: done\n"));
}

/***************************************************************************/
/*                            RESOURCE HANDLING                            */
/***************************************************************************/

/*
 * Add a resource to the LayerInfo resource list, dynamically allocating
 * extra storage space if needed.
 */
AROS_UFH4(BOOL, AddLayersResource,
    AROS_UFHA(struct Layer_Info *, li,         A0),
    AROS_UFHA(void *,              ptr,        A1),
    AROS_UFHA(ULONG,               Size,       D0),
    AROS_UFHA(struct LayersBase *, LayersBase, A6))
{
    struct ResourceNode *rn;
    struct ResData      *rd;

    if(!li)
	return TRUE;

    if(IsListEmpty(&((struct LayerInfo_extra *)li->LayerInfo_extra)->lie_ResourceList))
	if(!(rn = AddLayersResourceNode(li, LayersBase)))
	    return FALSE;

    /* Check to see if there are some entries left */
    if(--rn->rn_FreeCnt < 0)
    {
	/* If all entries are full, we have none left. Logic. :-) */
	rn->rn_FreeCnt = 0;

	/* So we add some more space for resources... */
	if(!(rn = AddLayersResourceNode(li, LayersBase)))
	    return FALSE;

	/* ...and decrement it for the following operations. */
	rn->rn_FreeCnt--;
    }

    rd = rn->rn_FirstFree++;

    rd->ptr  = ptr;
    rd->Size = Size;

    return TRUE;
}

/*
 * Add a new node to the LayerInfo resource list.
 */
AROS_UFH2(struct ResourceNode *, AddLayersResourceNode,
    AROS_UFHA(struct Layer_Info *, li,         A0),
    AROS_UFHA(struct LayersBase *, LayersBase, A6))
{
    struct ResourceNode *rn;

    if(!(rn = (struct ResourceNode *)AllocMem(sizeof(struct ResourceNode), MEMF_ANY)))
	return NULL;

    /*
     * We keep 48 entries in this list. Could change depending on resource
     * allocation going on in Layers. For every n*48 allocations, a new node
     * must be allocated. This can (slightly) slow down operations if this
     * happens a lot.
     */
    rn->rn_FreeCnt   = 48;

    /* Point the cached pointer to the first free vector. */
    rn->rn_FirstFree = &rn->rn_Data[0];

    AddHead((struct List *)&((struct LayerInfo_extra *)li->LayerInfo_extra)->lie_ResourceList, &rn->rn_Link);

    return rn;
}

/*
 * Allocate memory for a ClipRect.
 */
/*
AROS_UFH2(struct ClipRect *, AllocClipRect,
    AROS_UFHA(struct Layer_Info *, li,         A0),
    AROS_UFHA(struct LayersBase *, LayersBase, A6))
{
  return AllocLayerStruct(sizeof(struct ClipRect), MEMF_PUBLIC|MEMF_CLEAR, li, LayersBase);
}
*/

/*
 * Dispose of a ClipRect, free its BitMap if it has one.
 */
AROS_UFH2(void, DisposeClipRect,
    AROS_UFHA(struct ClipRect *,   cr,         A0),
    AROS_UFHA(struct LayersBase *, LayersBase, A6))
{
    if(cr->lobs && cr->BitMap)
	FreeBitMap(cr->BitMap);

    FreeMem(cr, sizeof(struct ClipRect));
}

/*
 * Free the BitMap of a ClipRect.
 */
AROS_UFH2(void, FreeCRBitMap,
    AROS_UFHA(struct ClipRect *,   cr,         A0),
    AROS_UFHA(struct LayersBase *, LayersBase, A6))
{
    struct BitMap *bm;

    DB2(bug("FreeCRBitMap($%lx)\n", cr));

    if((bm = cr->BitMap) == NULL)
	return;

    cr->BitMap = NULL;

    FreeBitMap(bm);

    DB2(bug("FreeCRBitMap: done\n"));
}

/*
 * Allocate memory for a BitMap to be added to a ClipRect.
 */
AROS_UFH3(BOOL, AllocCRBitMap,
    AROS_UFHA(struct Layer *,      l,          A0),
    AROS_UFHA(struct ClipRect *,   cr,         A1),
    AROS_UFHA(struct LayersBase *, LayersBase, A6))
{
    struct BitMap *bm;

    if( (bm = AllocBitMap((cr->bounds.MaxX & ~0xf) - (cr->bounds.MinX & ~0xf) + 16,
			   cr->bounds.MaxY - cr->bounds.MinY + 1,
			   l->rp->BitMap->Depth, BMF_MINPLANES, l->rp->BitMap)) )
    {
	/* If we succeeded, add the bitmap to the layers resource list. */
	if(!AddLayersResource(l->LayerInfo, bm, -2, LayersBase))
	{
	    FreeBitMap(bm);
	    bm = NULL;
	}
    }

    /* Drop back to previous environment if there was a failure. */
    if(!bm)
	ExitLIExtra(l->LayerInfo, LayersBase);

    /* Return error code depending on success or failure. */
    if( (cr->BitMap = bm) )
	return TRUE;
    else
	return FALSE;
}

/*
 * Allocate memory of a given size and enter it into the LayerInfo's
 * resource list.
 */
AROS_UFH4(void *, AllocLayerStruct,
    AROS_UFHA(ULONG,               Size,       D0),
    AROS_UFHA(ULONG,               Flags,      D1),
    AROS_UFHA(struct Layer_Info *, li,         D2),
    AROS_UFHA(struct LayersBase *, LayersBase, A6))
{
    void *mem;

    DB2(bug("AllocLayerStruct($%lx, $%lx, $%lx)\n", Size, Flags, li));

    mem = AllocMem(Size, Flags);

    /* If there is no LayerInfo, this is just a straight AllocMem(). */
    if(li)
    {
	/* But if there is a LI, and there was an error, drop back to the
	   previous environment. */
	if(!mem)
	    ExitLIExtra(li, LayersBase);

	/* If not, enter the memory into the layers resource list. */
	if(!AddLayersResource(li, mem, Size, LayersBase))
	{
	    FreeMem(mem, Size);

	    /* Again, drop back in case of an error. */
	    ExitLIExtra(li, LayersBase);
	}
    }

    DB2(bug("AllocLayerStruct: done\n"));
    return mem;
}

/*
 * Traverse the ResourceList associated with the LayerInfo, and free all
 * allocated resources.
 */
AROS_UFH3(void, FreeLayerResources,
    AROS_UFHA(struct Layer_Info *, li,         A0),
    AROS_UFHA(BOOL,                flag,       D0),
    AROS_UFHA(struct LayersBase *, LayersBase, A6))
{
    struct ResourceNode *rn;
    struct ResData      *rd;
    ULONG                count;

    DB2(bug("FreeLayerResources($%lx, %ld)...", li, flag));

    while( (rn = (struct ResourceNode *)
	RemHead((struct List *)&((struct LayerInfo_extra *)li->LayerInfo_extra)->lie_ResourceList)) )
    {
	if(flag)
	{
	    count = 48 - rn->rn_FreeCnt;

	    for(rd = &rn->rn_Data[0]; count-- != 0; rd++)
	    {
		if(rd->Size == RD_REGION)
		    DisposeRegion(rd->ptr);
		else if(rd->Size == RD_BITMAP)
		    FreeBitMap(rd->ptr);
		else
		    FreeMem(rd->ptr, rd->Size);
	    }

	    FreeMem(rn, sizeof(struct ResourceNode));
	}
    }

    DB2(bug("done\n"));
}

/***************************************************************************/
/*                              MISCELLANEOUS                              */
/***************************************************************************/



/*-----------------------------------END-----------------------------------*/
