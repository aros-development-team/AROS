/*
    Copyright 2009-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "arosmesa_funcs.h"
#include "arosmesa_funcs_gallium.h"
#include "main/context.h"
#include <proto/utility.h>
#include <proto/exec.h>
#include <gallium/pipe/p_screen.h>
#include <gallium/util/u_inlines.h>
#include <proto/gallium.h>
#include <aros/debug.h>

static BOOL AROSMesaSelectColorFormat(enum pipe_format * colorFormat, 
    struct pipe_screen * screen, GLint bpp)
{
    *colorFormat = PIPE_FORMAT_NONE;

    if (bpp == 16)
    {
        /* Try PIPE_FORMAT_B5G6R5_UNORM */
        if (screen->is_format_supported(screen, PIPE_FORMAT_B5G6R5_UNORM,
            PIPE_TEXTURE_2D, 0, PIPE_BIND_RENDER_TARGET))
        {
            *colorFormat = PIPE_FORMAT_B5G6R5_UNORM;
            return TRUE;
        }
    }
    
    if (bpp == 32)
    {
        /* Try PIPE_FORMAT_B8G8R8A8_UNORM */
        if (screen->is_format_supported(screen, PIPE_FORMAT_B8G8R8A8_UNORM,
            PIPE_TEXTURE_2D, 0, PIPE_BIND_RENDER_TARGET))
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
            PIPE_TEXTURE_2D, 0, PIPE_BIND_DEPTH_STENCIL)))
    {
        *depthStencilFormat  = PIPE_FORMAT_S8_USCALED_Z24_UNORM;
        return TRUE;
    }
    
    /* Try PIPE_FORMAT_X8Z24_UNORM */
    if(noStencil && (screen->is_format_supported(screen, PIPE_FORMAT_X8Z24_UNORM,
            PIPE_TEXTURE_2D, 0, PIPE_BIND_DEPTH_STENCIL)))
    {
        *depthStencilFormat  = PIPE_FORMAT_X8Z24_UNORM;
        return TRUE;
    }

    /* Try PIPE_FORMAT_Z24X8_UNORM */
    if(noStencil && (screen->is_format_supported(screen, PIPE_FORMAT_Z24X8_UNORM,
            PIPE_TEXTURE_2D, 0, PIPE_BIND_DEPTH_STENCIL)))
    {
        *depthStencilFormat  = PIPE_FORMAT_Z24X8_UNORM;
        return TRUE;
    }
    
    /* Try PIPE_FORMAT_Z16_UNORM */
    if(screen->is_format_supported(screen, PIPE_FORMAT_Z16_UNORM,
            PIPE_TEXTURE_2D, 0, PIPE_BIND_DEPTH_STENCIL))
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
    templ.array_size = 1;
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
        pipe_resource_reference(&amfb->render_resource, NULL);

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
                pipe_resource_reference(&amfb->render_resource, amfb->textures[ST_ATTACHMENT_FRONT_LEFT]);
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

VOID AROSMesaFreeFrameBuffer(struct arosmesa_framebuffer * framebuffer)
{
    if (framebuffer)
    {
        LONG i;

        pipe_resource_reference(&framebuffer->render_resource, NULL);

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

VOID AROSMesaFreeStManager(struct st_manager * stmanager)
{
    if (stmanager)
    {
        if (stmanager->screen)
            DestroyPipeScreen(stmanager->screen);
        FreeVec(stmanager);
    }
}
