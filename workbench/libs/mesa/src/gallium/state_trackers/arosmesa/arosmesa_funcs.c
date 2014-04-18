/*
    Copyright 2009-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "arosmesa_funcs.h"
#include <proto/utility.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <cybergraphx/cybergraphics.h>
#include <graphics/rpattr.h>
#include <aros/debug.h>

VOID AROSMesaSelectRastPort(struct arosmesa_context * amesa, struct TagItem * tagList)
{
    amesa->Screen = (struct Screen *)GetTagData(GLA_Screen, 0, tagList);
    amesa->window = (struct Window *)GetTagData(GLA_Window, 0, tagList);
    amesa->visible_rp = (struct RastPort *)GetTagData(GLA_RastPort, 0, tagList);

    if (amesa->Screen)
    {
        D(bug("[AROSMESA] AROSMesaSelectRastPort: Screen @ %x\n", amesa->Screen));
        if (amesa->window)
        {
            D(bug("[AROSMESA] AROSMesaSelectRastPort: Window @ %x\n", amesa->window));
            if (!(amesa->visible_rp))
            {
                /* Use the windows rastport */
                amesa->visible_rp = amesa->window->RPort;
                D(bug("[AROSMESA] AROSMesaSelectRastPort: Windows RastPort @ %x\n", amesa->visible_rp));
            }
        }
        else
        {
            if (!(amesa->visible_rp))
            {
                /* Use the screens rastport */
                amesa->visible_rp = &amesa->Screen->RastPort;
                D(bug("[AROSMESA] AROSMesaSelectRastPort: Screens RastPort @ %x\n", amesa->visible_rp));
            }
        }
    }
    else
    {
        /* Not passed a screen */
        if (amesa->window)
        {
            D(bug("[AROSMESA] AROSMesaSelectRastPort: Window @ %x\n", amesa->window));
            /* Use the windows Screen */
            amesa->Screen = amesa->window->WScreen;
            D(bug("[AROSMESA] AROSMesaSelectRastPort: Windows Screen @ %x\n", amesa->Screen));

            if (!(amesa->visible_rp))
            {
                /* Use the windows rastport */
                amesa->visible_rp = amesa->window->RPort;
                D(bug("[AROSMESA] AROSMesaSelectRastPort: Windows RastPort @ %x\n", amesa->visible_rp));
            }
        }
        else
        {
            /* Only Passed A Rastport */
            D(bug("[AROSMESA] AROSMesaSelectRastPort: Using RastPort only!\n"));
        }
    }

    D(bug("[AROSMESA] AROSMesaSelectRastPort: Using RastPort @ %x\n", amesa->visible_rp));
}

BOOL AROSMesaStandardInit(struct arosmesa_context * amesa, struct TagItem *tagList)
{
    LONG requestedwidth = 0, requestedheight = 0;
    LONG requestedright = 0, requestedbottom = 0;
    LONG defaultleft = 0, defaulttop = 0;
    LONG defaultright = 0, defaultbottom = 0;

    /* Set the defaults based on window information */    
    if (amesa->window)
    {
        if(!(amesa->window->Flags & WFLG_GIMMEZEROZERO))
        {
            defaultleft     = amesa->window->BorderLeft;
            defaulttop      = amesa->window->BorderTop;
            defaultright    = amesa->window->BorderRight;
            defaultbottom   = amesa->window->BorderBottom;
        }
    }

    D(bug("[AROSMESA] AROSMesaStandardInit(amesa @ %x, taglist @ %x)\n", amesa, tagList));
    D(bug("[AROSMESA] AROSMesaStandardInit: Using RastPort @ %x\n", amesa->visible_rp));

    amesa->visible_rp = CloneRastPort(amesa->visible_rp);

    D(bug("[AROSMESA] AROSMesaStandardInit: Cloned RastPort @ %x\n", amesa->visible_rp));

    /* We assume left and top are given or if there is a window, set to border left/top 
       or if there is no window set to 0 */
    amesa->left = GetTagData(GLA_Left, defaultleft, tagList);
    amesa->top = GetTagData(GLA_Top, defaulttop, tagList);
    
    requestedright = GetTagData(GLA_Right, -1, tagList);
    requestedbottom = GetTagData(GLA_Bottom, -1, tagList);
    requestedwidth = GetTagData(GLA_Width, -1 , tagList);
    requestedheight = GetTagData(GLA_Height, -1 , tagList);

    /* Calculate rastport dimensions */
    amesa->visible_rp_width = 
        amesa->visible_rp->Layer->bounds.MaxX - amesa->visible_rp->Layer->bounds.MinX + 1;

    amesa->visible_rp_height = 
        amesa->visible_rp->Layer->bounds.MaxY - amesa->visible_rp->Layer->bounds.MinY + 1;
    
    /* right will be either passed or calculated from width or 0 */
    amesa->right = 0;
    if (requestedright < 0)
    {
        if (requestedwidth >= 0)
        {
            requestedright = amesa->visible_rp_width - amesa->left - requestedwidth;
            if (requestedright < 0) requestedright = 0;
        }
        else
            requestedright = defaultright; /* Set the default here, not in GetDataData */
    }
    amesa->right = requestedright;
    
    /* bottom will be either passed or calculated from height or 0 */
    amesa->bottom = 0;
    if (requestedbottom < 0)
    {
        if (requestedheight >= 0)
        {
            requestedbottom = amesa->visible_rp_height - amesa->top - requestedheight;
            if (requestedbottom < 0) requestedbottom = 0;
        }
        else
            requestedbottom = defaultbottom; /* Set the default here, not in GetDataData */
    }
    amesa->bottom = requestedbottom;
    
    /* Init screen information */
    if (amesa->Screen)
        amesa->BitsPerPixel  = GetCyberMapAttr(amesa->Screen->RastPort.BitMap, CYBRMATTR_BPPIX) * 8;
    
    D(bug("[AROSMESA] AROSMesaStandardInit: Context Base dimensions set -:\n"));
    D(bug("[AROSMESA] AROSMesaStandardInit:    amesa->visible_rp_width        = %d\n", amesa->visible_rp_width));
    D(bug("[AROSMESA] AROSMesaStandardInit:    amesa->visible_rp_height       = %d\n", amesa->visible_rp_height));
    D(bug("[AROSMESA] AROSMesaStandardInit:    amesa->left                    = %d\n", amesa->left));
    D(bug("[AROSMESA] AROSMesaStandardInit:    amesa->right                   = %d\n", amesa->right));
    D(bug("[AROSMESA] AROSMesaStandardInit:    amesa->top                     = %d\n", amesa->top));
    D(bug("[AROSMESA] AROSMesaStandardInit:    amesa->bottom                  = %d\n", amesa->bottom));

    return TRUE;
}

