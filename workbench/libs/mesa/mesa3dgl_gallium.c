/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

/* aros/debug.h always leaves DEBUG defined (to 0 if unset), which trips
 * Mesa's u_debug_refcnt.h — it uses `#if defined(DEBUG)` and so emits
 * refs to debug_reference_slowpath even under -DNDEBUG. Drop the symbol
 * before pulling in the gallium pipe/util headers. */
#undef DEBUG

#include <proto/utility.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/gallium.h>

#include <stdint.h>
#include <stdbool.h>

#include "util/os_misc.h"
#include "util/os_time.h"
#include "pipe/p_defines.h"
#include "pipe/p_screen.h"
#include "pipe/p_state.h"

#include "util/u_inlines.h"

#include "main/context.h"
#include "state_tracker/st_manager.h"

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

    if(amfb->screen->caps.npot_textures)
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

/*
 * 3D scan-out. Try to give the framebuffer scan-out colour buffers from
 * gallium.library: the window's display shows them itself (a hardware
 * plane when the compositor grants one, a blit through the visible region
 * otherwise) and glASwapBuffers() becomes a flip. Any failure keeps the
 * BltPipeResourceRastPort() path.
 */
static BOOL MESA3DGLScanoutCreate(struct mesa3dgl_framebuffer * amfb)
{
    struct mesa3dgl_context * ctx = amfb->ctx;
    struct TagItem tags[] =
    {
        { SFB_Left,       0 },
        { SFB_Top,        0 },
        { SFB_Width,      0 },
        { SFB_Height,     0 },
        { SFB_Buffers,    0 },
        { SFB_PipeFormat, 0 },
        { SFB_PipeScreen, 0 },
        { TAG_DONE,       0 }
    };

    if (!ctx || !ctx->window || !ctx->driver || amfb->noscanout)
        return FALSE;
    if (amfb->width == 0 || amfb->height == 0)
        return FALSE;
    /* The render-scale path presents through the driver's own scaler */
    if (mesa3dgl_render_scale != 1)
        return FALSE;
    /* Only for the window's own RastPort (the rectangle is layer relative) */
    if (ctx->visible_rp->Layer != ctx->window->WLayer)
        return FALSE;

    tags[0].ti_Data = ctx->left;
    tags[1].ti_Data = ctx->top;
    tags[2].ti_Data = amfb->width;
    tags[3].ti_Data = amfb->height;
    tags[4].ti_Data = amfb->scanout_buffers;
    tags[5].ti_Data = amfb->stvis.color_format;
    tags[6].ti_Data = (IPTR)amfb->screen;

    amfb->scanout = CreateScanoutFrameBuffer(ctx->driver, ctx->window, tags);
    if (!amfb->scanout)
    {
        D(bug("[MESA3DGL] %s: no scan-out framebuffer for window %p (%lux%lu), using the blit path\n",
              __func__, ctx->window, (unsigned long)amfb->width, (unsigned long)amfb->height));
        return FALSE;
    }

    amfb->scanout_window = ctx->window;
    amfb->scanout_left   = ctx->left;
    amfb->scanout_top    = ctx->top;
    amfb->scanout_back   = GetScanoutFrameBufferAttr(amfb->scanout, SFBA_BackBuffer);

    D(bug("[MESA3DGL] scan-out framebuffer %p active for window %p: %lux%lu at %ld,%ld, %lu buffers\n",
        amfb->scanout, ctx->window, (unsigned long)amfb->width, (unsigned long)amfb->height,
        (long)ctx->left, (long)ctx->top,
        (unsigned long)GetScanoutFrameBufferAttr(amfb->scanout, SFBA_Buffers)));

    return TRUE;
}

/* The scan-out buffers are display memory, gone with the framebuffer: no
 * job may still render into them by then. */
VOID MESA3DGLScanoutIdle(struct mesa3dgl_framebuffer * amfb, struct st_context * st)
{
    struct pipe_fence_handle * fence = NULL;

    if (!amfb->scanout || !st || !st->pipe)
        return;

    st->pipe->flush(st->pipe, &fence, 0);
    if (fence)
    {
        amfb->screen->fence_finish(amfb->screen, NULL, fence, OS_TIMEOUT_INFINITE);
        amfb->screen->fence_reference(amfb->screen, &fence, NULL);
    }
}

static VOID MESA3DGLScanoutDestroy(struct mesa3dgl_framebuffer * amfb)
{
    if (amfb->scanout)
    {
        D(bug("[MESA3DGL] %s: scan-out framebuffer %p\n", __func__, amfb->scanout));
        DestroyScanoutFrameBuffer(amfb->scanout);
        amfb->scanout = NULL;
        amfb->scanout_window = NULL;
    }
}

/* Point the colour attachment at scan-out buffer `buffer` */
static VOID MESA3DGLScanoutSelect(struct mesa3dgl_framebuffer * amfb, ULONG buffer)
{
    struct pipe_resource * res = GetScanoutFrameBufferResource(amfb->scanout, buffer);

    amfb->scanout_back = buffer;
    pipe_resource_reference(&amfb->textures[ST_ATTACHMENT_FRONT_LEFT], res);
    pipe_resource_reference(&amfb->render_resource, res);
}

/*
 * glASwapBuffers() on a scan-out framebuffer: the caller flushed; flip the
 * rendered buffer and make the next one the render target. The state
 * tracker picks the new attachment up at its next framebuffer validate
 * (the drawable stamp changes and the drawables are invalidated).
 */
