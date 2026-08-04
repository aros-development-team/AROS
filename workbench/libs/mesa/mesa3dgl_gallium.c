/*
    Copyright (C) 2009-2020, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

/* aros/debug.h always leaves DEBUG defined (to 0 if unset), which trips
 * Mesa's u_debug_refcnt.h — it uses `#if defined(DEBUG)` and so emits
 * refs to debug_reference_slowpath even under -DNDEBUG. Drop the symbol
 * before pulling in the gallium pipe/util headers. */
#undef DEBUG

#include <proto/utility.h>
#include <proto/exec.h>
#include <proto/gallium.h>

#include <stdint.h>
#include <stdbool.h>

#include "util/os_misc.h"
#include "pipe/p_defines.h"
#include "pipe/p_screen.h"
#include "pipe/p_state.h"

#include "util/u_inlines.h"

#include "main/context.h"

#include "mesa3dgl_support.h"
#include "mesa3dgl_gallium.h"

static BOOL MESA3DGLSelectColorFormat(enum pipe_format * colorFormat,
    struct pipe_screen * screen, GLint bpp)
{
    D(bug("[MESA3DGL] %s()\n", __func__));

    *colorFormat = PIPE_FORMAT_NONE;

    if (bpp == 16)
    {
        if (screen->is_format_supported(screen,
            PIPE_FORMAT_B5G6R5_UNORM,
            PIPE_TEXTURE_2D,
            0,
            0,
            PIPE_BIND_RENDER_TARGET))
        {
            *colorFormat = PIPE_FORMAT_B5G6R5_UNORM;
            return TRUE;
        }
    }
    
    if (bpp == 32)
    {
        if (screen->is_format_supported(screen,
            PIPE_FORMAT_B8G8R8A8_UNORM,
            PIPE_TEXTURE_2D,
            0,
            0,
            PIPE_BIND_RENDER_TARGET))
        {
            *colorFormat = PIPE_FORMAT_B8G8R8A8_UNORM;
            return TRUE;
        }
    }
    
    return FALSE;
}

static BOOL MESA3DGLSelectDepthStencilFormat(enum pipe_format * depthStencilFormat,
    struct pipe_screen * screen, BOOL noDepth, BOOL noStencil)
{
    D(bug("[MESA3DGL] %s()\n", __func__));

    *depthStencilFormat = PIPE_FORMAT_NONE;
    
    if (noDepth)
        return TRUE;
    
    if(!noStencil && (screen->is_format_supported(screen,
            PIPE_FORMAT_S8_UINT_Z24_UNORM,
            PIPE_TEXTURE_2D,
            0,
            0,
            PIPE_BIND_DEPTH_STENCIL)))
    {
        *depthStencilFormat  = PIPE_FORMAT_S8_UINT_Z24_UNORM;
        return TRUE;
    }
    
    if(noStencil && (screen->is_format_supported(screen,
            PIPE_FORMAT_X8Z24_UNORM,
            PIPE_TEXTURE_2D,
            0,
            0,
            PIPE_BIND_DEPTH_STENCIL)))
    {
        *depthStencilFormat  = PIPE_FORMAT_X8Z24_UNORM;
        return TRUE;
    }

    if(noStencil && (screen->is_format_supported(screen,
            PIPE_FORMAT_Z24X8_UNORM,
            PIPE_TEXTURE_2D,
            0,
            0,
            PIPE_BIND_DEPTH_STENCIL)))
    {
        *depthStencilFormat  = PIPE_FORMAT_Z24X8_UNORM;
        return TRUE;
    }
    
    if(screen->is_format_supported(screen,
            PIPE_FORMAT_Z16_UNORM,
            PIPE_TEXTURE_2D,
            0,
            0,
            PIPE_BIND_DEPTH_STENCIL))
    {
        *depthStencilFormat = PIPE_FORMAT_Z16_UNORM;
        return TRUE;
    }
    
    return FALSE;
}

BOOL MESA3DGLFillVisual(struct st_visual * stvis, struct pipe_screen * screen, int bpp, struct TagItem *tagList)
{
    BOOL noDepth, noStencil, noAccum;
    
    D(bug("[MESA3DGL] %s()\n", __func__));
    
    noStencil   = GetTagData(GLA_NoStencil, GL_FALSE, tagList);
    noAccum     = GetTagData(GLA_NoAccum, GL_FALSE, tagList);
    noDepth     = GetTagData(GLA_NoDepth, GL_FALSE, tagList);

    stvis->color_format = PIPE_FORMAT_NONE;
    stvis->depth_stencil_format = PIPE_FORMAT_NONE;
    stvis->accum_format = PIPE_FORMAT_NONE;
    stvis->buffer_mask = ST_ATTACHMENT_FRONT_LEFT_MASK;
    stvis->render_buffer = ST_ATTACHMENT_FRONT_LEFT;
    stvis->samples = 1;

    if (!MESA3DGLSelectColorFormat(&stvis->color_format, screen, bpp))
    {
        D(bug("[MESA3DGL] %s: ERROR - No supported color format found\n", __func__));
        return FALSE;
    }

    if (!MESA3DGLSelectDepthStencilFormat(&stvis->depth_stencil_format, screen, noDepth, noStencil))
    {
        D(bug("[MESA3DGL] %s: ERROR - No supported depth/stencil format found\n", __func__));
        return FALSE;
    }

    if (noAccum)
        stvis->accum_format = PIPE_FORMAT_NONE;
    else
    {
        stvis->accum_format = PIPE_FORMAT_R16G16B16A16_SNORM;
        stvis->buffer_mask |= ST_ATTACHMENT_ACCUM;
    }

    /* MESA3DGL uses front buffer as back buffer */
    if (!noDepth || !noStencil)
    stvis->buffer_mask |= ST_ATTACHMENT_DEPTH_STENCIL_MASK;
    
    return TRUE;
}

