/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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
#include <proto/arossupport.h>

#include "../graphics/intregions.h"
#include "layers_intern.h"
#include "basicfuncs.h"

#define CLIPRECTS_OUTSIDE_OF_SHAPE 1

#define SCROLLSIGN +

#define USE_POOLS 1

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
               ULONG                Mode,
               struct LayersBase *  LayersBase)
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
               ULONG               Mode,
               struct LayersBase  * LayersBase)
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
                    WORD offsetY,
                    struct LayersBase * LayersBase)
{
  struct BitMap * bm = rp->BitMap;

  if (L)
  {
    if (IL(L)->intflags & INTFLAG_AVOID_BACKFILL) return;
  }
  
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

/*
 * Free a layer and all its associated structures
 */
void _FreeLayer(struct Layer * l, struct LayersBase *LayersBase)
{
  struct ClipRect * cr = l->ClipRect, * _cr;
  
  while (cr)
  {
    if (cr->BitMap)
      FreeBitMap(cr->BitMap);
    _cr = cr->Next;
#if USE_POOLS  
    ObtainSemaphore(&LayersBase->lb_MemLock);
    FreePooled(LayersBase->lb_ClipRectPool, cr, sizeof(struct ClipRect));
    ReleaseSemaphore(&LayersBase->lb_MemLock);
#else
    FreeMem(cr, sizeof(struct ClipRect));
#endif
    cr = _cr;
  }

  /*
   * also free all backed up cliprects.
   */
  cr = l->SuperSaveClipRects;

  while (cr)
  {
    _cr = cr->Next;
#if USE_POOLS  
    ObtainSemaphore(&LayersBase->lb_MemLock);
    FreePooled(LayersBase->lb_ClipRectPool, cr, sizeof(struct ClipRect));
    ReleaseSemaphore(&LayersBase->lb_MemLock);
#else
    FreeMem(cr, sizeof(struct ClipRect));
#endif
    cr = _cr;
  }

  FreeRastPort(l->rp);
  DisposeRegion(l->DamageList);
  DisposeRegion(l->VisibleRegion);
  DisposeRegion(l->shape);
  DisposeRegion(l->visibleshape);
  
  FreeMem(l, sizeof(struct IntLayer));
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
BOOL _AllocExtLayerInfo(struct Layer_Info * li, struct LayersBase *LayersBase)
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
void _FreeExtLayerInfo(struct Layer_Info * li, struct LayersBase *LayersBase)
{
    if(--li->fatten_count >= 0)
	return;

    /* Kill Root Layer */
    
    if (li->check_lp)
        DeleteLayer(0UL, li->check_lp);
    li->check_lp = NULL;

    if(li->LayerInfo_extra == NULL)
	return;
    
    FreeMem(li->LayerInfo_extra, sizeof(struct LayerInfo_extra));

    li->LayerInfo_extra = NULL;
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

    if(_AllocExtLayerInfo(li, LayersBase))
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
	_FreeExtLayerInfo(li, LayersBase);

    UnlockLayerInfo(li);
}

/***************************************************************************/
/*                                RECTANGLE                                */
/***************************************************************************/


#define MAX(a,b)    ((a) > (b) ? (a) : (b))
#define MIN(a,b)    ((a) < (b) ? (a) : (b))
/*
void _TranslateRect(struct Rectangle *rect, WORD dx, WORD dy)
{
    rect->MinX += dx;
    rect->MinY += dy;
    rect->MaxX += dx;
    rect->MaxY += dy;
}*/


/***************************************************************************/
/*                            RESOURCE HANDLING                            */
/***************************************************************************/

/*
 * Allocate memory for a ClipRect.
 */

struct ClipRect * _AllocClipRect(struct Layer * L, struct LayersBase *LayersBase)
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

#if USE_POOLS  
  ObtainSemaphore(&LayersBase->lb_MemLock);
  CR = (struct ClipRect *)AllocPooled(LayersBase->lb_ClipRectPool, sizeof(struct ClipRect));
  ReleaseSemaphore(&LayersBase->lb_MemLock);
#else
  CR = (struct ClipRect *) AllocMem(sizeof(struct ClipRect), MEMF_PUBLIC|MEMF_CLEAR);
#endif
  return CR;
}

/*
 * Return memory of a ClipRect for later use.
 */

void _FreeClipRect(struct ClipRect   * CR,
                   struct Layer      * L,
		   struct LayersBase * LayersBase)
{
  if (L->SuperSaveClipRectCounter < MAXSUPERSAVECLIPRECTS)
  {
    /* Add the ClipRect to the front of the list */
    CR -> Next = L -> SuperSaveClipRects;
    L -> SuperSaveClipRects = CR;
    L -> SuperSaveClipRectCounter++;
  }
  else
  {
#if USE_POOLS  
    ObtainSemaphore(&LayersBase->lb_MemLock);
    FreePooled(LayersBase->lb_ClipRectPool, CR, sizeof(struct ClipRect));
    ReleaseSemaphore(&LayersBase->lb_MemLock);
#else
    FreeMem(CR, sizeof(struct ClipRect));
#endif
  }
}

/*
 * Free a whole list of cliprects including the allocated bitmaps (if any)
 */

void _FreeClipRectListBM(struct Layer * L,
                         struct ClipRect * CR,
			 struct LayersBase *LayersBase)
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


/***************************************************************************/
/*                              MISCELLANEOUS                              */
/***************************************************************************/

struct ClipRect * _CreateClipRectsFromRegion(struct Region *r,
                                             struct Layer * l,
                                             int invisible,
                                             struct Region * inverter,
					     struct LayersBase *LayersBase)
{
  int looped = FALSE;
  struct ClipRect * firstcr = NULL, * cr;
  struct BitMap * display_bm = l->rp->BitMap;

