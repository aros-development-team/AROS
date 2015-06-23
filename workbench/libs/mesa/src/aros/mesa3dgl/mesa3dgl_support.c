/*
    Copyright 2009-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <proto/utility.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>

#include <cybergraphx/cybergraphics.h>
#include <graphics/rpattr.h>

#include "mesa3dgl_support.h"

VOID MESA3DGLSelectRastPort(struct mesa3dgl_context * ctx, struct TagItem * tagList)
{
    ctx->Screen = (struct Screen *)GetTagData(GLA_Screen, 0, tagList);
    ctx->window = (struct Window *)GetTagData(GLA_Window, 0, tagList);
    ctx->visible_rp = (struct RastPort *)GetTagData(GLA_RastPort, 0, tagList);

    if (ctx->Screen)
    {
        D(bug("[MESA3DGL] MESA3DGLSelectRastPort: Screen @ %x\n", ctx->Screen));
        if (ctx->window)
        {
            D(bug("[MESA3DGL] MESA3DGLSelectRastPort: Window @ %x\n", ctx->window));
            if (!(ctx->visible_rp))
            {
                /* Use the windows rastport */
                ctx->visible_rp = ctx->window->RPort;
                D(bug("[MESA3DGL] MESA3DGLSelectRastPort: Windows RastPort @ %x\n", ctx->visible_rp));
            }
        }
        else
        {
            if (!(ctx->visible_rp))
            {
                /* Use the screens rastport */
                ctx->visible_rp = &ctx->Screen->RastPort;
                D(bug("[MESA3DGL] MESA3DGLSelectRastPort: Screens RastPort @ %x\n", ctx->visible_rp));
            }
        }
    }
    else
    {
        /* Not passed a screen */
        if (ctx->window)
        {
            D(bug("[MESA3DGL] MESA3DGLSelectRastPort: Window @ %x\n", ctx->window));
            /* Use the windows Screen */
            ctx->Screen = ctx->window->WScreen;
            D(bug("[MESA3DGL] MESA3DGLSelectRastPort: Windows Screen @ %x\n", ctx->Screen));

            if (!(ctx->visible_rp))
            {
                /* Use the windows rastport */
                ctx->visible_rp = ctx->window->RPort;
                D(bug("[MESA3DGL] MESA3DGLSelectRastPort: Windows RastPort @ %x\n", ctx->visible_rp));
            }
        }
        else
        {
            /* Only Passed A Rastport */
            D(bug("[MESA3DGL] MESA3DGLSelectRastPort: Using RastPort only!\n"));
        }
    }

    D(bug("[MESA3DGL] MESA3DGLSelectRastPort: Using RastPort @ %x\n", ctx->visible_rp));
}

BOOL MESA3DGLStandardInit(struct mesa3dgl_context * ctx, struct TagItem *tagList)
{
    LONG requestedwidth = 0, requestedheight = 0;
    LONG requestedright = 0, requestedbottom = 0;
    LONG defaultleft = 0, defaulttop = 0;
    LONG defaultright = 0, defaultbottom = 0;

    /* Set the defaults based on window information */    
    if (ctx->window)
    {
        if(!(ctx->window->Flags & WFLG_GIMMEZEROZERO))
        {
            defaultleft     = ctx->window->BorderLeft;
            defaulttop      = ctx->window->BorderTop;
            defaultright    = ctx->window->BorderRight;
            defaultbottom   = ctx->window->BorderBottom;
        }
    }

    D(bug("[MESA3DGL] MESA3DGLStandardInit(ctx @ %x, taglist @ %x)\n", ctx, tagList));
    D(bug("[MESA3DGL] MESA3DGLStandardInit: Using RastPort @ %x\n", ctx->visible_rp));

    ctx->visible_rp = CloneRastPort(ctx->visible_rp);

    D(bug("[MESA3DGL] MESA3DGLStandardInit: Cloned RastPort @ %x\n", ctx->visible_rp));

    /* We assume left and top are given or if there is a window, set to border left/top 
       or if there is no window set to 0 */
    ctx->left = GetTagData(GLA_Left, defaultleft, tagList);
    ctx->top = GetTagData(GLA_Top, defaulttop, tagList);
    
    requestedright = GetTagData(GLA_Right, -1, tagList);
    requestedbottom = GetTagData(GLA_Bottom, -1, tagList);
    requestedwidth = GetTagData(GLA_Width, -1 , tagList);
    requestedheight = GetTagData(GLA_Height, -1 , tagList);

    /* Calculate rastport dimensions */
    ctx->visible_rp_width = 
        ctx->visible_rp->Layer->bounds.MaxX - ctx->visible_rp->Layer->bounds.MinX + 1;

    ctx->visible_rp_height = 
        ctx->visible_rp->Layer->bounds.MaxY - ctx->visible_rp->Layer->bounds.MinY + 1;
    
    /* right will be either passed or calculated from width or 0 */
    ctx->right = 0;
    if (requestedright < 0)
    {
        if (requestedwidth >= 0)
        {
            requestedright = ctx->visible_rp_width - ctx->left - requestedwidth;
            if (requestedright < 0) requestedright = 0;
        }
        else
            requestedright = defaultright; /* Set the default here, not in GetDataData */
    }
    ctx->right = requestedright;
    
    /* bottom will be either passed or calculated from height or 0 */
    ctx->bottom = 0;
    if (requestedbottom < 0)
    {
        if (requestedheight >= 0)
        {
            requestedbottom = ctx->visible_rp_height - ctx->top - requestedheight;
            if (requestedbottom < 0) requestedbottom = 0;
        }
        else
            requestedbottom = defaultbottom; /* Set the default here, not in GetDataData */
    }
    ctx->bottom = requestedbottom;
    
    /* Init screen information */
    if (ctx->Screen)
        ctx->BitsPerPixel  = GetCyberMapAttr(ctx->Screen->RastPort.BitMap, CYBRMATTR_BPPIX) * 8;
    
    D(bug("[MESA3DGL] MESA3DGLStandardInit: Context Base dimensions set -:\n"));
    D(bug("[MESA3DGL] MESA3DGLStandardInit:    ctx->visible_rp_width        = %d\n", ctx->visible_rp_width));
    D(bug("[MESA3DGL] MESA3DGLStandardInit:    ctx->visible_rp_height       = %d\n", ctx->visible_rp_height));
    D(bug("[MESA3DGL] MESA3DGLStandardInit:    ctx->left                    = %d\n", ctx->left));
    D(bug("[MESA3DGL] MESA3DGLStandardInit:    ctx->right                   = %d\n", ctx->right));
    D(bug("[MESA3DGL] MESA3DGLStandardInit:    ctx->top                     = %d\n", ctx->top));
    D(bug("[MESA3DGL] MESA3DGLStandardInit:    ctx->bottom                  = %d\n", ctx->bottom));

    return TRUE;
}

