/*
    Copyright 2009-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "arosmesa_funcs.h"
#include "state_tracker/st_public.h"
#include "main/context.h"
#include <aros/debug.h>
#include <proto/utility.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <cybergraphx/cybergraphics.h>
#include <graphics/rpattr.h>

VOID AROSMesaSelectRastPort(AROSMesaContext amesa, struct TagItem * tagList)
{
    amesa->ScreenInfo.Screen = (struct Screen *)GetTagData(AMA_Screen, 0, tagList);
    amesa->window = (struct Window *)GetTagData(AMA_Window, 0, tagList);
    amesa->visible_rp = (struct RastPort *)GetTagData(AMA_RastPort, 0, tagList);

    if (amesa->ScreenInfo.Screen)
    {
        D(bug("[AROSMESA] AROSMesaSelectRastPort: Screen @ %x\n", amesa->ScreenInfo.Screen));
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
                amesa->visible_rp = &amesa->ScreenInfo.Screen->RastPort;
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
            amesa->ScreenInfo.Screen = amesa->window->WScreen;
            D(bug("[AROSMESA] AROSMesaSelectRastPort: Windows Screen @ %x\n", amesa->ScreenInfo.Screen));

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

BOOL AROSMesaStandardInit(AROSMesaContext amesa, struct TagItem *tagList)
{
    GLint requestedwidth = 0, requestedheight = 0;
    GLint requestedright = 0, requestedbottom = 0;

    D(bug("[AROSMESA] AROSMesaStandardInit(amesa @ %x, taglist @ %x)\n", amesa, tagList));
    D(bug("[AROSMESA] AROSMesaStandardInit: Using RastPort @ %x\n", amesa->visible_rp));

    amesa->visible_rp = CloneRastPort(amesa->visible_rp);

    D(bug("[AROSMESA] AROSMesaStandardInit: Cloned RastPort @ %x\n", amesa->visible_rp));

    /* We assume left and top are given or set to 0 */
    amesa->left = GetTagData(AMA_Left, 0, tagList);
    amesa->top = GetTagData(AMA_Top, 0, tagList);
    
    requestedright = GetTagData(AMA_Right, -1, tagList);
    requestedbottom = GetTagData(AMA_Bottom, -1, tagList);
    requestedwidth = GetTagData(AMA_Width, -1 , tagList);
    requestedheight = GetTagData(AMA_Height, -1 , tagList);

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
            requestedright = 0;
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
            requestedbottom = 0;
    }
    amesa->bottom = requestedbottom;
    
    /* Init screen information */
    if (amesa->ScreenInfo.Screen)
    {
        amesa->ScreenInfo.Depth         = GetCyberMapAttr(amesa->ScreenInfo.Screen->RastPort.BitMap, CYBRMATTR_DEPTH);
        amesa->ScreenInfo.BitsPerPixel  = GetCyberMapAttr(amesa->ScreenInfo.Screen->RastPort.BitMap, CYBRMATTR_BPPIX) * 8;
        amesa->ScreenInfo.Width         = GetCyberMapAttr(amesa->ScreenInfo.Screen->RastPort.BitMap, CYBRMATTR_WIDTH);
        amesa->ScreenInfo.Height        = GetCyberMapAttr(amesa->ScreenInfo.Screen->RastPort.BitMap, CYBRMATTR_HEIGHT);
    }
    
    D(bug("[AROSMESA] AROSMesaStandardInit: Context Base dimensions set -:\n"));
    D(bug("[AROSMESA] AROSMesaStandardInit:    amesa->visible_rp_width        = %d\n", amesa->visible_rp_width));
    D(bug("[AROSMESA] AROSMesaStandardInit:    amesa->visible_rp_height       = %d\n", amesa->visible_rp_height));
    D(bug("[AROSMESA] AROSMesaStandardInit:    amesa->width                   = %d\n", amesa->width));
    D(bug("[AROSMESA] AROSMesaStandardInit:    amesa->height                  = %d\n", amesa->height));
    D(bug("[AROSMESA] AROSMesaStandardInit:    amesa->left                    = %d\n", amesa->left));
    D(bug("[AROSMESA] AROSMesaStandardInit:    amesa->right                   = %d\n", amesa->right));
    D(bug("[AROSMESA] AROSMesaStandardInit:    amesa->top                     = %d\n", amesa->top));
    D(bug("[AROSMESA] AROSMesaStandardInit:    amesa->bottom                  = %d\n", amesa->bottom));
    D(bug("[AROSMESA] AROSMesaStandardInit:    amesa->depth                   = %d\n", amesa->ScreenInfo.Depth));

    return TRUE;
}