VOID AROSMesaRecalculateBufferWidthHeight(struct arosmesa_context * amesa)
{
    ULONG newwidth = 0;
    ULONG newheight = 0;
    
    D(bug("[AROSMESA] AROSMesaRecalculateBufferWidthHeight\n"));
    
    
    amesa->visible_rp_width = 
        amesa->visible_rp->Layer->bounds.MaxX - amesa->visible_rp->Layer->bounds.MinX + 1;

    amesa->visible_rp_height = 
        amesa->visible_rp->Layer->bounds.MaxY - amesa->visible_rp->Layer->bounds.MinY + 1;


    newwidth = amesa->visible_rp_width - amesa->left - amesa->right;
    newheight = amesa->visible_rp_height - amesa->top - amesa->bottom;
    
    if (newwidth < 0) newwidth = 0;
    if (newheight < 0) newheight = 0;
    
    
    if ((newwidth != amesa->framebuffer->width) || (newheight != amesa->framebuffer->height))
    {
        /* The drawing area size has changed. Buffer must change */
        D(bug("[AROSMESA] AROSMesaRecalculateBufferWidthHeight: current height    =   %d\n", amesa->framebuffer->height));
        D(bug("[AROSMESA] AROSMesaRecalculateBufferWidthHeight: current width     =   %d\n", amesa->framebuffer->width));
        D(bug("[AROSMESA] AROSMesaRecalculateBufferWidthHeight: new height        =   %d\n", newheight));
        D(bug("[AROSMESA] AROSMesaRecalculateBufferWidthHeight: new width         =   %d\n", newwidth));
        
        amesa->framebuffer->width = newwidth;
        amesa->framebuffer->height = newheight;
        amesa->framebuffer->resized = TRUE;
        
        if (amesa->window)
        {
            struct Rectangle rastcliprect;
            struct TagItem crptags[] =
            {
                { RPTAG_ClipRectangle      , (IPTR)&rastcliprect },
                { RPTAG_ClipRectangleFlags , (RPCRF_RELRIGHT | RPCRF_RELBOTTOM) },
                { TAG_DONE }
            };
        
            D(bug("[AROSMESA] AROSMesaRecalculateBufferWidthHeight: Clipping Rastport to Window's dimensions\n"));

            /* Clip the rastport to the visible area */
            rastcliprect.MinX = amesa->left;
            rastcliprect.MinY = amesa->top;
            rastcliprect.MaxX = amesa->left + amesa->framebuffer->width;
            rastcliprect.MaxY = amesa->top + amesa->framebuffer->height;
            SetRPAttrsA(amesa->visible_rp, crptags);
        }
    }
}

VOID AROSMesaFreeContext(struct arosmesa_context * amesa)
{
    if (amesa)
    {
        FreeVec(amesa);
    }
}
