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

#define SCROLLSIGN +

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

/*
 * Free a layer and all its associated structures
 */
void _FreeLayer(struct Layer * l)
{
  struct ClipRect * cr = l->ClipRect, * _cr;
  
  while (cr)
  {
    if (cr->BitMap)
      FreeBitMap(cr->BitMap);
    _cr = cr->Next;
    FreeMem(cr, sizeof(struct ClipRect));
    cr = _cr;
  }

  /*
   * also free all backed up cliprects.
   */
  cr = l->SuperSaveClipRects;

  while (cr)
  {
    _cr = cr->Next;
    FreeMem(cr, sizeof(struct ClipRect));
    cr = _cr;
  }

  DisposeRegion(l->DamageList);
  DisposeRegion(l->VisibleRegion);
  DisposeRegion(l->shape);
  
  FreeMem(l, sizeof(struct Layer));
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
//    FreeLayerResources(li, TRUE);

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


#define MAX(a,b)    ((a) > (b) ? (a) : (b))
#define MIN(a,b)    ((a) < (b) ? (a) : (b))

void _TranslateRect(struct Rectangle *rect, WORD dx, WORD dy)
{
    rect->MinX += dx;
    rect->MinY += dy;
    rect->MaxX += dx;
    rect->MaxY += dy;
}


/***************************************************************************/
/*                            RESOURCE HANDLING                            */
/***************************************************************************/

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


/***************************************************************************/
/*                              MISCELLANEOUS                              */
/***************************************************************************/

struct ClipRect * _CreateClipRectsFromRegion(struct Region *r,
                                             struct Layer * l,
                                             int invisible,
                                             struct Region * inverter)
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
      cr = _AllocClipRect(l);
      cr->bounds.MinX = rr->bounds.MinX + r->bounds.MinX;
      cr->bounds.MinY = rr->bounds.MinY + r->bounds.MinY;
      cr->bounds.MaxX = rr->bounds.MaxX + r->bounds.MinX;
      cr->bounds.MaxY = rr->bounds.MaxY + r->bounds.MinY;
      cr->lobs  = invisible;
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

      AndRegionRegion(l->shape,r);
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
                              int dx,
                              int backupmode,
                              int freelist,
                              int addtodamagelist)
{
  struct BitMap * display_bm = l->rp->BitMap;

  while (NULL != oldcr)
  {
    struct ClipRect * _cr = newcr;
    int area = RECTAREA(&oldcr->bounds);
    while ((NULL != _cr) &&  (0 != area) )
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
              struct Rectangle rect;
              rect.MinX = oldcr->bounds.MinX > _cr->bounds.MinX ?
                          oldcr->bounds.MinX :
                            _cr->bounds.MinX;
              rect.MinY = oldcr->bounds.MinY > _cr->bounds.MinY ?
                          oldcr->bounds.MinY :
                            _cr->bounds.MinY;
              rect.MaxX = oldcr->bounds.MaxX < _cr->bounds.MaxX ?
                          oldcr->bounds.MaxX :
                            _cr->bounds.MaxX;
              rect.MaxY = oldcr->bounds.MaxY < _cr->bounds.MaxY ?
                          oldcr->bounds.MaxY :
                            _cr->bounds.MaxY;
            
              _TranslateRect(&rect, -l->bounds.MinX, -l->bounds.MinY);
            
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
              struct Rectangle rect;
              rect.MinX = oldcr->bounds.MinX > _cr->bounds.MinX ?
                          oldcr->bounds.MinX :
                            _cr->bounds.MinX;
              rect.MinY = oldcr->bounds.MinY > _cr->bounds.MinY ?
                          oldcr->bounds.MinY :
                            _cr->bounds.MinY;
              rect.MaxX = oldcr->bounds.MaxX < _cr->bounds.MaxX ?
                          oldcr->bounds.MaxX :
                            _cr->bounds.MaxX;
              rect.MaxY = oldcr->bounds.MaxY < _cr->bounds.MaxY ?
                          oldcr->bounds.MaxY :
                            _cr->bounds.MaxY;
            
              _TranslateRect(&rect, -l->bounds.MinX, -l->bounds.MinY);
            
              OrRectRegion(l->DamageList, &rect);
#if 0
kprintf("_cr->BitMap: %p ,_cr->lobs: %d\n",_cr->BitMap,_cr->lobs);
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
          else
          {
            LONG xSrc, xDest;
            LONG ySrc, yDest;
            LONG xSize, ySize;
            struct BitMap * destbm;
            
            if (IS_SUPERREFRESH(l)) 
              destbm = l->SuperBitMap;
            else
              destbm = _cr->BitMap;
            
            xSize = oldcr->bounds.MaxX - oldcr->bounds.MinX + 1;
            ySize = oldcr->bounds.MaxY - oldcr->bounds.MinY + 1;

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
                xSize += xSrc;
                xSrc = -xSrc + ALIGN_OFFSET(oldcr->bounds.MinX);
                xDest = ALIGN_OFFSET((_cr->bounds.MinX + dx));
              }
              else
              {
                /*
                 * oldcr is further to the right
                 */
                xDest = xSrc + ALIGN_OFFSET((_cr->bounds.MinX + dx));
                xSrc = ALIGN_OFFSET(oldcr->bounds.MinX);
              }
              
              ySrc = (oldcr->bounds.MinY - _cr->bounds.MinY);
              if (ySrc < 0)
              {
                ySize += ySrc;
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
                  xDest = (oldcr->bounds.MinX - _cr->bounds.MinX) + ALIGN_OFFSET((_cr->bounds.MinX + dx));
              }
              else
              {
                xSrc = _cr->bounds.MinX;
                xSize -= (_cr->bounds.MinX - oldcr->bounds.MinX);
                if (IS_SUPERREFRESH(l))
                  xDest = SCROLLSIGN l->Scroll_X;
                else 
                  xDest = ALIGN_OFFSET((_cr->bounds.MinX + dx));
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
                ySize -= (_cr->bounds.MinY - oldcr->bounds.MinY);
                yDest = 0;
                if (IS_SUPERREFRESH(l))
                  yDest = yDest SCROLLSIGN l->Scroll_Y;
              }
              
              srcbm = l->rp->BitMap;
//kprintf("Using bitmap of screen!\n");
            }

            if (oldcr->bounds.MaxX > _cr->bounds.MaxX)
              xSize -= (oldcr->bounds.MaxX - _cr->bounds.MaxX);

            if (oldcr->bounds.MaxY > _cr->bounds.MaxY)
              ySize -= (oldcr->bounds.MaxY - _cr->bounds.MaxY);

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
              struct Rectangle rect;
              rect.MinX = oldcr->bounds.MinX > _cr->bounds.MinX ?
                          oldcr->bounds.MinX :
                            _cr->bounds.MinX;
              rect.MinY = oldcr->bounds.MinY > _cr->bounds.MinY ?
                          oldcr->bounds.MinY :
                            _cr->bounds.MinY;
              rect.MaxX = oldcr->bounds.MaxX < _cr->bounds.MaxX ?
                          oldcr->bounds.MaxX :
                            _cr->bounds.MaxX;
              rect.MaxY = oldcr->bounds.MaxY < _cr->bounds.MaxY ?
                          oldcr->bounds.MaxY :
                            _cr->bounds.MaxY;
            
              _CallLayerHook(l->BackFill,
                             l->rp,
                             l,
                             &rect,
                             rect.MinX,
                             rect.MinY);
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
              LONG xSize, ySize;
              struct BitMap * srcbm;
              
              if (IS_SUPERREFRESH(l))
                srcbm = l->SuperBitMap;
              else
                srcbm = oldcr->BitMap;
           
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
                
                if (_cr->bounds.MinX > oldcr->bounds.MinX)
                  xSize += (_cr->bounds.MinX - oldcr->bounds.MinX);

                if (_cr->bounds.MinY > oldcr->bounds.MinY)
                  ySize += (_cr->bounds.MinY - oldcr->bounds.MinY);
                  
              }
              
              if (oldcr->bounds.MaxX > _cr->bounds.MaxX)
                xSize -= (oldcr->bounds.MaxX - _cr->bounds.MaxX);

              if (oldcr->bounds.MaxY > _cr->bounds.MaxY)
                ySize -= (oldcr->bounds.MaxY - _cr->bounds.MaxY);
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
      _FreeClipRect(oldcr, l);
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
    struct RegionRectangle * rr = dr->RegionRectangle;
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
                     rr->bounds.MinY);
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
  struct Region r, * clipregion;
  r.RegionRectangle = NULL;  // min. initialization!

  /*
   * Uninstall clipping region. This causes all pixels to
   * be copied into the cliprects that cover the complete
   * area of the layer.
   */

  clipregion = InstallClipRegion(l, NULL);  

  ClearRegionRegion(hide_region,l->VisibleRegion);
  _SetRegion(l->VisibleRegion, &r);
  AndRegionRegion(l->shape,&r);
  AndRegionRegion(l->parent->shape,&r);

  newcr = _CreateClipRectsFromRegion(&r,l,FALSE,NULL);

  _CopyClipRectsToClipRects(l,
                            l->ClipRect /* source */,
                            newcr  /* destination */,
                            dx,
                            backupsimplerefresh,
                            TRUE,
                            TRUE);

  l->ClipRect = newcr;

  /*
   * Reinstall the clipping region. This causes the
   * whole visible area of the layer to be copied
   * into the clipping regions cliprects. The
   * regular list of cliprects is still maintained.
   */
  if (clipregion)
    InstallClipRegion(l, clipregion);

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
  struct Region r;
  struct Region * clipregion;
  
  r.RegionRectangle = NULL;  // min. initialization
