/*
    Copyright 2009-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "arosmesa_funcs.h"
#include "main/context.h"
#include <proto/utility.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <cybergraphx/cybergraphics.h>
#include <graphics/rpattr.h>
#include <gallium/pipe/p_screen.h>
#include <gallium/util/u_inlines.h>
#include <proto/gallium.h>
#include <aros/debug.h>

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

static BOOL AROSMesaSelectColorFormat(enum pipe_format * colorFormat, 
    struct pipe_screen * screen, GLint bpp)
{
    *colorFormat = PIPE_FORMAT_NONE;

    if (bpp == 16)
    {
        /* Try PIPE_FORMAT_B5G6R5_UNORM */
        if (screen->is_format_supported(screen, PIPE_FORMAT_B5G6R5_UNORM,
            PIPE_TEXTURE_2D, 0, PIPE_BIND_RENDER_TARGET, 0))
        {
            *colorFormat = PIPE_FORMAT_B5G6R5_UNORM;
            return TRUE;
        }
    }
    
    if (bpp == 32)
    {
        /* Try PIPE_FORMAT_B8G8R8A8_UNORM */
        if (screen->is_format_supported(screen, PIPE_FORMAT_B8G8R8A8_UNORM,
            PIPE_TEXTURE_2D, 0, PIPE_BIND_RENDER_TARGET, 0))
        {
            *colorFormat = PIPE_FORMAT_B8G8R8A8_UNORM;
            return TRUE;
        }
    }
    
    return FALSE;
}

static BOOL AROSMesaSelectDepthStencilFormat(enum pipe_format * depthStencilFormat, 
    struct pipe_screen * screen, BOOL noDepth, BOOL noStencil)
{
    /* Defeaul values */
    *depthStencilFormat = PIPE_FORMAT_NONE;
    
    if (noDepth)
        return TRUE;
    
    /* Try PIPE_FORMAT_S8_USCALED_Z24_UNORM */
    if(!noStencil && (screen->is_format_supported(screen, PIPE_FORMAT_S8_USCALED_Z24_UNORM,
            PIPE_TEXTURE_2D, 0, PIPE_BIND_DEPTH_STENCIL, 0)))
    {
        *depthStencilFormat  = PIPE_FORMAT_S8_USCALED_Z24_UNORM;
        return TRUE;
    }
    
    /* Try PIPE_FORMAT_X8Z24_UNORM */
    if(noStencil && (screen->is_format_supported(screen, PIPE_FORMAT_X8Z24_UNORM,
            PIPE_TEXTURE_2D, 0, PIPE_BIND_DEPTH_STENCIL, 0)))
    {
        *depthStencilFormat  = PIPE_FORMAT_X8Z24_UNORM;
        return TRUE;
    }

    /* Try PIPE_FORMAT_Z24X8_UNORM */
    if(noStencil && (screen->is_format_supported(screen, PIPE_FORMAT_Z24X8_UNORM,
            PIPE_TEXTURE_2D, 0, PIPE_BIND_DEPTH_STENCIL, 0)))
    {
        *depthStencilFormat  = PIPE_FORMAT_Z24X8_UNORM;
        return TRUE;
    }
    
    /* Try PIPE_FORMAT_Z16_UNORM */
    if(screen->is_format_supported(screen, PIPE_FORMAT_Z16_UNORM,
            PIPE_TEXTURE_2D, 0, PIPE_BIND_DEPTH_STENCIL, 0))
    {
        *depthStencilFormat = PIPE_FORMAT_Z16_UNORM;
        return TRUE;
    }
    
    return FALSE;
}

BOOL AROSMesaFillVisual(struct st_visual * stvis, struct pipe_screen * screen, int bpp, struct TagItem *tagList)
{
    BOOL noDepth, noStencil, noAccum;
    
    D(bug("[AROSMESA] AROSMesaFillVisual\n"));
    
    noStencil   = GetTagData(AMA_NoStencil, GL_FALSE, tagList);
    noAccum     = GetTagData(AMA_NoAccum, GL_FALSE, tagList);
    noDepth     = GetTagData(AMA_NoDepth, GL_FALSE, tagList);

    stvis->color_format = PIPE_FORMAT_NONE;
    stvis->depth_stencil_format = PIPE_FORMAT_NONE;
    stvis->accum_format = PIPE_FORMAT_NONE;
    stvis->render_buffer = ST_ATTACHMENT_INVALID;
    stvis->samples = 0;
    stvis->buffer_mask = 0;
    
    /* Color buffer */
    if (!AROSMesaSelectColorFormat(&stvis->color_format, screen, bpp))
    {
        D(bug("[AROSMESA] AROSMesaFillVisual - ERROR - No supported color format found\n"));        
        return FALSE;
    } 
    
    /* Z-buffer / Stencil buffer */
    if (!AROSMesaSelectDepthStencilFormat(&stvis->depth_stencil_format, screen, noDepth, noStencil))
    {
        D(bug("[AROSMESA] AROSMesaFillVisual - ERROR - No supported depth/stencil format found\n"));        
        return FALSE;
    }

    /* Accum buffer */
    if (noAccum)
        stvis->accum_format = PIPE_FORMAT_NONE;
    else
        stvis->accum_format = PIPE_FORMAT_R16G16B16A16_SNORM;
    
    /* Buffers */ /* AROSMesa uses front buffer as back buffer */
    stvis->buffer_mask |= ST_ATTACHMENT_FRONT_LEFT_MASK;
    if (!noDepth || !noStencil)
    stvis->buffer_mask |= ST_ATTACHMENT_DEPTH_STENCIL_MASK;
    
    return TRUE;
}