static VOID AROSMesaSelectColorFormat(GLint bpp, struct pipe_screen * screen, 
    GLint * redBits, GLint * greenBits, GLint * blueBits, GLint * alphaBits,
    enum pipe_format * colorFormat)
{
    *redBits        = 0;
    *greenBits      = 0;
    *blueBits       = 0;
    *alphaBits      = 0;
    *colorFormat    = PIPE_FORMAT_NONE;

    if (bpp == 16)
    {
        /* Try PIPE_FORMAT_B5G6R5_UNORM */
        if (screen->is_format_supported(screen, PIPE_FORMAT_B5G6R5_UNORM,
            PIPE_TEXTURE_2D, PIPE_TEXTURE_USAGE_RENDER_TARGET, 0))
        {
            *redBits        = 5;
            *greenBits      = 6;
            *blueBits       = 5;
            *alphaBits      = 0;
            *colorFormat    = PIPE_FORMAT_B5G6R5_UNORM;
        }
    }
    
    if (bpp == 32)
    {
        /* Try PIPE_FORMAT_B8G8R8A8_UNORM */
        if (screen->is_format_supported(screen, PIPE_FORMAT_B5G6R5_UNORM,
            PIPE_TEXTURE_2D, PIPE_TEXTURE_USAGE_RENDER_TARGET, 0))
        {
            *redBits        = 8;
            *greenBits      = 8;
            *blueBits       = 8;
            *alphaBits      = 8;
            *colorFormat    = PIPE_FORMAT_B8G8R8A8_UNORM;
        }
    }    
}

static VOID AROSMesaSelectDepthStencilFormat(struct pipe_screen * screen,
    GLint * depthBits, enum pipe_format * depthFormat, 
    GLint * stencilBits, enum pipe_format * stencilFormat)
{
    /* Defeaul values */
    *depthBits      = 0;
    *depthFormat    = PIPE_FORMAT_NONE;
    *stencilBits    = 0;
    *stencilFormat  = PIPE_FORMAT_NONE;
    
    /* Try PIPE_FORMAT_S8Z24_UNORM */
    if((*depthFormat == PIPE_FORMAT_NONE) && (*stencilFormat == PIPE_FORMAT_NONE) &&
        (screen->is_format_supported(screen, PIPE_FORMAT_S8Z24_UNORM,
            PIPE_TEXTURE_2D, PIPE_TEXTURE_USAGE_DEPTH_STENCIL, 0)))
    {
        *depthBits      = 24;
        *depthFormat    = PIPE_FORMAT_S8Z24_UNORM;
        *stencilBits    = 8;
        *stencilFormat  = PIPE_FORMAT_S8Z24_UNORM;
    }
    
    /* Try PIPE_FORMAT_Z16_UNORM */
    if((*depthFormat == PIPE_FORMAT_NONE) && 
        (screen->is_format_supported(screen, PIPE_FORMAT_Z16_UNORM,
            PIPE_TEXTURE_2D, PIPE_TEXTURE_USAGE_DEPTH_STENCIL, 0)))
    {
        *depthBits      = 16;
        *depthFormat    = PIPE_FORMAT_Z16_UNORM;
    }
    
    /* Try PIPE_FORMAT_Z16_UNORM */
    if((*stencilFormat == PIPE_FORMAT_NONE) && 
        (screen->is_format_supported(screen, PIPE_FORMAT_S8_UNORM,
            PIPE_TEXTURE_2D, PIPE_TEXTURE_USAGE_DEPTH_STENCIL, 0)))
    {
        *stencilBits    = 8;
        *stencilFormat  = PIPE_FORMAT_S8_UNORM;
    }
}