  /*
   * From region r create cliprects
   */
  while (1)
  {
    struct RegionRectangle * rr = r->RegionRectangle;
    while (rr)
    {
      cr = _AllocClipRect(l, LayersBase);
      cr->bounds.MinX = rr->bounds.MinX + r->bounds.MinX;
      cr->bounds.MinY = rr->bounds.MinY + r->bounds.MinY;
      cr->bounds.MaxX = rr->bounds.MaxX + r->bounds.MinX;
      cr->bounds.MaxY = rr->bounds.MaxY + r->bounds.MinY;
      cr->lobs  = (struct Layer *)invisible;
      cr->Next  = firstcr;

      if (TRUE == invisible && IS_SMARTREFRESH(l))
      {
        cr->BitMap = AllocBitMap(
                   cr->bounds.MaxX - cr->bounds.MinX + 1 + 16,
                   cr->bounds.MaxY - cr->bounds.MinY + 1,
                   display_bm->Depth,
                   BMF_CLEAR,
                   display_bm);
      }

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

    if (FALSE == looped)
    {
      /*
       * Flip the shape to the opposite part and
       * limit it to its own shape.
       */
      if (inverter)
        XorRegionRegion(inverter, r);
      else
        XorRectRegion(r,&l->bounds);

#if !CLIPRECTS_OUTSIDE_OF_SHAPE
      AndRegionRegion(l->shape,r);
#else
      AndRectRegion(r,&l->bounds);
#endif
      if (TRUE == invisible)
        invisible = FALSE;
      else
        invisible = TRUE;
      looped = TRUE;
    }
    else
      break;
  } /* while (1) */

  return firstcr;
}


int _CopyClipRectsToClipRects(struct Layer * l,
                              struct ClipRect * oldcr,
                              struct ClipRect * newcr,
                              int srcdx,
			      int destdx,
                              int backupmode,
                              int freelist,
                              int addtodamagelist,
			      struct LayersBase *LayersBase)
{
  struct BitMap * display_bm = l->rp->BitMap;
  