VOID AROSMesaRecalculateBufferWidthHeight(AROSMesaContext amesa)
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

static VOID AROSMesaFrameBufferCreateResource(struct arosmesa_framebuffer * amfb,
    const enum st_attachment_type statt)
{
    struct pipe_resource templ;

    memset(&templ, 0, sizeof(templ));

    if(amfb->screen->get_param(amfb->screen, PIPE_CAP_NPOT_TEXTURES))
        templ.target = PIPE_TEXTURE_2D;
    else
        templ.target = PIPE_TEXTURE_RECT;
    templ.width0 = amfb->width;
    templ.height0 = amfb->height;
    templ.depth0 = 1;
    templ.last_level = 0;
    switch(statt)
    {
    case ST_ATTACHMENT_FRONT_LEFT:
    case ST_ATTACHMENT_BACK_LEFT:
    case ST_ATTACHMENT_FRONT_RIGHT:
    case ST_ATTACHMENT_BACK_RIGHT:
        templ.format    = amfb->stvis.color_format;
        templ.bind      = PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW;
        break;
    case ST_ATTACHMENT_DEPTH_STENCIL:
        templ.format    = amfb->stvis.depth_stencil_format;
        templ.bind      = PIPE_BIND_DEPTH_STENCIL;
        break;
    default:
        return; /* Failure */
    }
    
    /* Create resource */
    amfb->textures[statt] = amfb->screen->resource_create(amfb->screen, &templ);
}

static boolean AROSMesaFrameBufferValidate(struct st_framebuffer_iface *stfbi,
    const enum st_attachment_type *statts, unsigned count, struct pipe_resource **out)
{
    struct arosmesa_framebuffer * amfb = (struct arosmesa_framebuffer *)stfbi;
    LONG i;

    /* Check for resize */
    if (amfb->resized)
    {
        amfb->resized = FALSE;
        /* Detach "front surface" */
        pipe_surface_reference(&amfb->render_surface, NULL);

        /* Detach all resources */
        for (i = 0; i < ST_ATTACHMENT_COUNT; i++)
            pipe_resource_reference(&amfb->textures[i], NULL);
    }
    
    /* Create new resources */
    for (i = 0; i < count; i++)
    {
        if (amfb->textures[statts[i]] == NULL)
        {
            AROSMesaFrameBufferCreateResource(amfb, statts[i]);
            if (statts[i] == ST_ATTACHMENT_FRONT_LEFT)
            {
                amfb->render_surface = amfb->screen->get_tex_surface(amfb->screen, 
                    amfb->textures[ST_ATTACHMENT_FRONT_LEFT], 0, 0, 0, 
                    PIPE_BIND_RENDER_TARGET);
            }
        }
    }

    if (!out)
        return TRUE;

    for (i = 0; i < count; i++) 
    {
        out[i] = NULL;
        pipe_resource_reference(&out[i], amfb->textures[statts[i]]);
    }
    
    return TRUE;
}

static boolean AROSMesaFrameBufferFlushFront(struct st_framebuffer_iface *stfbi,
    enum st_attachment_type statt)
{
    /* No Op */
    return TRUE;
}

struct arosmesa_framebuffer * AROSMesaNewFrameBuffer(AROSMesaContext amesa, struct st_visual * stvis)
{
    struct arosmesa_framebuffer * framebuffer = 
        AllocVec(sizeof(struct arosmesa_framebuffer), MEMF_PUBLIC | MEMF_CLEAR);

    if (!framebuffer)
        return NULL;

    framebuffer->stvis = *stvis;
    framebuffer->base.visual = &framebuffer->stvis;
    framebuffer->base.flush_front = AROSMesaFrameBufferFlushFront;
    framebuffer->base.validate = AROSMesaFrameBufferValidate;
    framebuffer->screen = amesa->stmanager->screen;

    return framebuffer;
}

VOID AROSMesaDestroyContext(AROSMesaContext amesa)
{
    if (amesa)
    {
        FreeVec(amesa);
    }
}

VOID AROSMesaDestroyFrameBuffer(struct arosmesa_framebuffer * framebuffer)
{
    if (framebuffer)
    {
        LONG i;

        pipe_surface_reference(&framebuffer->render_surface, NULL);

        for (i = 0; i < ST_ATTACHMENT_COUNT; i++)
            pipe_resource_reference(&framebuffer->textures[i], NULL);        

        FreeVec(framebuffer);
    }
}

VOID AROSMesaCheckAndUpdateBufferSize(AROSMesaContext amesa)
{
    AROSMesaRecalculateBufferWidthHeight(amesa);
    if (amesa->framebuffer->resized)
        amesa->st->notify_invalid_framebuffer(amesa->st, 
            (struct st_framebuffer_iface *) amesa->framebuffer);
}

static int AROSMesaStManagerGetParam(struct st_manager *smapi,
                enum st_manager_param param)
{
    return 0;
}

struct st_manager * AROSMesaNewStManager(struct pipe_screen * pscreen)
{
    struct st_manager * stmanager = 
        (struct st_manager *)AllocVec(sizeof(struct st_manager), MEMF_PUBLIC | MEMF_CLEAR);

    if (stmanager)
    {
        stmanager->screen = pscreen;
        stmanager->get_param = AROSMesaStManagerGetParam;
    }
    
    return stmanager;
}

VOID AROSMesaDestroyStManager(struct st_manager * stmanager)
{
    if (stmanager)
    {
        if (stmanager->screen)
            DestroyPipeScreen(stmanager->screen);
        FreeVec(stmanager);
    }
}
