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

extern struct ExecBase * SysBase;

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

void BltRPtoCR(struct RastPort *    rp,
               struct ClipRect *    cr,
               ULONG                Mode)
{
    BltBitMap(rp->BitMap, 
              cr->bounds.MinX, 
              cr->bounds.MinY,
	      cr->BitMap, 
	      ALIGN_OFFSET(cr->bounds.MinX), 0,
	      cr->bounds.MaxX - cr->bounds.MinX + 1,
	      cr->bounds.MaxY - cr->bounds.MinY + 1,
	      Mode, 
	      ~0, 
	      NULL);
}

void BltCRtoRP(struct RastPort *   rp,
               struct ClipRect *   cr,
               ULONG               Mode)
{
    BltBitMap(cr->BitMap, 
              ALIGN_OFFSET(cr->bounds.MinX), 
              0,
	      rp->BitMap, 
	      cr->bounds.MinX, 
	      cr->bounds.MinY,
	      cr->bounds.MaxX - cr->bounds.MinX + 1,
	      cr->bounds.MaxY - cr->bounds.MinY + 1,
	      Mode, 
	      ~0, 
	      NULL);
}

#endif /* if !native */

/***************************************************************************/
/*                                  HOOK                                   */
/***************************************************************************/

struct layerhookmsg
{
    struct Layer *l;
/*  struct Rectangle rect; (replaced by the next line!) */
    WORD MinX, MinY, MaxX, MaxY;
    LONG OffsetX, OffsetY;
};

void _CallLayerHook(struct Hook * h,
                    struct RastPort * rp,
                    struct Layer * L,
                    struct Rectangle * R,
                    WORD offsetX,
                    WORD offsetY)
{
  struct BitMap * bm = rp->BitMap;
  if (h == LAYERS_BACKFILL)
  {
    /* Use default backfill, which means that I will clear the area */
    BltBitMap(bm,
              0,
              0,
              bm,
              R->MinX,
              R->MinY,
              R->MaxX - R->MinX + 1,
              R->MaxY - R->MinY + 1,
              0x000,
              0xff,
              NULL);
    /* that's it */
    return;
  }
  
  if (h != LAYERS_NOBACKFILL)
  {
    struct layerhookmsg msg;
    msg.l    = L;
    msg.MinX = R->MinX;
    msg.MinY = R->MinY;
    msg.MaxX = R->MaxX;
    msg.MaxY = R->MaxY;
    msg.OffsetX = offsetX;
    msg.OffsetY = offsetY;
    
    AROS_UFC3(void, h->h_Entry,
        AROS_UFCA(struct Hook *,         h   ,A0),
        AROS_UFCA(struct RastPort *,     rp  ,A2),
        AROS_UFCA(struct layerhookmsg *, &msg,A1)
    );  
  }
  
}         


/***************************************************************************/
/*                                 LAYER                                   */
/***************************************************************************/


struct Layer * internal_WhichLayer(struct Layer * l, WORD x, WORD y)
{
  while(l != NULL)
  {
    if(x >= l->bounds.MinX && x <= l->bounds.MaxX &&
       y >= l->bounds.MinY && y <= l->bounds.MaxY)
	     return l;
    l = l->back;
  }

  return NULL;
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
BOOL _AllocExtLayerInfo(struct Layer_Info * li)
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
void _FreeExtLayerInfo(struct Layer_Info * li)
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
ULONG _InitLIExtra(struct Layer_Info * li,
                   struct LayersBase * LayersBase)
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

void ExitLIExtra(struct Layer_Info * li,
                 struct LayersBase * LayersBase)
{
    struct LayerInfo_extra *lie = li->LayerInfo_extra;

    /* Free all resources associated with the layers. */
    FreeLayerResources(li, TRUE);

    UnlockLayerInfo(li);

    longjmp(lie->lie_JumpBuf, 1);
}

/*
 * Dynamically allocate LayerInfo_extra if it isn't already there.
 */
BOOL SafeAllocExtLI(struct Layer_Info * li,
                    struct LayersBase * LayersBase)
{
    LockLayerInfo(li);

    /* Check to see if we can ignore the rest of this call. :-) */
    if(li->Flags & NEWLAYERINFO_CALLED)
	return TRUE;

    if(_AllocExtLayerInfo(li))
	return TRUE;

    UnlockLayerInfo(li);