AROSMesaVisual AROSMesaNewVisual(GLint bpp, struct pipe_screen * screen, struct TagItem *tagList)
{
    AROSMesaVisual aros_vis = NULL;
    GLvisual * vis = NULL;
    GLint  redBits, greenBits, blueBits, alphaBits, accumBits;
    GLint depthBits, stencilBits;
    BOOL noDepth, noStencil, noAccum;
    
    D(bug("[AROSMESA] AROSMesaNewVisual\n"));
    
    noStencil   = GetTagData(AMA_NoStencil, GL_FALSE, tagList);
    noAccum     = GetTagData(AMA_NoAccum, GL_FALSE, tagList);
    noDepth     = GetTagData(AMA_NoDepth, GL_FALSE, tagList);

    /* Allocate memory for aros structure */
    aros_vis = AllocVec(sizeof(struct arosmesa_visual), MEMF_PUBLIC | MEMF_CLEAR);

    if (!aros_vis)
        return NULL;

    /* Color buffer */
    AROSMesaSelectColorFormat(bpp, screen, &redBits, &greenBits, &blueBits,
        &alphaBits, &aros_vis->ColorFormat);
    if (aros_vis->ColorFormat == PIPE_FORMAT_NONE)
    {
        D(bug("[AROSMESA] AROSMesaNewVisual - ERROR - No supported color format found\n"));        
        AROSMesaDestroyVisual(aros_vis);
        return NULL;        
    } 
    
    /* Z-buffer / Stencil buffer */
    AROSMesaSelectDepthStencilFormat(screen, &depthBits, &aros_vis->DepthFormat, 
        &stencilBits, &aros_vis->StencilFormat);
    if (noDepth)
    {
        depthBits = 0;
        aros_vis->DepthFormat = PIPE_FORMAT_NONE;
    }
    else
        if (aros_vis->DepthFormat == PIPE_FORMAT_NONE)
        {
            D(bug("[AROSMESA] AROSMesaNewVisual - ERROR - No supported depth format found\n"));        
            AROSMesaDestroyVisual(aros_vis);
            return NULL;        
        }

    if (noStencil)
    {
        stencilBits = 0;
        aros_vis->StencilFormat = PIPE_FORMAT_NONE;
    }
    else
        if (aros_vis->StencilFormat == PIPE_FORMAT_NONE)
        {
            D(bug("[AROSMESA] AROSMesaNewVisual - ERROR - No supported stencil format found\n"));        
            AROSMesaDestroyVisual(aros_vis);
            return NULL;        
        }

    /* Accum buffer */
    if (noAccum)
        accumBits = 0;
    else
        accumBits = 16;
    
    
    /* AMA_RGBMode, AMA_DoubleBuf and AMA_AlphaFlag are always GL_TRUE in this implementation */

    vis = GET_GL_VIS_PTR(aros_vis);

    /* Initialize mesa structure */
    if(!_mesa_initialize_visual(vis,
                                GL_FALSE,       /* Double buffer - AROSMesa uses front buffer as back buffer */
                                GL_FALSE,       /* stereo */
                                redBits,
                                greenBits,
                                blueBits,
                                alphaBits,
                                depthBits,
                                stencilBits,
                                accumBits,
                                accumBits,
                                accumBits,
                                alphaBits ? accumBits : 0,
                                0))
    {
        AROSMesaDestroyVisual(aros_vis);
        return NULL;
    }

    return aros_vis;
}

