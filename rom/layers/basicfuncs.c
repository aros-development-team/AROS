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
    BltBitMap(rp->BitMap, cr->bounds.MinX, cr->bounds.MinY,
	      cr->BitMap, cr->bounds.MinX & 0xf, 0,
	      cr->bounds.MaxX - cr->bounds.MinX + 1,
	      cr->bounds.MaxY - cr->bounds.MinY + 1,
	      Mode, ~0, NULL);
}

void BltCRtoRP(struct RastPort *   rp,
               struct ClipRect *   cr,
               ULONG               Mode)
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

/* Set the priorities of the layer. The farther in the front it is the
   higher its priority will be.
 */
 
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
  /* Add the ClipRect to the front of the list */
  CR -> Next = L -> SuperSaveClipRects;
  L -> SuperSaveClipRects = CR; 
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
    
  while (TRUE)
  {
    if (NULL != _CR->BitMap && TRUE == isSmart)
    {
      FreeBitMap(_CR->BitMap);
      _CR->BitMap = NULL;
    }
    if (NULL != _CR->Next)
      _CR = _CR->Next;
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

void CleanTopLayer(struct Layer_Info * LI)
{
  struct Layer * L_top = LI->top_layer;
  struct ClipRect * CR;
  if (NULL == L_top)
    return;
  
  L_top->Flags &= ~LAYERUPDATING;
  
  CR = L_top->ClipRect;
  if (NULL != CR->Next)
  {
    struct ClipRect * _CR = CR->Next;
    /* the topmost layer should only have one ClipRect */
    CR->bounds = L_top->bounds;
    CR->Next = NULL;
    
    CR = _CR;
    while (CR != NULL)
    {
      _CR = CR->Next;
      _FreeClipRect(CR, L_top);
      
      CR = _CR;
    }
  }
}

/*

  CleanupLayers: All layers that are totally visible but might have more
                 than one cliprect are recombined by this function.
*/

void CleanupLayers(struct Layer_Info * LI)
{
  struct Layer * L;
  if (NULL == LI->top_layer)
    return;
    
  CleanTopLayer(LI);
  
  L = LI->top_layer->back;
  
  while (L != NULL)
  {
    /* Was this layer affected at all? */
    if (0 != (L->Flags & LAYERUPDATING))
    {    
      BOOL is_hidden = FALSE;
      struct ClipRect * CR = L->ClipRect;
 
      /* Clear the updating flag */
      L->Flags &= ~LAYERUPDATING;
    
      while (NULL != CR)
      {
        if (NULL != CR->lobs)
        {
          is_hidden = TRUE;
          break;
        }
        CR = CR->Next;
      }
    
      if (FALSE == is_hidden)
      {
        struct ClipRect * _CR;
        CR = L->ClipRect;
      
        _CR = CR->Next;

        CR->bounds = L->bounds;
        CR->Next   = NULL;
      
        CR = _CR;
        while (NULL != CR)
        {
          _CR = CR->Next;
        
          _FreeClipRect(CR, L);
         
          CR = _CR;
        }
      
      }
    }
    L = L->back;
  }
}



/*
  Unsplit all layers: Some layers' cliprects have fallen into too many
  tiny cliprects. But when a layer is deleted those cliprects could 
  actually be recombined. This is the function to do that.
*/

void UnsplitLayers(struct Layer_Info * LI, struct Rectangle * rect)
{

  struct Layer * L = LI->top_layer;
  if (NULL == L)
    return;
  /* The following function takes care of the top layer. */
  CleanTopLayer(LI); 

  L = L->back;
  while (NULL != L)
  {
    /* just in case this flag is set */
    L->Flags &= ~LAYERUPDATING;
    if (!(L->bounds.MinX > rect->MaxX ||
          L->bounds.MaxX < rect->MinX ||
          L->bounds.MinY > rect->MaxY ||
          L->bounds.MaxY < rect->MinY   ))
    {
      struct ClipRect * CR, * _CR;    
//kprintf("Found layer to clean!\n");
      
      if (LAYERSMART == (L->Flags & (LAYERSMART|LAYERSUPER)))
      {
        /* In case of a smart layer the cliprect list must be preserved 
           as the content of the cliprects' bitmaps is valuable */
        L->cr = L->ClipRect;
      }
      else
      {
        /* Otherwise those cliprects are not very valueable as simplelayers
           don't have anything stored in the cliprects and superbitlayers
           only contain pointers to the superbitmap */
        _CR = L->SuperSaveClipRects;
        if (NULL == _CR)
        {
          /* make the used cliprects head of the list */
          L->SuperSaveClipRects = L->ClipRect;
        }
        else
        {
          /* search for the end of the save cliprects list */
          while (NULL != _CR->Next)
            _CR = _CR->Next;
          
          /* connect the current list to the end. */
          _CR->Next = L->ClipRect;
        }
      }
      
      /* get one new cliprect */
      L->ClipRect = _AllocClipRect(L);
      L->ClipRect->bounds = L->bounds;
      /* 
         Create the cliprects of this layer by letting all the other
         layers split it. This will automatically create the minimum 
         list of rectangles. 
      */
      CreateClipRectsSelf(L, TRUE);
      /* 
         Now I can copy all contents from the L->cr list to the 
         L->ClipRect list.
      */

      if (LAYERSMART == (L->Flags & (LAYERSMART|LAYERSUPER)))
      {
        /* in case of a smart layer the backedup bitmaps' contents
           must be blitted into the newly created cliprects and
           the old list of cliprects can be freed.
           The newly created cliprectlist will have less cliprects
           than the old on and besides that any cliprect of the old
           list will fit into a cliprect of the new list which means
           that contents from old cliprects can be blitted to
           the new cliprects in one piece.
        */
        CR = L->cr;
        
        while (NULL != CR)
        {
          int area = (CR->bounds.MaxX - CR->bounds.MinX + 1) *
                     (CR->bounds.MaxY - CR->bounds.MinY + 1);
          if (NULL != CR->BitMap)
          {
            _CR = L->ClipRect;
            /* search for the cliprect where this bitmap info will
               go into
            */
            while (NULL != _CR)
            {
              if (!(CR->bounds.MinX > _CR->bounds.MaxX ||
                    CR->bounds.MaxX < _CR->bounds.MinX ||
                    CR->bounds.MinY > _CR->bounds.MaxY ||
                    CR->bounds.MaxY < _CR->bounds.MinY    ))
              {
                ULONG srcX, srcY;
                ULONG destX, destY;
                ULONG width, height;
                
                width  = CR->bounds.MaxX - CR->bounds.MinX + 1;
                height = CR->bounds.MaxY - CR->bounds.MinY + 1; 
                
                if (CR->bounds.MinX > _CR->bounds.MinX)
                {
                  srcX  = CR->bounds.MinX & 0x0f;
                  destX = CR->bounds.MinX - _CR->bounds.MinX + (_CR->bounds.MinX & 0x0f);
                }
                else
                {
                  srcX   = _CR->bounds.MinX - CR->bounds.MinX + (CR->bounds.MinX & 0x0f);
                  destX  = _CR->bounds.MinX & 0x0f;
                  width -= (_CR->bounds.MinX - CR->bounds.MinX);
                }
                  
                if (CR->bounds.MinY > _CR->bounds.MinY)
                {
                  srcY  = 0;
                  destY = CR->bounds.MinY - _CR->bounds.MinY;                  
                }
                else
                {
                  srcY    = _CR->bounds.MinY - CR->bounds.MinY;
                  destY   = 0; 
                  height -= srcY;
                }
                  
                if (CR->bounds.MaxX > _CR->bounds.MaxX)
                  width  -= (CR->bounds.MaxX - _CR->bounds.MaxX);
                
                if (CR->bounds.MaxY > _CR->bounds.MaxY)
                  height -= (CR->bounds.MaxY - _CR->bounds.MaxY);


                BltBitMap(CR->BitMap,
                          srcX,
                          srcY,
                          _CR->BitMap,
                          destX,
                          destY,
                          width,
                          height,
                          0x0c0,/*copy  */
                          0xff, 
                          NULL); 
                area -= width * height;
                if (0 == area)
                  break;
                   
              }
              _CR = _CR->Next;
            }
            
            /* free the bitmap */
            FreeBitMap(CR->BitMap);
          }
          
          /* 
            Free the cliprect and go to the next cliprect of the old cliprects 
          */
          _CR = CR->Next;
          _FreeClipRect(CR, L);
          CR = _CR;
        } /* while (NULL != CR) */
        L->cr = NULL;
      }  
    }
    L = L->back;
  }
}


/* 
 * A function that may only be called for pure SMART layers!
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
               sCR->bounds.MaxY < dCR->bounds.MinY))
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
            srcX  = sCR->bounds.MinX & 0x0f;
            destX = sCR->bounds.MinX - dCR->bounds.MinX + (dCR->bounds.MinX & 0x0f);
          }
          else
          {
            srcX   = dCR->bounds.MinX - sCR->bounds.MinX + (sCR->bounds.MinX & 0x0F);
            destX  = dCR->bounds.MinX & 0x0f;
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
           
          if (a == area)
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

/*-----------------------------------END-----------------------------------*/