VOID MESA3DGLRecalculateBufferWidthHeight(struct mesa3dgl_context * ctx)
{
    ULONG newwidth = 0;
    ULONG newheight = 0;
    
    D(bug("[MESA3DGL] MESA3DGLRecalculateBufferWidthHeight\n"));
    
    
    ctx->visible_rp_width = 
        ctx->visible_rp->Layer->bounds.MaxX - ctx->visible_rp->Layer->bounds.MinX + 1;

    ctx->visible_rp_height = 
        ctx->visible_rp->Layer->bounds.MaxY - ctx->visible_rp->Layer->bounds.MinY + 1;


    newwidth = ctx->visible_rp_width - ctx->left - ctx->right;
    newheight = ctx->visible_rp_height - ctx->top - ctx->bottom;
    
    if (newwidth < 0) newwidth = 0;
    if (newheight < 0) newheight = 0;
    
    
    if ((newwidth != ctx->framebuffer->width) || (newheight != ctx->framebuffer->height))
    {
        /* The drawing area size has changed. Buffer must change */
        D(bug("[MESA3DGL] MESA3DGLRecalculateBufferWidthHeight: current height    =   %d\n", ctx->framebuffer->height));
        D(bug("[MESA3DGL] MESA3DGLRecalculateBufferWidthHeight: current width     =   %d\n", ctx->framebuffer->width));
        D(bug("[MESA3DGL] MESA3DGLRecalculateBufferWidthHeight: new height        =   %d\n", newheight));
        D(bug("[MESA3DGL] MESA3DGLRecalculateBufferWidthHeight: new width         =   %d\n", newwidth));
        
        ctx->framebuffer->width = newwidth;
        ctx->framebuffer->height = newheight;
        ctx->framebuffer->resized = TRUE;
        
        if (ctx->window)
        {
            struct Rectangle rastcliprect;
            struct TagItem crptags[] =
            {
                { RPTAG_ClipRectangle      , (IPTR)&rastcliprect },
                { RPTAG_ClipRectangleFlags , (RPCRF_RELRIGHT | RPCRF_RELBOTTOM) },
                { TAG_DONE }
            };
        
            D(bug("[MESA3DGL] MESA3DGLRecalculateBufferWidthHeight: Clipping Rastport to Window's dimensions\n"));

            /* Clip the rastport to the visible area */
            rastcliprect.MinX = ctx->left;
            rastcliprect.MinY = ctx->top;
            rastcliprect.MaxX = ctx->left + ctx->framebuffer->width;
            rastcliprect.MaxY = ctx->top + ctx->framebuffer->height;
            SetRPAttrsA(ctx->visible_rp, crptags);
        }
    }
}

VOID MESA3DGLFreeContext(struct mesa3dgl_context * ctx)
{
    if (ctx)
    {
        FreeVec(ctx);
    }
}