GLboolean AROSMesaRecalculateBufferWidthHeight(AROSMesaContext amesa)
{
    GLsizei newwidth = 0;
    GLsizei newheight = 0;
    
    D(bug("[AROSMESA] AROSMesaRecalculateBufferWidthHeight\n"));
    
    
    amesa->visible_rp_width = 
        amesa->visible_rp->Layer->bounds.MaxX - amesa->visible_rp->Layer->bounds.MinX + 1;

    amesa->visible_rp_height = 
        amesa->visible_rp->Layer->bounds.MaxY - amesa->visible_rp->Layer->bounds.MinY + 1;


    newwidth = amesa->visible_rp_width - amesa->left - amesa->right;
    newheight = amesa->visible_rp_height - amesa->top - amesa->bottom;
    
    if (newwidth < 0) newwidth = 0;
    if (newheight < 0) newheight = 0;
    
    
    if ((newwidth != amesa->width) || (newheight != amesa->height))
    {
        /* The drawing area size has changed. Buffer must change */
        D(bug("[AROSMESA] AROSMesaRecalculateBufferWidthHeight: current height    =   %d\n", amesa->height));
        D(bug("[AROSMESA] AROSMesaRecalculateBufferWidthHeight: current width     =   %d\n", amesa->width));
        D(bug("[AROSMESA] AROSMesaRecalculateBufferWidthHeight: new height        =   %d\n", newheight));
        D(bug("[AROSMESA] AROSMesaRecalculateBufferWidthHeight: new width         =   %d\n", newwidth));
        
        amesa->width = newwidth;
        amesa->height = newheight;
        
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
            rastcliprect.MaxX = amesa->left + amesa->width;
            rastcliprect.MaxY = amesa->top + amesa->height;
            SetRPAttrsA(amesa->visible_rp, crptags);
        }
        
        return GL_TRUE;
    }
    
    return GL_FALSE;
}

AROSMesaFrameBuffer AROSMesaNewFrameBuffer(AROSMesaContext amesa, AROSMesaVisual visual)
{
    AROSMesaFrameBuffer aros_fb = NULL;
    GLvisual * vis = GET_GL_VIS_PTR(visual);
    
    D(bug("[AROSMESA] AROSMesaNewFrameBuffer\n"));

    /* Allocate memory for aros structure */
    aros_fb = AllocVec(sizeof(struct arosmesa_framebuffer), MEMF_PUBLIC | MEMF_CLEAR);

    if (!aros_fb)
        return NULL;
    
    /* Create framebuffer */
    aros_fb->stfb = st_create_framebuffer(vis,
                                    visual->ColorFormat, visual->DepthFormat, 
                                    visual->StencilFormat, amesa->width, 
                                    amesa->height, (void *) aros_fb);    
    
    return aros_fb;
}

VOID AROSMesaDestroyContext(AROSMesaContext amesa)
{
    if (amesa)
    {
        FreeVec(amesa);
    }
}

VOID AROSMesaDestroyVisual(AROSMesaVisual aros_vis)
{
    if (aros_vis)
    {
        FreeVec(aros_vis);
    }
}

VOID AROSMesaDestroyFrameBuffer(AROSMesaFrameBuffer aros_fb)
{
    if (aros_fb)
    {
        if (aros_fb->stfb)
        {
            /* So that reference count goes to 0 and buffer is freed */
            st_unreference_framebuffer(aros_fb->stfb);
            aros_fb->stfb = NULL;
        }
        
        FreeVec(aros_fb);
    }
}

VOID AROSMesaCheckAndUpdateBufferSize(AROSMesaContext amesa)
{
    if (AROSMesaRecalculateBufferWidthHeight(amesa))
        st_resize_framebuffer(amesa->framebuffer->stfb, amesa->width, amesa->height);
}