  while (NULL != oldcr)
  {
    struct ClipRect * _cr = newcr;
    int area = RECTAREA(&oldcr->bounds);
    while ((NULL != _cr) &&  (0 != area) )
    {
      struct Rectangle intersect;
      
      /*
       * Do the two rectangles overlap?
       */
      if (_AndRectRect(&_cr->bounds, &oldcr->bounds, &intersect))
      {
        LONG xSize = intersect.MaxX - intersect.MinX + 1;
	LONG ySize = intersect.MaxY - intersect.MinY + 1;
	
        /*
         * Is this new one supposed to be invisible?
         */
        if (NULL != _cr->lobs)
        {
          struct BitMap * srcbm;
          /*
           * The new one is supposed to be invisible.
           * So for SIMPLEREFRESH layers I don't have to
           * do anything if 
           * a) not in backupmode or
           * b) old cr was invisible
           */
          if (IS_SIMPLEREFRESH(l) && FALSE == backupmode && NULL == _cr->BitMap)
          {
            if (FALSE == addtodamagelist)
            {
              struct Rectangle rect = intersect;
            
              _TranslateRect(&rect, -l->bounds.MinX, -l->bounds.MinY);

#warning: stegerg: Not sure if this is a good idea. What for example if updating is done in several passes? And CopyClipRectsToClipRects is used by all kinds of functions including BeginUpdate/EndUpdate/InstallClipRegion/etc.
              ClearRectRegion(l->DamageList, &rect);
#if 0
kprintf("");
kprintf("%s: Removing %d/%d-%d/%d from damagelist!\t",
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
kprintf("%s: oldcr: %d/%d-%d/%d!\t",
        __FUNCTION__,
        oldcr->bounds.MinX,
        oldcr->bounds.MinY,
        oldcr->bounds.MaxX,
        oldcr->bounds.MaxY
        );
kprintf("%s: _cr: %d/%d-%d/%d!\n\n",
        __FUNCTION__,
        _cr->bounds.MinX,
        _cr->bounds.MinY,
        _cr->bounds.MaxX,
        _cr->bounds.MaxY
        );
#endif
            }
          }
          else if (IS_SIMPLEREFRESH(l) && TRUE == backupmode && NULL  != oldcr->lobs)
          {
            if (TRUE == addtodamagelist)
            {
              struct Rectangle rect = intersect;
              _TranslateRect(&rect, -l->bounds.MinX, -l->bounds.MinY);
            
              // FIXME (if possible)
              // !!! Also areas where a child disappears beyond the
              // boundaries of its parent are added here!
              OrRectRegion(l->DamageList, &rect);
#if 0
kprintf("_cr->BitMap: %p ,_cr->lobs: %d\n",_cr->BitMap,_cr->lobs);
#endif
#if 0
kprintf("%s: Adding %d/%d-%d/%d to damagelist of l=%p!\t",
        __FUNCTION__,
        rect.MinX,
        rect.MinY,
        rect.MaxX,
        rect.MaxY,
        l
        );
#endif
#if 0
kprintf("%s: Layer: %d/%d-%d/%d!\n",
        __FUNCTION__,
        l->bounds.MinX,
        l->bounds.MinY,
        l->bounds.MaxX,
        l->bounds.MaxY
        );
kprintf("%s: oldcr: %d/%d-%d/%d!\t",
        __FUNCTION__,
        oldcr->bounds.MinX,
        oldcr->bounds.MinY,
        oldcr->bounds.MaxX,
        oldcr->bounds.MaxY
        );
kprintf("%s: _cr: %d/%d-%d/%d!\n\n",
        __FUNCTION__,
        _cr->bounds.MinX,
        _cr->bounds.MinY,
        _cr->bounds.MaxX,
        _cr->bounds.MaxY
        );
#endif
            } else {
//kprintf("Not adding to damage list for l=%p!\n",l);
            }
          }
          else
          {
            LONG xSrc, xDest;
            LONG ySrc, yDest;
            struct BitMap * destbm;
            
            if (IS_SUPERREFRESH(l)) 
              destbm = l->SuperBitMap;
            else
              destbm = _cr->BitMap;
            
            /*
             * Does the source rect have a bitmap (off screen)
             * or is it on the screen.
             */            
            if (oldcr->lobs && !IS_SUPERREFRESH(l))
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
                xSrc = -xSrc;
                xDest = 0;
              }
              else
              {
                /*
                 * oldcr is further to the right
                 */
                xDest = xSrc;
                xSrc = 0;
              }
              
	      xSrc  += ALIGN_OFFSET(oldcr->bounds.MinX + srcdx);
	      xDest += ALIGN_OFFSET(_cr->bounds.MinX + destdx);
	      
              ySrc = (oldcr->bounds.MinY - _cr->bounds.MinY);
              if (ySrc < 0)
              {
                ySrc   = -ySrc;
                yDest  = 0;
              }
              else
              {
                yDest = ySrc;
                ySrc  = 0;
              }
//kprintf("Using old cr's BitMap!\n");
              srcbm = oldcr->BitMap;
            }
            else
            {
              /*
               * Copy from screen.
               */
              if (oldcr->bounds.MinX > _cr->bounds.MinX)
              {
                xSrc = oldcr->bounds.MinX;
                if (IS_SUPERREFRESH(l))
                  xDest = (oldcr->bounds.MinX - _cr->bounds.MinX) SCROLLSIGN l->Scroll_X;
                else
                  xDest = (oldcr->bounds.MinX - _cr->bounds.MinX) + ALIGN_OFFSET((_cr->bounds.MinX + destdx));
              }
              else
              {
                xSrc = _cr->bounds.MinX;
                if (IS_SUPERREFRESH(l))
                  xDest = SCROLLSIGN l->Scroll_X;
                else 
                  xDest = ALIGN_OFFSET((_cr->bounds.MinX + destdx));
              }
              
              if (oldcr->bounds.MinY > _cr->bounds.MinY)
              {
                ySrc = oldcr->bounds.MinY;
                yDest = oldcr->bounds.MinY - _cr->bounds.MinY;
                if (IS_SUPERREFRESH(l))
                  yDest = yDest SCROLLSIGN l->Scroll_Y;
              }
              else
              {
                ySrc = _cr->bounds.MinY;
                yDest = 0;
                if (IS_SUPERREFRESH(l))
                  yDest = yDest SCROLLSIGN l->Scroll_Y;
              }
              
              srcbm = l->rp->BitMap;
//kprintf("Using bitmap of screen!\n");
            }

            if (IS_SIMPLEREFRESH(l) &&
                NULL == _cr->BitMap &&
                TRUE == backupmode)
            {
              /*
               * Get a bitmap (if not there) and make a backup
               */
              _cr->BitMap = AllocBitMap(
                 _cr->bounds.MaxX - _cr->bounds.MinX + 1 + 16 ,
                 _cr->bounds.MaxY - _cr->bounds.MinY + 1,
                 display_bm->Depth,
                 BMF_CLEAR,
                 display_bm);
              destbm = _cr->BitMap;
            }

            BltBitMap(srcbm,
                      xSrc,
                      ySrc,
                      destbm,
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

            area -= (xSize * ySize);
            
          }
        }
        else //if (FALSE == backupmode)
        {
          /*
           * The new one is visible. if the old one was not visible
           * then I have to show it.
           * If it is a simple refresh layer and it has it
           * backed up (only when moving!) then I must show
           * this. If it has nothing backed up then I must
           * add a part to the damage list.
           */
          if (IS_SIMPLEREFRESH(l) && 
              (NULL != oldcr->lobs) && 
              (NULL == oldcr->BitMap))
          {
            if (NULL != oldcr->lobs && NULL == oldcr->BitMap)
            {
              struct Rectangle rect = intersect;
           
              _CallLayerHook(l->BackFill,
                             l->rp,
                             l,
                             &rect,
                             rect.MinX,
                             rect.MinY,
                             LayersBase);
              _TranslateRect(&rect, -l->bounds.MinX, -l->bounds.MinY);
              OrRectRegion(l->DamageList, &rect);
#if 0
kprintf("Adding: %d\n",addtodamagelist);
kprintf("%s: Adding %d/%d-%d/%d to damagelist!\t",
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
kprintf("%s: oldcr: %d/%d-%d/%d!\t",
        __FUNCTION__,
        oldcr->bounds.MinX,
        oldcr->bounds.MinY,
        oldcr->bounds.MaxX,
        oldcr->bounds.MaxY
        );
kprintf("%s: _cr: %d/%d-%d/%d!\n",
        __FUNCTION__,
        _cr->bounds.MinX,
        _cr->bounds.MinY,
        _cr->bounds.MaxX,
        _cr->bounds.MaxY
        );
#endif
            }
          }
          else
          {
//kprintf("%s: Showing a part of a backed up bitmap!\n",__FUNCTION__);
            if (NULL != oldcr->lobs)
            {
              /*
               * Copy out of hidden bitmap
               */
              LONG xSrc, xDest;
              LONG ySrc, yDest;
              struct BitMap * srcbm;
              
              if (IS_SUPERREFRESH(l))
                srcbm = l->SuperBitMap;
              else
                srcbm = oldcr->BitMap;
           
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
                  xSrc   = -xSrc + ALIGN_OFFSET(oldcr->bounds.MinX + srcdx);
                  xDest  = _cr->bounds.MinX;
                }
                else
                {
                  /*
                   * oldcr is further to the right
                   */
                  xDest = oldcr->bounds.MinX;
                  xSrc  = ALIGN_OFFSET(oldcr->bounds.MinX + srcdx);
                }
                
                ySrc = (oldcr->bounds.MinY - _cr->bounds.MinY);
                if (ySrc < 0)
                {
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
                /*
                 * superbitmap layer
                 */
                xSrc = (oldcr->bounds.MinX > _cr->bounds.MinX) ?
                        oldcr->bounds.MinX - _cr->bounds.MinX SCROLLSIGN l->Scroll_X :
                        _cr->bounds.MinX - oldcr->bounds.MinX SCROLLSIGN l->Scroll_X;
                xDest = _cr->bounds.MinX;
                
                ySrc = (oldcr->bounds.MinY > _cr->bounds.MinY) ?
                        oldcr->bounds.MinY - _cr->bounds.MinY SCROLLSIGN l->Scroll_Y :
                        _cr->bounds.MinY - oldcr->bounds.MinY SCROLLSIGN l->Scroll_Y;
                yDest = _cr->bounds.MinY;
                
              }
              
#if 0
kprintf("\t\t%s: Show cliprect: %d/%d-%d/%d; blitting to %d/%d _cr->lobs: %d\n",
        __FUNCTION__,
        oldcr->bounds.MinX,
        oldcr->bounds.MinY,
        oldcr->bounds.MaxX,
        oldcr->bounds.MaxY,
        xDest,
        yDest,
        _cr->lobs);
#endif

#warning Must have oldcr->BitMap also for SuperBitMap layers!

              BltBitMap(srcbm,
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
      else
      {

        if (IS_SMARTREFRESH(l) && TRUE == backupmode && NULL == _cr->BitMap && NULL != _cr->lobs)
        {
           _cr->BitMap = AllocBitMap(_cr->bounds.MaxX - _cr->bounds.MinX + 1 + 16 ,
                                     _cr->bounds.MaxY - _cr->bounds.MinY + 1,
                                     display_bm->Depth,
                                     BMF_CLEAR,
                                     display_bm);
        }
      }
      _cr = _cr->Next;
    } /* all new cliprects */
    
    if (TRUE == freelist)
    {
      _cr = oldcr->Next;
      if (oldcr->BitMap)
        FreeBitMap(oldcr->BitMap);
      _FreeClipRect(oldcr, l, LayersBase);
      oldcr = _cr;
    }
    else
      oldcr = oldcr->Next;
  } /* for all old cliprects */

  CHECKDAMAGELIST(l);

  /*
   * If this is a simple refresh layer and I am not in
   * backup mode and I am not adding to the damagelist
   * the I must call the backfillhook for the
   * area of the damage list of a simple refresh layer
   */

  if (IS_SIMPLEREFRESH(l) && 
      (l->Flags & LAYERREFRESH) &&
      FALSE == backupmode &&
      FALSE == addtodamagelist)
  {
    struct Region * dr = l->DamageList;
    struct RegionRectangle * rr;

    _TranslateRect(&dr->bounds, l->bounds.MinX, l->bounds.MinY);
    AndRectRegion(dr, &l->bounds);
    AndRegionRegion(l->VisibleRegion, dr);
    AndRegionRegion(l->visibleshape, dr);
    
    _TranslateRect(&dr->bounds, -l->bounds.MinX, -l->bounds.MinY);
    
    rr = dr->RegionRectangle;
    while (rr)
    {
      _TranslateRect(&rr->bounds, 
                     dr->bounds.MinX + l->bounds.MinX,
                     dr->bounds.MinY + l->bounds.MinY);

      _CallLayerHook(l->BackFill,
                     l->rp,
                     l,
                     &rr->bounds,
                     rr->bounds.MinX,
                     rr->bounds.MinY,
                     LayersBase);

      _TranslateRect(&rr->bounds, 
                     -dr->bounds.MinX-l->bounds.MinX,
                     -dr->bounds.MinY-l->bounds.MinY);
      rr = rr->Next;
 
    }    
  }

  return TRUE;
}

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
                        int backupsimplerefresh,
                        struct LayersBase * LayersBase)
{
  struct ClipRect * newcr;
  struct Region *r, * clipregion;

  /*
   * Uninstall clipping region. This causes all pixels to
   * be copied into the cliprects that cover the complete
   * area of the layer.
   */

  clipregion = _InternalInstallClipRegion(l, NULL, 0, 0, LayersBase);

  ClearRegionRegion(hide_region,l->VisibleRegion);
  r = AndRegionRegionND(l->visibleshape, l->VisibleRegion);
  
  if (r)
  {
     newcr = _CreateClipRectsFromRegion(r,l,FALSE,NULL,LayersBase);
     DisposeRegion(r);

     if (newcr)
     {
         _CopyClipRectsToClipRects(l,
                                   l->ClipRect /* source */,
                                   newcr  /* destination */,
		                   0,
                                   dx,
                                   backupsimplerefresh,
                                   TRUE,
                                   TRUE,
			           LayersBase);

          l->ClipRect = newcr;
     }
  }
  
  /*
   * Reinstall the clipping region. This causes the
   * whole visible area of the layer to be copied
   * into the clipping regions cliprects. The
   * regular list of cliprects is still maintained.
   */
  if (clipregion)
    _InternalInstallClipRegion(l, clipregion, dx, dx, LayersBase);

  return TRUE;
}

/*
 * Show any parts of the layer that overlap with the backup_region
 * and that are not already show.
 *
 * This function MUST not manipulate show_region!!!!
 */

int _ShowPartsOfLayer(struct Layer * l,
                      struct Region * show_region,
                      struct LayersBase * LayersBase)
{
  struct ClipRect * newcr;
  struct Region *r;
  struct Region * clipregion;

//kprintf("%s called for %p\n",__FUNCTION__,l);

  /*
   * If there is a clipping region then the whole
   * window is currently backed up in l->ClipRect
   * That covers the complete area. I must first
   * make these visible, move them back to
   * l->_cliprects and recreate the clipping cliprects
   * according to the clipregion
   */

if (show_region == l->VisibleRegion)
  kprintf("ERROR - same regions!! %s\n",__FUNCTION__);

  clipregion = InstallClipRegion(l, NULL);

  OrRegionRegion(show_region,l->VisibleRegion);
  r = AndRegionRegionND(l->visibleshape, l->VisibleRegion);
  if (r != NULL)
  {
      newcr = _CreateClipRectsFromRegion(r,l,FALSE,NULL,LayersBase);
      DisposeRegion(r);

      _CopyClipRectsToClipRects(l,
				l->ClipRect /* source */,
				newcr /* destination */,
				0,
				0,
				FALSE,
				TRUE,
				FALSE,
				LayersBase);


      l->ClipRect = newcr;
  }
  if (clipregion)
    InstallClipRegion(l, clipregion);

  return TRUE;
}

int _ShowLayer(struct Layer * l, struct LayersBase *LayersBase)
{
  struct Region *r;
  struct RegionRectangle * rr;
  struct ClipRect * prevcr = NULL;
  struct BitMap * bm = l->rp->BitMap;
  int invisible = FALSE;

  r = AndRegionRegionND(l->shape, l->VisibleRegion);
  AndRegionRegion(l->parent->shape, r);

  while (1)
  {
    rr = r->RegionRectangle;

    while (NULL != rr)
    {
      struct ClipRect * cr;

#if USE_POOLS
      ObtainSemaphore(&LayersBase->lb_MemLock);
      cr = (struct ClipRect *)AllocPooled(LayersBase->lb_ClipRectPool, sizeof(struct ClipRect));
      ReleaseSemaphore(&LayersBase->lb_MemLock);
#else
      cr = AllocMem(sizeof(struct ClipRect), MEMF_CLEAR);
#endif

//kprintf("\t\tinvisible: %d !!!!!!!!!!!!\n",invisible);

      MinX(cr) = MinX(rr) + MinX(r);
      MinY(cr) = MinY(rr) + MinY(r);
      MaxX(cr) = MaxX(rr) + MinX(r);
      MaxY(cr) = MaxY(rr) + MinY(r);
      cr->lobs = (struct Layer *)invisible;
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
        if (IS_SUPERREFRESH(l))
        {
          BltBitMap(l->SuperBitMap,
                    cr->bounds.MinX - l->bounds.MinX,
                    cr->bounds.MinY - l->bounds.MinY,
                    l->rp->BitMap,
                    cr->bounds.MinX,
                    cr->bounds.MinY,
                    cr->bounds.MaxX - cr->bounds.MinX + 1,
                    cr->bounds.MaxY - cr->bounds.MinY + 1,
                    0x0c0,
                    0xff,
                    NULL);
        }
        else
        {
          _CallLayerHook(l->BackFill,
                         l->rp,
                         l,
                         &cr->bounds,
                         cr->bounds.MinX,
                         cr->bounds.MinY,
                         LayersBase);
        }
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

#warning stegerg: the backfill hook should be called for this bitmap! But _CallLayerHook always uses rp->BitMap
        }
      }

      rr=rr->Next;
    }

    if (FALSE == invisible)
    {
      XorRectRegion(r, &l->bounds);
#if !CLIPRECTS_OUTSIDE_OF_SHAPE
      AndRegionRegion(l->shape, r);
#endif
      invisible = TRUE;
    }
    else
      break;
  }

  DisposeRegion(r);

  return TRUE;
}

/*
 * It is assumed that the region r is not needed anymore.
 */
void _BackFillRegion(struct Layer * l,
                     struct Region * r,
                     int addtodamagelist,
                     struct LayersBase * LayersBase)
{
  struct RegionRectangle * RR;

  RR = r->RegionRectangle;
  if (NULL == RR) return;
  
  if (IS_SIMPLEREFRESH(l))
  {
    /* Only for simple refresh layers, becuase smart refresh layers
       may have damage outside of visibleshape, like when being dragged
       off screen */
       
    AndRegionRegion(l->visibleshape, r);
  }
  else
  {
    /* Maybe not needed, but to be sure ... */
    
    AndRectRegion(r, &l->bounds);
  }
  
  
  if (TRUE == addtodamagelist)
  {
    l->Flags |= LAYERREFRESH;

#if 1
     /* Region coords are screen relative, but damagelist coords are layer relative! */
      
    _TranslateRect(&r->bounds, -l->bounds.MinX, -l->bounds.MinY);
    OrRegionRegion(r, l->DamageList);
    _TranslateRect(&r->bounds, l->bounds.MinX, l->bounds.MinY);
    
#else
    while (NULL != RR)
    {
      struct Rectangle rect = RR->bounds;

//kprintf("%s: adding to damagelist!\n",__FUNCTION__);

      /* Region coords are screen relative, but damagelist coords are layer relative! */

      _TranslateRect(&rect, 
                     r->bounds.MinX - l->bounds.MinX,
                     r->bounds.MinY - l->bounds.MinY);
#if 0
kprintf("%s: Adding %d/%d-%d/%d to damagelist!\n",
      __FUNCTION__,
      rect.MinX,
      rect.MinY,
      rect.MaxX,
      rect.MaxY
      );
#endif
      OrRectRegion(l->DamageList, &rect);

      _TranslateRect(&rect, 
                     -r->bounds.MinX + l->bounds.MinX,
                     -r->bounds.MinY + l->bounds.MinY);

      RR = RR->Next;
      
    } /* while (NULL != RR) */
#endif
    
  } /* if (TRUE == addtodamagelist) */

  AndRegionRegion(l->VisibleRegion, r);
  if (l->shaperegion)
  {
    /* shaperegion is layer relative, while r is screen relative */
    
    _TranslateRect(&r->bounds, -l->bounds.MinX, -l->bounds.MinY);
    AndRegionRegion(l->shaperegion, r);
    _TranslateRect(&r->bounds, l->bounds.MinX, l->bounds.MinY);    
  }
  
  RR  = r->RegionRectangle;
  /* check if a region is empty */
  while (NULL != RR)
  {
     _TranslateRect(&RR->bounds, r->bounds.MinX, r->bounds.MinY);

#if 0
kprintf("\t\t: %s Clearing rect : %d/%d-%d/%d  layer: %p, hook: %p, bitmap: %p\n",
        __FUNCTION__,
        RR->bounds.MinX,
        RR->bounds.MinY,
        RR->bounds.MaxX,
        RR->bounds.MaxY,
        l,
        l->BackFill,
        l->rp->BitMap);
#endif
     _CallLayerHook(l->BackFill,
                    l->rp,
                    l,
                    &RR->bounds,
                    RR->bounds.MinX,
                    RR->bounds.MinY,
                    LayersBase);
    RR = RR->Next;
  }

}

struct Region *_InternalInstallClipRegion(struct Layer *l, struct Region *region,
    	    	    	    	    	  WORD srcdx, WORD destdx,
    	    	    	    	    	  struct LayersBase *LayersBase)
{
  struct Region * OldRegion;
  BOOL updating = FALSE;
  OldRegion = l->ClipRegion;

  if ((OldRegion != NULL) || (region != NULL))
  {
    if (l->Flags & LAYERUPDATING)
    {
      /* InstallClipRegion does not work if the layer is in update state (BeginUpdate) */

      updating = TRUE;
      EndUpdate(l, FALSE);
      
      OldRegion = l->ClipRegion;
    }

    /* is there a clipregion currently installed? */
    if (NULL != OldRegion)
    { 
      /*
       *  Copy the contents of the region cliprects to the regular
       *  cliprects if layer is a SMARTLAYER. Also free the list of 
       *  region cliprects.
       */
      if (NULL != l->ClipRect)
      {
	if (IS_SMARTREFRESH(l))
	  _CopyClipRectsToClipRects(l,
	                            l->ClipRect,
	                            l->_cliprects,
	                            srcdx,
				    destdx,
	                            FALSE,
	                            TRUE,
				    FALSE,
				    LayersBase);
	else
          _FreeClipRectListBM(l, l->ClipRect, LayersBase);
      }

      /* restore the regular ClipRects */
      l->ClipRect = l->_cliprects;    

    }

    /* at this point the regular cliprects are in l->ClipRect in any case !*/

    /* if there's no new region to install then there's not much to do */
    l->ClipRegion = region;

    if (NULL == region)
      l->_cliprects = NULL;
    else
    {
      struct Region *r;

      /* convert the region to a list of ClipRects */
      /* backup the old cliprects */
      l->_cliprects = l->ClipRect;

      _TranslateRect(&region->bounds, l->bounds.MinX, l->bounds.MinY);

      r = AndRegionRegionND(l->VisibleRegion, region);
      AndRegionRegion(l->shape, r);
      
      l->ClipRect = _CreateClipRectsFromRegion(r,
                                               l,
                                               FALSE,
                                               region,
					       LayersBase);
      DisposeRegion(r);

      _CopyClipRectsToClipRects(l,
                                l->_cliprects,
                                l->ClipRect,
                                srcdx,
				destdx,
                                FALSE,
                                FALSE,
				TRUE,
				LayersBase); /* stegerg: should be FALSE. but that does not work??? */

      _TranslateRect(&region->bounds, -l->bounds.MinX, -l->bounds.MinY);

      /* right now I am assuming that everything went alright */
    }

    if (updating)
      BeginUpdate(l);

  } /* if ((OldRegion != NULL) || (region != NULL)) */

  return OldRegion;
}