static VOID MESA3DGLFrameBufferCreateResource(struct mesa3dgl_framebuffer * amfb,
    const enum st_attachment_type statt)
{
    struct pipe_resource templ;

    D(bug("[MESA3DGL] %s()\n", __func__));

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
        /* LINEAR + SCANOUT keep the swap target linear instead of the
         * GPU's native T-tile: the AROS present path (DisplayResource ->
         * DMA blit) needs a linear source and can't detile. Drivers that
         * ignore the flags (or default to linear) are unaffected. */
        templ.bind      = PIPE_BIND_RENDER_TARGET
                        | PIPE_BIND_SAMPLER_VIEW
                        | PIPE_BIND_LINEAR
                        | PIPE_BIND_SCANOUT;
        break;
    case ST_ATTACHMENT_DEPTH_STENCIL:
        templ.format    = amfb->stvis.depth_stencil_format;
        templ.bind      = PIPE_BIND_DEPTH_STENCIL;
        break;
    default:
        return;
    }
    
    amfb->textures[statt] = amfb->screen->resource_create(amfb->screen, &templ);
}

static bool MESA3DGLFrameBufferValidate(struct st_context_iface *stctx,
                       struct st_framebuffer_iface *stfbi,
                       const enum st_attachment_type *statts,
                       unsigned count,
                       struct pipe_resource **out)
{
    struct mesa3dgl_framebuffer * amfb = (struct mesa3dgl_framebuffer *)stfbi;
    LONG i;

    D(bug("[MESA3DGL] %s()\n", __func__));

    if (amfb->resized)
    {
        amfb->resized = FALSE;
        pipe_resource_reference(&amfb->render_resource, NULL);

        for (i = 0; i < ST_ATTACHMENT_COUNT; i++)
            pipe_resource_reference(&amfb->textures[i], NULL);
    }
    
    for (i = 0; i < count; i++)
    {
        if (amfb->textures[statts[i]] == NULL)
        {
            MESA3DGLFrameBufferCreateResource(amfb, statts[i]);
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

static bool MESA3DGLFrameBufferFlushFront(struct st_context_iface *stctx,
                          struct st_framebuffer_iface *stfbi,
                          enum st_attachment_type statt)
{
    D(bug("[MESA3DGL] %s()\n", __func__));

    return TRUE;
}

struct mesa3dgl_framebuffer * MESA3DGLNewFrameBuffer(struct mesa3dgl_context * ctx, struct st_visual * stvis)
{
    D(bug("[MESA3DGL] %s()\n", __func__));

    struct mesa3dgl_framebuffer * framebuffer =
        AllocVec(sizeof(struct mesa3dgl_framebuffer), MEMF_PUBLIC | MEMF_CLEAR);

    if (!framebuffer)
        return NULL;

    CopyMem(stvis, &framebuffer->stvis, sizeof(struct st_visual));

    framebuffer->base.visual = &framebuffer->stvis;
    framebuffer->base.flush_front = MESA3DGLFrameBufferFlushFront;
    framebuffer->base.validate = MESA3DGLFrameBufferValidate;
    framebuffer->base.state_manager = ctx->stmanager;
#if (0)
    framebuffer->base.stamp = 1;
    framebuffer->base.ID = 1; // p_atomic_inc_return(&osmesa_fb_ID);
#endif
    framebuffer->screen = ctx->stmanager->screen;

    return framebuffer;
}

VOID MESA3DGLFreeFrameBuffer(struct mesa3dgl_framebuffer * framebuffer)
{
    D(bug("[MESA3DGL] %s()\n", __func__));

    if (framebuffer)
    {
        LONG i;

        pipe_resource_reference(&framebuffer->render_resource, NULL);

        for (i = 0; i < ST_ATTACHMENT_COUNT; i++)
            pipe_resource_reference(&framebuffer->textures[i], NULL);

        FreeVec(framebuffer);
    }
}

VOID MESA3DGLCheckAndUpdateBufferSize(struct mesa3dgl_context * ctx)
{
    D(bug("[MESA3DGL] %s()\n", __func__));

    MESA3DGLRecalculateBufferWidthHeight(ctx);
    if (ctx->framebuffer->resized)
    {
        /* The state tracker revalidates the framebuffer only when the
         * stamp DIFFERS from the one it cached at the last validate.
         * Setting it to a constant made every resize after the first
         * invisible — increment instead. */
        p_atomic_inc(&ctx->framebuffer->base.stamp);
    }
}

static int MESA3DGLStManagerGetParam(struct st_manager *smapi,
                enum st_manager_param param)
{
    D(bug("[MESA3DGL] %s()\n", __func__));

    return 0;
}

struct st_manager * MESA3DGLNewStManager(struct pipe_screen * pscreen)
{
    D(bug("[MESA3DGL] %s()\n", __func__));

    struct st_manager * stmanager =
        (struct st_manager *)AllocVec(sizeof(struct st_manager), MEMF_PUBLIC | MEMF_CLEAR);

    if (stmanager)
    {
        stmanager->screen = pscreen;
        stmanager->get_param = MESA3DGLStManagerGetParam;
    }
    
    return stmanager;
}

VOID MESA3DGLFreeStManager(APTR pipe, struct st_manager * stmanager)
{
    D(bug("[MESA3DGL] %s()\n", __func__));

    if (stmanager)
    {
        if (stmanager->screen)
            DestroyPipeScreen(pipe, stmanager->screen);
        FreeVec(stmanager);
    }
}