//kprintf("%s called for %p\n",__FUNCTION__,l);

  /*
   * If there is a clipping region then the whole
   * window is currently backed up in l->ClipRect
   * That covers the complete area. I must first
   * make these visible, move them back to 
   * l->_cliprects and recreate the clipping cliprects
   * according to the clipregion
   */ 

  clipregion = InstallClipRegion(l, NULL);

  OrRegionRegion(show_region,l->VisibleRegion);
  _SetRegion(l->VisibleRegion,&r);
  AndRegionRegion(l->shape,&r);
  AndRegionRegion(l->parent->shape,&r);
  
  newcr = _CreateClipRectsFromRegion(&r,l,FALSE,NULL);

  _CopyClipRectsToClipRects(l,
                            l->ClipRect /* source */,
                            newcr /* destination */,
                            0,
                            FALSE,
                            TRUE,
                            FALSE);


  l->ClipRect = newcr;

  if (clipregion)
    InstallClipRegion(l, clipregion);

  return TRUE;
}

int _ShowLayer(struct Layer * l)
{
  struct Region r;
  struct RegionRectangle * rr;
  struct ClipRect * prevcr = NULL;
  struct BitMap * bm = l->rp->BitMap;
  int invisible = FALSE;
  
  r.RegionRectangle = NULL;
  _SetRegion(l->VisibleRegion, &r);
  AndRegionRegion(l->shape, &r);

  while (1)
  {
    rr = r.RegionRectangle;

    while (NULL != rr)
    {
      struct ClipRect * cr = AllocMem(sizeof(struct ClipRect), MEMF_CLEAR);

//kprintf("\t\tinvisible: %d !!!!!!!!!!!!\n",invisible);

      cr->bounds.MinX = rr->bounds.MinX + r.bounds.MinX;
      cr->bounds.MinY = rr->bounds.MinY + r.bounds.MinY;
      cr->bounds.MaxX = rr->bounds.MaxX + r.bounds.MinX;
      cr->bounds.MaxY = rr->bounds.MaxY + r.bounds.MinY;
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
                         cr->bounds.MinY);
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
        }
      }
      
      rr=rr->Next;
    }
    
    if (FALSE == invisible)
    {
      XorRectRegion(&r, &l->bounds);
      AndRegionRegion(l->shape, &r);
      invisible = TRUE;
    }
    else
      break;
  }
  
  return TRUE;
}