VOID MESA3DGLScanoutSwap(struct mesa3dgl_context * ctx)
{
    struct mesa3dgl_framebuffer * amfb = ctx->framebuffer;
    ULONG next;

    if (!amfb->scanout)
        return;

    next = SwapScanoutFrameBuffer(amfb->scanout);
    if (next != amfb->scanout_back)
    {
        MESA3DGLScanoutSelect(amfb, next);
        p_atomic_inc(&amfb->base.stamp);
        if (ctx->st && ctx->st->ctx)
            st_manager_invalidate_drawables(ctx->st->ctx);
    }
}

static bool MESA3DGLFrameBufferValidate(struct st_context *stctx,
                       struct pipe_frontend_drawable *stfbi,
                       const enum st_attachment_type *statts,
                       unsigned count,
                       struct pipe_resource **out,
                       struct pipe_resource **resolve)
{
    struct mesa3dgl_framebuffer * amfb = (struct mesa3dgl_framebuffer *)stfbi;
    LONG i;

    D(bug("[MESA3DGL] %s()\n", __func__));

    if (amfb->resized)
    {
        amfb->resized = FALSE;
        MESA3DGLScanoutIdle(amfb, stctx);
        pipe_resource_reference(&amfb->render_resource, NULL);

        for (i = 0; i < ST_ATTACHMENT_COUNT; i++)
            pipe_resource_reference(&amfb->textures[i], NULL);

        /* A new size (or window / rectangle): new scan-out framebuffer */
        MESA3DGLScanoutDestroy(amfb);
        amfb->scanout_tried = FALSE;
    }
    
    for (i = 0; i < count; i++)
    {
        if (amfb->textures[statts[i]] == NULL)
        {
            if (statts[i] == ST_ATTACHMENT_FRONT_LEFT)
            {
                if (!amfb->scanout && !amfb->scanout_tried)
                {
                    amfb->scanout_tried = TRUE;
                    MESA3DGLScanoutCreate(amfb);
                }
                if (amfb->scanout)
                {
                    MESA3DGLScanoutSelect(amfb, amfb->scanout_back);
                    continue;
                }
            }

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

static bool MESA3DGLFrameBufferFlushFront(struct st_context *stctx,
                          struct pipe_frontend_drawable *stfbi,
                          enum st_attachment_type statt)
{
    D(bug("[MESA3DGL] %s()\n", __func__));

    return TRUE;
}

static uint32_t mesa3dgl_fb_ID;

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
    framebuffer->base.fscreen = ctx->stmanager;
    /* the state tracker keys its per-drawable bookkeeping on ID */
    framebuffer->base.ID = p_atomic_inc_return(&mesa3dgl_fb_ID);
    framebuffer->base.stamp = 1;
    framebuffer->screen = ctx->stmanager->screen;
    framebuffer->ctx = ctx;

    /* 3D scan-out switches: ENV:MESA3DGL_NOSCANOUT (any value) keeps the
     * blit path; ENV:MESA3DGL_SCANOUT_BUFFERS 2 or 3 (default 2: swaps
     * wait for the flip, so a buffer is never rendered while scanned out;
     * 3 relies on the driver's latch bookkeeping, still to be verified). */
    {
        TEXT buf[8];

        framebuffer->scanout_buffers = 2;
        if (GetVar("MESA3DGL_NOSCANOUT", buf, sizeof(buf), 0) > 0)
            framebuffer->noscanout = TRUE;
        if (GetVar("MESA3DGL_SCANOUT_BUFFERS", buf, sizeof(buf), 0) > 0
            && buf[0] >= '2' && buf[0] <= '3')
            framebuffer->scanout_buffers = buf[0] - '0';
    }

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

        MESA3DGLScanoutDestroy(framebuffer);

        FreeVec(framebuffer);
    }
}

VOID MESA3DGLCheckAndUpdateBufferSize(struct mesa3dgl_context * ctx)
{
    D(bug("[MESA3DGL] %s()\n", __func__));

    MESA3DGLRecalculateBufferWidthHeight(ctx);

    /* glASetRast() moved the context to another window or rectangle: the
     * scan-out framebuffer belongs to the old one. */
    if (ctx->framebuffer->scanout &&
        (ctx->framebuffer->scanout_window != ctx->window ||
         ctx->framebuffer->scanout_left != (LONG)ctx->left ||
         ctx->framebuffer->scanout_top != (LONG)ctx->top))
        ctx->framebuffer->resized = TRUE;

    if (ctx->framebuffer->resized)
    {
        /* The state tracker revalidates the framebuffer only when the
         * stamp DIFFERS from the one it cached at the last validate.
         * Setting it to a constant made every resize after the first
         * invisible — increment instead. */
        p_atomic_inc(&ctx->framebuffer->base.stamp);
    }
}

static int MESA3DGLStManagerGetParam(struct pipe_frontend_screen *smapi,
                enum st_manager_param param)
{
    D(bug("[MESA3DGL] %s()\n", __func__));

    return 0;
}

struct pipe_frontend_screen * MESA3DGLNewStManager(struct pipe_screen * pscreen)
{
    D(bug("[MESA3DGL] %s()\n", __func__));

    struct pipe_frontend_screen * stmanager =
        (struct pipe_frontend_screen *)AllocVec(sizeof(struct pipe_frontend_screen), MEMF_PUBLIC | MEMF_CLEAR);

    if (stmanager)
    {
        stmanager->screen = pscreen;
        stmanager->get_param = MESA3DGLStManagerGetParam;
    }
    
    return stmanager;
}

VOID MESA3DGLFreeStManager(APTR pipe, struct pipe_frontend_screen * stmanager)
{
    D(bug("[MESA3DGL] %s()\n", __func__));

    if (stmanager)
    {
        /* drops the state tracker's per-screen data (shader cache etc.) */
        st_screen_destroy(stmanager);
        if (stmanager->screen)
            DestroyPipeScreen(pipe, stmanager->screen);
        FreeVec(stmanager);
    }
}
