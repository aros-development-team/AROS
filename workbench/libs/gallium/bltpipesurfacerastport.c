/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "gallium_intern.h"
#include <graphics/rastport.h>
#include <proto/layers.h>
#include <proto/graphics.h>
#include <gallium/pipe/p_state.h>
#include <aros/debug.h>

#define HIDD_BM_OBJ(bitmap)     (*(OOP_Object **)&((bitmap)->Planes[0]))

/*****************************************************************************

    NAME */

      AROS_LH8(void, BltPipeSurfaceRastPort,

/*  SYNOPSIS */ 
      AROS_LHA(struct pipe_surface * , srcPipeSurface, A0),
      AROS_LHA(LONG                  , xSrc, D0),
      AROS_LHA(LONG                  , ySrc, D1),
      AROS_LHA(struct RastPort *     , destRP, A1),
      AROS_LHA(LONG                  , xDest, D2),
      AROS_LHA(LONG                  , yDest, D3),
      AROS_LHA(LONG                  , xSize, D4),
      AROS_LHA(LONG                  , ySize, D5),

/*  LOCATION */
      struct Library *, GalliumBase, 7, Gallium)

/*  NAME
 
    FUNCTION
    Copies part of pipe surface onto rast port. Clips output by using layers of
    rastport.
 
    INPUTS
	srcPipeSurface - Copy from this pipe surface.
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

    struct HIDDT_WinSys * ws = (struct HIDDT_WinSys *)srcPipeSurface->texture->screen->winsys;
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
        layer coords are needed: see surface_copy */
    
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
                struct pHidd_Gallium_DisplaySurface dsmsg = {
                mID : OOP_GetMethodID(IID_Hidd_Gallium, moHidd_Gallium_DisplaySurface),
                context : NULL, /* was: amesa->st->pipe, TODO: remove the need for passing context NOW IT DOES NOT WORK WITH NOUVEAU!!!!!!!!!*/
                rastport : destRP, /* TODO: remove the need for passing rastport */
                left : xSrc + result.MinX - L->bounds.MinX - xDest, /* x in the source buffer */
                top : ySrc + result.MinY - L->bounds.MinY - yDest, /* y in the source buffer */
                width : result.MaxX - result.MinX + 1, /* width of the rect in source buffer */
                height : result.MaxY - result.MinY + 1, /* height of the rect in source buffer */
                surface : srcPipeSurface,
                absx : result.MinX, /* Absolute (on screen) X of dest blit */
                absy : result.MinY, /* Absolute (on screen) Y of the dest blit */
                relx : result.MinX - L->bounds.MinX, /* Relative (on rastport) X of the desc blit */
                rely : result.MinY - L->bounds.MinY /* Relative (on rastport) Y of the desc blit */
                };
                OOP_DoMethod(ws->driver, (OOP_Msg)&dsmsg);
                            
                /* FIXME: clip last 4 parameters to actuall surface deminsions */
/*                pipe->surface_copy(pipe, visiblescreen, 
                            result.MinX, 
                            result.MinY, 
                            surface, 
                            result.MinX - L->bounds.MinX - msg->left, 
                            result.MinY - L->bounds.MinY - msg->top, 
                            result.MaxX - result.MinX + 1, 
                            result.MaxY - result.MinY + 1);*/
            }
        }
    }
    
    /* Notify the bitmap about blitting */
    /* TODO: don't do this call if nothing was blitted */
    /* TODO: are coords correct in respect to screen with moved LeftEdge/TopEdge ? */
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

    /* Flush all copy operations done */
//TODO: is this needed anymore?    pipe->flush(pipe, PIPE_FLUSH_RENDER_CACHE, NULL);


    UnlockLayerRom(L);    
    
    AROS_LIBFUNC_EXIT
}
