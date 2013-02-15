/*
    Copyright 2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <graphics/rastport.h>
#include <hidd/graphics.h>
#include <proto/alib.h>
#include <proto/layers.h>
#include <proto/graphics.h>
#include <gallium/pipe/p_state.h>
#include <aros/debug.h>

#include "gallium_intern.h"

/*****************************************************************************

    NAME */

      AROS_LH8(void, BltPipeResourceRastPort,

/*  SYNOPSIS */ 
      AROS_LHA(struct pipe_resource *, srcPipeResource, A0),
      AROS_LHA(LONG                  , xSrc, D0),
      AROS_LHA(LONG                  , ySrc, D1),
      AROS_LHA(struct RastPort *     , destRP, A1),
      AROS_LHA(LONG                  , xDest, D2),
      AROS_LHA(LONG                  , yDest, D3),
      AROS_LHA(LONG                  , xSize, D4),
      AROS_LHA(LONG                  , ySize, D5),

/*  LOCATION */
      struct Library *, GalliumBase, 8, Gallium)

/*  NAME
 
    FUNCTION
    Copies part of pipe resource onto rast port. Clips output by using layers of
    rastport.
 
    INPUTS
	srcPipeResource - Copy from this pipe resource.
	xSrc, ySrc - This is the upper left corner of the area to copy.
	destRP - Destination RastPort.
	xDest, yDest - Upper left corner where to place the copy
	xSize, ySize - The size of the area to copy
 
    RESULT
 
    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Layer *L = destRP->Layer;
    struct ClipRect *CR;
    struct Rectangle renderableLayerRect;
    BOOL copied = FALSE;

    struct HIDDT_WinSys * ws = (struct HIDDT_WinSys *)srcPipeResource->screen->winsys;
    if (!ws)
        return;

    if (!IsLayerVisible(L))
        return;

    LockLayerRom(L);
    
    renderableLayerRect.MinX = L->bounds.MinX + xDest;
    renderableLayerRect.MaxX = L->bounds.MinX + xDest + xSize - 1;
    renderableLayerRect.MinY = L->bounds.MinY + yDest;
    renderableLayerRect.MaxY = L->bounds.MinY + yDest + ySize - 1;
    if (renderableLayerRect.MinX < L->bounds.MinX)
        renderableLayerRect.MinX = L->bounds.MinX;
    if (renderableLayerRect.MaxX > L->bounds.MaxX)
        renderableLayerRect.MaxX = L->bounds.MaxX;
    if (renderableLayerRect.MinY < L->bounds.MinY)
        renderableLayerRect.MinY = L->bounds.MinY;
    if (renderableLayerRect.MaxY > L->bounds.MaxY)
        renderableLayerRect.MaxY = L->bounds.MaxY;

    
    /*  Do not clip renderableLayerRect to screen rast port. CRs are already clipped and unclipped 
        layer coords are needed. */
    
    CR = L->ClipRect;
    
    for (;NULL != CR; CR = CR->Next)
    {
        D(bug("Cliprect (%d, %d, %d, %d), lobs=%p\n",
            CR->bounds.MinX, CR->bounds.MinY, CR->bounds.MaxX, CR->bounds.MaxY,
            CR->lobs));

        /* I assume this means the cliprect is visible */
        if (NULL == CR->lobs)
        {
            struct Rectangle result;
            
            if (AndRectRect(&renderableLayerRect, &CR->bounds, &result))
            {
                /* This clip rect intersects renderable layer rect */
                struct pHidd_Gallium_DisplayResource drmsg = {
                mID : OOP_GetMethodID(IID_Hidd_Gallium, moHidd_Gallium_DisplayResource),
                resource : srcPipeResource,
                srcx : xSrc + result.MinX - L->bounds.MinX - xDest, /* x in the source buffer */
                srcy : ySrc + result.MinY - L->bounds.MinY - yDest, /* y in the source buffer */
                bitmap: destRP->BitMap,
                dstx : result.MinX, /* Absolute (on bitmap) X of dest blit */
                dsty : result.MinY, /* Absolute (on bitmap) Y of dest blit */
                width : result.MaxX - result.MinX + 1, /* width of the rect in source buffer */
                height : result.MaxY - result.MinY + 1, /* height of the rect in source buffer */
                };
                OOP_DoMethod(ws->driver, (OOP_Msg)&drmsg);
                
                copied = TRUE;
            }
        }
    }
    
    /* Notify the bitmap about blitting */
    if (copied)
    {
        struct pHidd_BitMap_UpdateRect urmsg = {
            mID     :   OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_UpdateRect),
            x       :   renderableLayerRect.MinX,
            y       :   renderableLayerRect.MinY,
            width   :   renderableLayerRect.MaxX - renderableLayerRect.MinX + 1,
            height  :   renderableLayerRect.MaxY - renderableLayerRect.MinY + 1
        };
        
        OOP_Object * bm = HIDD_BM_OBJ(destRP->BitMap);
        OOP_DoMethod(bm, (OOP_Msg)&urmsg);
    }

    UnlockLayerRom(L);    
    
    AROS_LIBFUNC_EXIT
}