    return FALSE;
}

/*
 * Free LayerInfo_extra if it was dynamically allocated, and unlock the LI.
 */
void SafeFreeExtLI(struct Layer_Info * li,
                   struct LayersBase * LayersBase)
{
    if(!(li->Flags & NEWLAYERINFO_CALLED))
	_FreeExtLayerInfo(li);

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

#define MAX(a,b)    ((a) > (b) ? (a) : (b))
#define MIN(a,b)    ((a) < (b) ? (a) : (b))



/***************************************************************************/
/*                            RESOURCE HANDLING                            */
/***************************************************************************/

/*
 * Add a resource to the LayerInfo resource list, dynamically allocating
 * extra storage space if needed.
 */
BOOL AddLayersResource(struct Layer_Info * li,
                       void *              ptr,
                       ULONG               Size)
{
    struct ResourceNode *rn;
    struct ResData      *rd;

    if(!li)
	return TRUE;

    if(IsListEmpty(&((struct LayerInfo_extra *)li->LayerInfo_extra)->lie_ResourceList))
	if(!(rn = AddLayersResourceNode(li)))
	    return FALSE;

    /* Check to see if there are some entries left */
    if(--rn->rn_FreeCnt < 0)
    {
	/* If all entries are full, we have none left. Logic. :-) */
	rn->rn_FreeCnt = 0;

	/* So we add some more space for resources... */
	if(!(rn = AddLayersResourceNode(li)))
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
struct ResourceNode * AddLayersResourceNode(struct Layer_Info * li)
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

struct ClipRect * _AllocClipRect(struct Layer * L)
{
  struct ClipRect * CR;
  
  CR =  L->SuperSaveClipRects;

  if (NULL != CR)
  {
   /* I want to access the list of free ClipRects alone */
    L->SuperSaveClipRects = CR->Next;
    L->SuperSaveClipRectCounter--;

    CR->Flags  = 0;
    CR->Next   = NULL; 
    CR->lobs   = NULL;
    CR->BitMap = NULL;
    return CR;
  }
  
  CR = (struct ClipRect *) AllocMem(sizeof(struct ClipRect), MEMF_PUBLIC|MEMF_CLEAR);
  return CR;
}

/*
 * Return memory of a ClipRect for later use.
 */

void _FreeClipRect(struct ClipRect   * CR,
                   struct Layer      * L)
{
  if (L->SuperSaveClipRectCounter < MAXSUPERSAVECLIPRECTS)
  {
    /* Add the ClipRect to the front of the list */
    CR -> Next = L -> SuperSaveClipRects;
    L -> SuperSaveClipRects = CR;
    L -> SuperSaveClipRectCounter++;
  }
  else
    FreeMem(CR, sizeof(struct ClipRect));
}

/*
 * Free a whole list of cliprects including the allocated bitmaps (if any)
 */

void _FreeClipRectListBM(struct Layer * L,
                         struct ClipRect * CR)
{
  struct ClipRect * _CR = CR;
  BOOL isSmart; 
  if ((L->Flags & (LAYERSUPER|LAYERSMART)) == LAYERSMART)
    isSmart = TRUE;
  else
    isSmart = FALSE;
 
  /*
   * This function is not watching for the upper limit of 
   * pre allocated cliprects.
   */
  L->SuperSaveClipRectCounter++;
  
  while (TRUE)
  {
    if (NULL != _CR->BitMap && TRUE == isSmart)
    {
      FreeBitMap(_CR->BitMap);
      _CR->BitMap = NULL;
    }
    if (NULL != _CR->Next)
    {
      L->SuperSaveClipRectCounter++;
      _CR = _CR->Next;
    }
    else
      break;
  }
  /* _CR is the last ClipRect in the list. I concatenate the
     currently preallocated list of ClipRects with that list. */
  _CR->Next = L->SuperSaveClipRects;
  
  /* CR is the head of the ClipRect list now */
  L->SuperSaveClipRects = CR;
}

/*
 * Allocate memory of a given size and enter it into the LayerInfo's
 * resource list.
 */
void * AllocLayerStruct(ULONG               Size,
                        ULONG               Flags,
                        struct Layer_Info * li,
                        struct LayersBase * LayersBase)
{
    void *mem;

    mem = AllocMem(Size, Flags);

    /* If there is no LayerInfo, this is just a straight AllocMem(). */
    if(li)
    {
	/* But if there is a LI, and there was an error, drop back to the
	   previous environment. */
	if(!mem)
	    ExitLIExtra(li, LayersBase);

	/* If not, enter the memory into the layers resource list. */
	if(!AddLayersResource(li, mem, Size))
	{
	    FreeMem(mem, Size);

	    /* Again, drop back in case of an error. */
	    ExitLIExtra(li, LayersBase);
	}
    }

    return mem;
}

/*
 * Traverse the ResourceList associated with the LayerInfo, and free all
 * allocated resources.
 */
void FreeLayerResources(struct Layer_Info * li,
                        BOOL                flag)
{
    struct ResourceNode *rn;
    struct ResData      *rd;
    ULONG                count;

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

}

/***************************************************************************/
/*                              MISCELLANEOUS                              */
/***************************************************************************/

/*
 * Makes sure that the top most layer only consists of one cliprect 
 */


/* 
 * When a clipregion is installed a secondary list of cliprects
 * exists that represents those parts where the paint operations may
 * be performed on.
 * This function copies all BitMaps of the cliprect list where
 * the region is into  the usual list of cliprects.
 * It also frees the bitmaps and cliprects of the region cliprect
 * list.
 */

void CopyAndFreeClipRectsClipRects(struct Layer * L,
                                   struct ClipRect * srcCR,
                                   struct ClipRect * destCR)
{
  struct ClipRect * sCR = srcCR;

  /* if it's a SUPERBITMAP layer then just free the cliprects */  
  if (LAYERSUPER == (L->Flags & (LAYERSMART|LAYERSUPER)))
  {
    _FreeClipRectListBM(L, L->ClipRect);
    return;
  }
  
  while (TRUE)
  {
    /* only copy from ClipRects that are hidden as the visible
       ones have all info in the screens rastport anyway 
    */
    if (NULL != sCR->lobs)
    {
      struct ClipRect * dCR = destCR;
      int area = (sCR->bounds.MaxX - sCR->bounds.MinX + 1) *
                 (sCR->bounds.MaxY - sCR->bounds.MinY + 1);
      int areacopied = 0;
                 
      while (NULL != dCR && areacopied != area)
      {   
        if (! (sCR->bounds.MinX > dCR->bounds.MaxX ||
               sCR->bounds.MinY > dCR->bounds.MaxY ||
               sCR->bounds.MaxX < dCR->bounds.MinX ||
               sCR->bounds.MaxY < dCR->bounds.MinY) &&
               NULL != dCR->BitMap)
        { 
          /* these two overlap */
          int a;
          ULONG srcX, srcY;
          ULONG destX, destY;
          ULONG width, height;
          
          width = sCR->bounds.MaxX - sCR->bounds.MinX + 1;
          height= sCR->bounds.MaxY - sCR->bounds.MinY + 1;
          
          if (sCR->bounds.MinX > dCR->bounds.MinX)
          {
            srcX  = ALIGN_OFFSET(sCR->bounds.MinX);
            destX = sCR->bounds.MinX - dCR->bounds.MinX + ALIGN_OFFSET(dCR->bounds.MinX);
          }
          else
          {
            srcX   = dCR->bounds.MinX - sCR->bounds.MinX + ALIGN_OFFSET(sCR->bounds.MinX);
            destX  = ALIGN_OFFSET(dCR->bounds.MinX);
            width -= (dCR->bounds.MinX - sCR->bounds.MinX);
          }
          
          if (sCR->bounds.MinY > dCR->bounds.MinY)
          {
            srcY  = 0;
            destY = sCR->bounds.MinY - dCR->bounds.MinY;
          }
          else
          {
            srcY    = dCR->bounds.MinY - sCR->bounds.MinY;
            destY   = 0;
            height -= srcY;
          }
          
          if (sCR->bounds.MaxX > dCR->bounds.MaxX)
            width  -= (sCR->bounds.MaxX - dCR->bounds.MaxX);
            
          if (sCR->bounds.MaxY > dCR->bounds.MaxY)
            height -= (sCR->bounds.MaxY - dCR->bounds.MaxY);
            
          a = width * height;
           
          if (a == area && 
              dCR->bounds.MaxX - dCR->bounds.MinX + 1 == width &&
              dCR->bounds.MaxY - dCR->bounds.MinY + 1 == height )
          {
            FreeBitMap(dCR->BitMap);
            dCR -> BitMap = sCR -> BitMap;
            break;
          }
          else
          {
            areacopied += a;
            
            BltBitMap(sCR->BitMap,
            	      srcX,
            	      srcY,
            	      dCR->BitMap,
            	      destX,
            	      destY,
            	      width,
            	      height,
            	      0x0c0,
            	      0xff,
            	      NULL);
            FreeBitMap(sCR->BitMap);
          }
        }
        dCR = dCR -> Next;
      } /* walk through all destination ClipRects */
    } /* if (NULL != sCR->lobs) */
  
    if (NULL == sCR->Next)
      break;
  
    sCR = sCR -> Next;
  }
  
  sCR->Next = L->SuperSaveClipRects;
  L->SuperSaveClipRects = srcCR;
  
}

/* 
** Uninstalls the ClipRegion ClipRects form all the layers that are
** found, if they haven't alreay been uninstalled.
*/

void UninstallClipRegionClipRects(struct Layer_Info * LI)
{
  struct Layer * L = LI->top_layer;
  while (NULL != L)
  {
    /* does this one have a ClipRegion and are the ClipRegion ClipRects 
       still installed? 
    */
    if (NULL != L->ClipRegion && NULL != L->_cliprects)
    {
      CopyAndFreeClipRectsClipRects(L, L->ClipRect, L->_cliprects);
      /* 
      ** make the regular (not clipped cliprects) cliprects the
      ** actual cliprects.
      */
      L->ClipRect = L->_cliprects;
      L->_cliprects = NULL;
    }
    L = L->back;
  }
}


/*-----------------------------------END-----------------------------------*/


/*
 * Backup any parts of the layer that overlap with the backup_region
 * and that are not already backed up. Create the cliprects and
 * bitmaps if necessary.
 * Assumption: Only visible parts become invisible,
 *             invisible parts will not become visible.
 *
 * This function MUST not manipulate hide_region!!!!
 */
int _BackupPartsOfLayer(struct Layer * l, 
                        struct Region * hide_region,
                        int dx,
                        int backupsimplerefresh)
{
  struct ClipRect * cr, * firstcr = NULL, * oldcr;
  struct RegionRectangle * rr;
  struct Region * r = NewRegion();
  struct BitMap * display_bm = l->rp->BitMap; 
  int invisible = FALSE;
#ifdef CHECKSIZE
  int size = 0;
#endif


#if 0
kprintf("\t %s: l=%p\n",
        __FUNCTION__,
        l);  
#endif
#warning Write function to copy a region

  OrRegionRegion(l->VisibleRegion, r);
  ClearRegionRegion(hide_region,r);
  AndRegionRegion(l->shape,r);
  AndRegionRegion(l->parent->shape,r);
  
  /*
   * From region r create cliprects
   */
  while (1)
  {
    rr = r->RegionRectangle;
    while (rr)
    {
      cr = _AllocClipRect(l);
      cr->bounds.MinX = rr->bounds.MinX + r->bounds.MinX;
      cr->bounds.MinY = rr->bounds.MinY + r->bounds.MinY;
      cr->bounds.MaxX = rr->bounds.MaxX + r->bounds.MinX;
      cr->bounds.MaxY = rr->bounds.MaxY + r->bounds.MinY;
      cr->lobs  = invisible;
      cr->Next  = firstcr;


#if 1
kprintf("\t\t%s: Created cliprect %d/%d-%d/%d invisible: %d\n",
        __FUNCTION__,
        cr->bounds.MinX,
        cr->bounds.MinY,
        cr->bounds.MaxX,
        cr->bounds.MaxY,
        invisible);
#endif

      firstcr = cr;

      rr = rr->Next;
    }

    if (FALSE == invisible)
    {
      /*
       * Flip the shape to the visible part
       */
      XorRegionRegion(l->shape,r);
      invisible = TRUE;
    }
    else
      break;
  }
  
  DisposeRegion(r);

  /*
   * firstcr holds all new cliprects.
   * flags = TRUE means that that cr will be visible.
   */
  oldcr = l->ClipRect;

  while (NULL != oldcr)
  {
    struct ClipRect * _cr = firstcr;
    int area = RECTAREA(&oldcr->bounds);
    while ((NULL != _cr)  && (0 != area) )
    {
      /*
       * Do the two rectangles overlap?
       */
      if (DO_OVERLAP(&_cr->bounds,&oldcr->bounds))
      {
        /*
         * Is this new one supposed to be invisible?
         */
        if (NULL != _cr->lobs)
        {
          /*
           * It is now invisible
           */
          struct BitMap * srcbm;
          if (IS_SIMPLEREFRESH(l) && 
                 (FALSE == backupsimplerefresh))
          {
              struct Rectangle rect;
              rect.MinX = _cr->bounds.MinX - l->bounds.MinX;
              rect.MinY = _cr->bounds.MinY - l->bounds.MinY;
              rect.MaxX = _cr->bounds.MaxX - l->bounds.MinX;
              rect.MaxY = _cr->bounds.MaxY - l->bounds.MinY;

#if 1
kprintf("%s: Subtracting %d/%d-%d/%d from damagelist!\n",
        __FUNCTION__,
        rect.MinX,
        rect.MinY,
        rect.MaxX,
        rect.MaxY
        );
kprintf("%s: Layer: %d/%d-%d/%d!\n",
        __FUNCTION__,
        l->bounds.MinX,
        l->bounds.MinY,
        l->bounds.MaxX,
        l->bounds.MaxY
        );
kprintf("%s: _cr: %d/%d-%d/%d!\n",
        __FUNCTION__,
        _cr->bounds.MinX,
        _cr->bounds.MinY,
        _cr->bounds.MaxX,
        _cr->bounds.MaxY
        );
#endif
              ClearRectRegion(l->DamageList, &rect);
              l->Flags |= LAYERREFRESH;
          }
          else
          {
            LONG xSrc, xDest;
            LONG ySrc, yDest;
            LONG xSize, ySize;
            
            xSize = oldcr->bounds.MaxX - oldcr->bounds.MinX + 1;
            ySize = oldcr->bounds.MaxY - oldcr->bounds.MinY + 1;

            /*
             * Does the source rect have a bitmap (off screen)
             * or is it on the screen.
             */
            if (oldcr->BitMap)
            {
              /*
               * Copy from hidden BitMap to hidden BitMap
               */
              xSrc = (oldcr->bounds.MinX - _cr->bounds.MinX);
              if (xSrc < 0)
              {
                /*
                 * oldcr is further to the left
                 */
                xSize += xSrc;
                xSrc = -xSrc + ALIGN_OFFSET(oldcr->bounds.MinX);
                xDest = ALIGN_OFFSET(_cr->bounds.MinX+dx);
              }
              else
              {
                /*
                 * oldcr is further to the right 
                 */
                xDest = xSrc + ALIGN_OFFSET(_cr->bounds.MinX + dx);
                xSrc  = ALIGN_OFFSET(oldcr->bounds.MinX);
              }
              
              ySrc = (oldcr->bounds.MinY - _cr->bounds.MinY);
              if (ySrc < 0)
              {
                ySize += ySrc;
                ySrc = -ySrc;
                yDest = 0;
              }
              else
              {
                yDest = ySrc;
                ySrc = 0;
              }
//kprintf("Using olc cr's BitMap!\n");
              srcbm = oldcr->BitMap;
            }
            else
            {
              /*
               * Copy from screen to hidden bitmap
               */
              xSrc = _cr->bounds.MinX;
              ySrc = _cr->bounds.MinY;
              xDest = ALIGN_OFFSET(_cr->bounds.MinX + dx);
              yDest = 0;
              
              if (oldcr->bounds.MinX < _cr->bounds.MinX)
                xSize -= (_cr->bounds.MinX - oldcr->bounds.MinX);

              if (oldcr->bounds.MinY < _cr->bounds.MinY)
                ySize -= (_cr->bounds.MinY - oldcr->bounds.MinY);
              
              srcbm = l->rp->BitMap;
//kprintf("Using bitmap of screen!\n");
            }

            if (oldcr->bounds.MaxX > _cr->bounds.MaxX)
              xSize = xSize - (oldcr->bounds.MaxX - _cr->bounds.MaxX);

            if (oldcr->bounds.MaxY > _cr->bounds.MaxY)
              ySize = ySize - (oldcr->bounds.MaxY - _cr->bounds.MaxY);

            
            if (!IS_SUPERREFRESH(l))
            {
              /*
               * Get a bitmap (if not there) and make a backup
               */
              if (NULL == _cr->BitMap)
              {
//kprintf("Alloc bitmap!\n");
                _cr->BitMap = AllocBitMap(
                   _cr->bounds.MaxX - _cr->bounds.MinX + 1 + 16,
                   _cr->bounds.MaxY - _cr->bounds.MinY + 1,
                   display_bm->Depth,
                   BMF_CLEAR,
                   display_bm);
              }
            }

            BltBitMap(srcbm,
                      xSrc,
                      ySrc,
                      _cr->BitMap,
                      xDest,
                      yDest,
                      xSize,
                      ySize,
                      0x0c0,
                      0xff,
                      NULL);
#if 0
kprintf("\t\t %s backing up: from %d/%d to %d/%d  width:%d, height: %d\n",
        __FUNCTION__,
        xSrc,
        ySrc,
        xDest,
        yDest,
        xSize,
        ySize);
#endif

            area-= (xSize * ySize);
            
          }
        }
        
      } /* if the two cliprects overlap */
      _cr = _cr->Next;
    }
    
    _cr = oldcr->Next;
    if (oldcr->BitMap)
      FreeBitMap(oldcr->BitMap);
    _FreeClipRect(oldcr, l);
    oldcr = _cr;
  }
  
  l->ClipRect = firstcr;

  /*
   * The hide region must be subtracted from
   * the visible region of this layer.
   */
  ClearRegionRegion(hide_region, l->VisibleRegion);

  return TRUE;
}

/*
 * Show any parts of the layer that overlap with the backup_region
 * and that are not already show.
 *
 * This function MUST not manipulate show_region!!!!
 */
int _ShowPartsOfLayer(struct Layer * l, 
                      struct Region * show_region)
{
  struct ClipRect * cr, * firstcr = NULL, * oldcr;
  struct RegionRectangle * rr;
  struct Region * r = NewRegion();
  struct BitMap * display_bm = l->rp->BitMap;
  int invisible = FALSE;
  
//kprintf("%s called for %p\n",__FUNCTION__,l);
#warning Write function to copy a region
  OrRegionRegion(show_region,r);
  OrRegionRegion(l->VisibleRegion,r);
  AndRegionRegion(l->shape,r);
  AndRegionRegion(l->parent->shape,r);
  
  /*
   * From region r create cliprects
   */
  while (1)
  {
    rr = r->RegionRectangle;
    while (rr)
    {
      cr = _AllocClipRect(l);
      cr->bounds.MinX = rr->bounds.MinX + r->bounds.MinX;
      cr->bounds.MinY = rr->bounds.MinY + r->bounds.MinY;
      cr->bounds.MaxX = rr->bounds.MaxX + r->bounds.MinX;
      cr->bounds.MaxY = rr->bounds.MaxY + r->bounds.MinY;
      cr->lobs  = invisible;
      cr->Next  = firstcr;
#if 0
kprintf("\t\t%s: Created cliprect %d/%d-%d/%d invisible: %d\n",
        __FUNCTION__,
        cr->bounds.MinX,
        cr->bounds.MinY,
        cr->bounds.MaxX,
        cr->bounds.MaxY,
        invisible);
#endif

      firstcr = cr;

      rr = rr->Next;
    }

    if (FALSE == invisible)
    {
      /*
       * Flip the shape to the visible part
       */
      XorRegionRegion(l->shape,r);
      invisible = TRUE;
    }
    else
      break;
  }
  
  DisposeRegion(r);
  /*
   * firstcr holds all new cliprects.
   * lobs = TRUE means that that cr will be visible.
   */
  oldcr = l->ClipRect;

  while (NULL != oldcr)
  {
    struct ClipRect * _cr = firstcr;
    int area = RECTAREA(&oldcr->bounds);
    while (NULL != _cr /* &&  0 != area */)
    {
      /*
       * Do the two rectangles overlap?
       */
#if 0
kprintf("%s: oldcr: %d/%d - %d/%d\n",
        __FUNCTION__,
        oldcr->bounds.MinX,
        oldcr->bounds.MinY,
        oldcr->bounds.MaxX,
        oldcr->bounds.MaxY
        );

#endif
      if (DO_OVERLAP(&_cr->bounds,&oldcr->bounds))
      {
        /*
         * Is this new one supposed to be invisible?
         */
        if (NULL != _cr->lobs)
        {
          struct BitMap * srcbm;
          if (IS_SIMPLEREFRESH(l))
          {
            struct Rectangle rect;
            rect.MinX = _cr->bounds.MinX - l->bounds.MinX;
            rect.MinY = _cr->bounds.MinY - l->bounds.MinY;
            rect.MaxX = _cr->bounds.MaxX - l->bounds.MinX;
            rect.MaxY = _cr->bounds.MaxY - l->bounds.MinY;
#if 1
kprintf("%s: Subtracting %d/%d-%d/%d from damagelist!\n",
        __FUNCTION__,
        rect.MinX,
        rect.MinY,
        rect.MaxX,
        rect.MaxY
        );
#endif
            ClearRectRegion(l->DamageList, &rect);
            l->Flags |= LAYERREFRESH;
          }
          else
          {
            LONG xSrc, xDest;
            LONG ySrc, yDest;
            LONG xSize, ySize;
            
            xSize = oldcr->bounds.MaxX - oldcr->bounds.MinX + 1;
            ySize = oldcr->bounds.MaxY - oldcr->bounds.MinY + 1;

            /*
             * Does the source rect have a bitmap (off screen)
             * or is it on the screen.
             */            
            if (oldcr->BitMap)
            { 
              /*
               * Copy from hidden bitmap to hidden bitmap
               */
              xSrc = (oldcr->bounds.MinX - _cr->bounds.MinX);
              if (xSrc < 0)
              {
                /*
                 * oldcr is further to the left
                 */
                xSize += xSrc;
                xSrc = -xSrc + ALIGN_OFFSET(oldcr->bounds.MinX);
                xDest = ALIGN_OFFSET(_cr->bounds.MinX);
              }
              else
              {
                /*
                 * oldcr is further to the right
                 */
                xDest = xSrc + ALIGN_OFFSET(_cr->bounds.MinX);
                xSrc = ALIGN_OFFSET(oldcr->bounds.MinX);
              }
              
              ySrc = (oldcr->bounds.MinY - _cr->bounds.MinY);
              if (ySrc < 0)
              {
                ySize += ySrc;
                ySrc = -ySrc;
                yDest = 0;
              }
              else
              {
                yDest = ySrc;
                ySrc = 0;
              }
//kprintf("Using old cr's BitMap!\n");
              srcbm = oldcr->BitMap;
            }
            else
            {
              /*
               * Copy from screen.
               */
              xSrc = _cr->bounds.MinX;
              ySrc = _cr->bounds.MinY;
              xDest = ALIGN_OFFSET(_cr->bounds.MinX);
              yDest = 0;

              if (oldcr->bounds.MinX < _cr->bounds.MinX)
                xSize -= (_cr->bounds.MinX - oldcr->bounds.MinX);

              if (oldcr->bounds.MinY < _cr->bounds.MinY)
                ySize -= (_cr->bounds.MinY - oldcr->bounds.MinY);
              
              srcbm = l->rp->BitMap;
//kprintf("Using bitmap of screen!\n");
            }

            if (oldcr->bounds.MaxX > _cr->bounds.MaxX)
              xSize -= (oldcr->bounds.MaxX - _cr->bounds.MaxX);

            if (oldcr->bounds.MaxY > _cr->bounds.MaxY)
              ySize -= (oldcr->bounds.MaxY - _cr->bounds.MaxY);

            if (!IS_SUPERREFRESH(l))
            {
              /*
               * Get a bitmap (if not there) and make a backup
               */
              if (NULL == _cr->BitMap)
              {
//kprintf("Alloc bitmap!\n");
                _cr->BitMap = AllocBitMap(
                   _cr->bounds.MaxX - _cr->bounds.MinX + 1 + 16 ,
                   _cr->bounds.MaxY - _cr->bounds.MinY + 1,
                   display_bm->Depth,
                   BMF_CLEAR,
                   display_bm);
              }
            }

            BltBitMap(srcbm,
                      xSrc,
                      ySrc,
                      _cr->BitMap,
                      xDest,
                      yDest,
                      xSize,
                      ySize,
                      0x0c0,
                      0xff,
                      NULL);
#if 0
kprintf("%s: backing up: from %d/%d to %d/%d  width:%d, height: %d\n",
        __FUNCTION__,
        xSrc,
        ySrc,
        xDest,
        yDest,
        xSize,
        ySize);
#endif

            area-= (xSize * ySize);
            
          }
        }
        else
        {
          /*
           * The new one is visible. if the old one was not visible
           * then I have to show it.
           */
if (IS_SIMPLEREFRESH(l))
  kprintf("Simple REFRESH!\n");

          if (IS_SIMPLEREFRESH(l) && (NULL == oldcr->BitMap))
          {
              struct Rectangle rect;
              rect.MinX = _cr->bounds.MinX - l->bounds.MinX;
              rect.MinY = _cr->bounds.MinY - l->bounds.MinY;
              rect.MaxX = _cr->bounds.MaxX - l->bounds.MinX;
              rect.MaxY = _cr->bounds.MaxY - l->bounds.MinY;
#if 1
kprintf("%s: Adding %d/%d-%d/%d to damagelist!\n",
        __FUNCTION__,
        rect.MinX,
        rect.MinY,
        rect.MaxX,
        rect.MaxY
        );
#endif
              OrRectRegion(l->DamageList, &rect);
              l->Flags |= LAYERREFRESH;
              
              _CallLayerHook(l->BackFill,
                             l->rp,
                             l,
                             &_cr->bounds,
                             _cr->bounds.MinX,
                             _cr->bounds.MinY);
          }
          else
          {
//kprintf("%s: Showing a part of a backed up bitmap!\n",__FUNCTION__);
            if (NULL != oldcr->lobs)
            {
              LONG xSrc, xDest;
              LONG ySrc, yDest;
              LONG xSize, ySize;
           
              xSize = oldcr->bounds.MaxX - oldcr->bounds.MinX + 1;
              ySize = oldcr->bounds.MaxY - oldcr->bounds.MinY + 1;
              /*
               * I have to make the old one visible
               * two cases left: SMART REFRESH and SUPERBITMAP
               */

              if (!IS_SUPERREFRESH(l))
              {
                /*
                 * Copy from hidden BitMap to screen!
                 * If a simple refresh layer is moved it might
                 * also have a BitMap!!!
                 */
                xSrc = (oldcr->bounds.MinX - _cr->bounds.MinX);
                if (xSrc < 0)
                {
                  /*
                   * old cr is further to the left
                   */
                  xSize += xSrc;
                  xSrc   = -xSrc + ALIGN_OFFSET(oldcr->bounds.MinX);
                  xDest  = _cr->bounds.MinX;
                }
                else
                {
                  /*
                   * oldcr is further to the right
                   */
                  xDest = oldcr->bounds.MinX;
                  xSrc  = ALIGN_OFFSET(oldcr->bounds.MinX);
                }
                
                ySrc = (oldcr->bounds.MinY - _cr->bounds.MinY);
                if (ySrc < 0)
                {
                  ySize += ySrc;
                  ySrc   = -ySrc;
                  yDest = _cr->bounds.MinY;
                }
                else
                {
                  yDest = oldcr->bounds.MinY;
                  ySrc  = 0;
                }
              }
              else
              {
kprintf("!!!!!!!!!!!!! MISSING !!!!!!!!!!!!!!\n");
#warning Missing, maybe the same as above.
              }

              if (oldcr->bounds.MaxX > _cr->bounds.MaxX)
                xSize -= (oldcr->bounds.MaxX - _cr->bounds.MaxX);

              if (oldcr->bounds.MaxY > _cr->bounds.MaxY)
                ySize -= (oldcr->bounds.MaxY - _cr->bounds.MaxY);
#if 1
kprintf("\t\t%s: Show cliprect: %d/%d-%d/%d; blitting to %d/%d\n",
        __FUNCTION__,
        oldcr->bounds.MinX,
        oldcr->bounds.MinY,
        oldcr->bounds.MaxX,
        oldcr->bounds.MaxY,
        xDest,
        yDest);
#endif

              BltBitMap(oldcr->BitMap,
                        xSrc,
                        ySrc,
                        l->rp->BitMap,
                        xDest,
                        yDest,
                        xSize,
                        ySize,
                        0x0c0,
                        0xff,
                        NULL);
              
            } /* if was hidden cliprect */
          } /* if is simple else ... */
        } /* if new cliprect is visible or invisible   */
      } /* if rectangles overlap */
      _cr = _cr->Next;
    } /* all new cliprects */
    
    _cr = oldcr->Next;
    if (oldcr->BitMap)
      FreeBitMap(oldcr->BitMap);
    _FreeClipRect(oldcr, l);
    oldcr = _cr;
  } /* for all old cliprects */
  
  l->ClipRect = firstcr;

  /*
   * The hid region must be subtracted from
   * the visible region of this layer.
   */
  OrRegionRegion(show_region, l->VisibleRegion);

  return TRUE;
}

int _ShowLayer(struct Layer * l)
{
  struct Region * r = NewRegion();
  struct RegionRectangle * rr;
  struct ClipRect * prevcr = NULL;
  struct BitMap * bm = l->rp->BitMap;
  int invisible = FALSE;
  if (NULL == r)
    return FALSE;
    
   OrRegionRegion(l->VisibleRegion, r);
  AndRegionRegion(l->shape, r);

  while (1)
  {
    rr = r->RegionRectangle;

if (NULL == rr)
  kprintf("\t\t empty region! invisible: %d\n",invisible);

    while (NULL != rr)
    {
      struct ClipRect * cr = AllocMem(sizeof(struct ClipRect), MEMF_CLEAR);

//kprintf("\t\tinvisible: %d !!!!!!!!!!!!\n",invisible);

      cr->bounds.MinX = rr->bounds.MinX + r->bounds.MinX;
      cr->bounds.MinY = rr->bounds.MinY + r->bounds.MinY;
      cr->bounds.MaxX = rr->bounds.MaxX + r->bounds.MinX;
      cr->bounds.MaxY = rr->bounds.MaxY + r->bounds.MinY;
      cr->lobs = invisible;
#if 0
kprintf("\t\t%s: Created cliprect %d/%d-%d/%d invisible: %d\n",
        __FUNCTION__,
        cr->bounds.MinX,
        cr->bounds.MinY,
        cr->bounds.MaxX,
        cr->bounds.MaxY,
        invisible);
#endif      
      if (prevcr)
        prevcr->Next = cr;
      else
        l->ClipRect = cr;
       
      prevcr = cr;
      
      if (FALSE == invisible)
      {
#if 0
kprintf("\t\tClearing background! %d/%d-%d/%d  bitmap: %p\n",
         cr->bounds.MinX,
         cr->bounds.MinY,
         cr->bounds.MaxX,
         cr->bounds.MaxY,
         l->rp->BitMap
         );
#endif
          _CallLayerHook(l->BackFill,
                         l->rp,
                         l,
                         &cr->bounds,
                         cr->bounds.MinX,
                         cr->bounds.MinY);
      }
      else
      {
        /*
         * This part is to be invisible!
         */
        if (IS_SMARTREFRESH(l))
        {
          cr->BitMap = AllocBitMap(
             cr->bounds.MaxX - cr->bounds.MinX + 1 + 16,
             cr->bounds.MaxY - cr->bounds.MinY + 1,
             bm->Depth,
             BMF_CLEAR,
             bm);
        }
      }
      
      rr=rr->Next;
    }
    
    if (FALSE == invisible)
    {
      XorRegionRegion(l->shape, r);
      invisible = TRUE;
    }
    else
      break;
  }
  
  return TRUE;
}


int ClearRegionRegion(struct Region * rd, struct Region * r)
{
  struct RegionRectangle * rr = rd->RegionRectangle;
  while (rr) 
  {
    struct Rectangle rect;
    rect.MinX = rd->bounds.MinX + rr->bounds.MinX;   
    rect.MinY = rd->bounds.MinY + rr->bounds.MinY;   
    rect.MaxX = rd->bounds.MinX + rr->bounds.MaxX;   
    rect.MaxY = rd->bounds.MinY + rr->bounds.MaxY;   
    ClearRectRegion(r, &rect);
    rr = rr->Next;
  }
  
  return TRUE;
}

struct Layer * _FindFirstFamilyMember(struct Layer * l)
{
  struct Layer * lastgood = l, *_l = l->front;
  
  while ((NULL != _l) && (_l->nesting > l->nesting))
  {
    lastgood = _l;
    _l = _l->front;
  }
  return lastgood;
}

/*
 * It is assumed that the region r is not needed anymore.
 */
void _BackFillRegion(struct Layer * l, 
                     struct Region * r)
{
  struct RegionRectangle * RR = r->RegionRectangle;
  /* check if a region is empty */
  while (NULL != RR)
  {
     RR->bounds.MinX += r->bounds.MinX;
     RR->bounds.MinY += r->bounds.MinY;
     RR->bounds.MaxX += r->bounds.MinX;
     RR->bounds.MaxY += r->bounds.MinY;

kprintf("\t\t: %s Clearing rect : %d/%d-%d/%d  layer: %p, hook: %p, bitmap: %p\n",
        __FUNCTION__,
        RR->bounds.MinX,
        RR->bounds.MinY,
        RR->bounds.MaxX,
        RR->bounds.MaxY,
        l,
        l->BackFill,
        l->rp->BitMap);

     _CallLayerHook(l->BackFill,
            	    l->rp,
            	    l,
            	    &RR->bounds,
            	    RR->bounds.MinX,
            	    RR->bounds.MinY);
    RR = RR->Next;
  }
}