/*
 * It is assumed that the region r is not needed anymore.
 */
void _BackFillRegion(struct Layer * l, 
                     struct Region * r,
                     int addtodamagelist)
{
  struct RegionRectangle * RR;

  if (TRUE == addtodamagelist)
  {
    RR  = r->RegionRectangle;
    /* check if a region is empty */

    if (NULL != RR)
    {
      l->Flags |= LAYERREFRESH;
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
      }
    }
  }

  AndRegionRegion(l->VisibleRegion, r);

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
                    RR->bounds.MinY);
    RR = RR->Next;
  }

}

int _SetRegion(struct Region * src, struct Region * dest)
{
  struct RegionRectangle * rrs =  src->RegionRectangle;
  struct RegionRectangle * rrd = dest->RegionRectangle;
  struct RegionRectangle * rrd_prev = NULL;
  
  dest->bounds = src->bounds;

  while (NULL != rrs)
  {
    /*
     * Is there a destination region rectangle available?
     */
    if (NULL == rrd)
    {
      rrd = NewRegionRectangle();
    
      if (NULL == rrd)
        return FALSE;
        
      if (NULL == rrd_prev)
        dest->RegionRectangle = rrd;
      else
        rrd_prev->Next = rrd;
      
      rrd->Next = NULL;
    }
    
    /*
     * Copy the bounds.
     */
    rrd->bounds = rrs->bounds;
    rrd->Prev   = rrd_prev;

    /*
     * On to the next one in both lists.
     */
    rrs = rrs->Next;
    rrd_prev = rrd;
    rrd = rrd->Next;
  }
  
  /*
   * Deallocate any excessive RegionRectangles that might be in
   * the destination Region.
   */
  if (NULL == rrd_prev)
  {
    /*
     * Did never enter above loop...
     */
    rrd = dest->RegionRectangle;
    dest->RegionRectangle = NULL;
  }
  else
  {
    /*
     * Was in the loop.
     */
    rrd_prev->Next = NULL;
  }
  
  while (NULL != rrd)
  {
    struct Region * _rr = rrd->Next;
    DisposeRegionRectangle(rrd);
    rrd = _rr;
  }
  
  return TRUE;
}
