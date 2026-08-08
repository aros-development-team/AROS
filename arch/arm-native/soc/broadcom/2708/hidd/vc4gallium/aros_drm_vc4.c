/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    vc4 driver glue for the mesa3dgl-side gallium path: screen creation,
    the display/present path (which dereferences vc4_resource / vc4_bo /
    vc4_screen, so it must live here on the Mesa side, not in the
    Mesa-type-free hidd), the zero-copy fullscreen scanout, and the
    vc4/v3d linker stubs the statically linked Mesa driver references.

    The generic POSIX/libdrm shims and the shared aros_drm_bridge pointer
    live in aros_drm_shim.c. A future v3d driver gets aros_drm_v3d.c.
*/

#define DEBUG 0
#include <aros/debug.h>
/* aros/debug.h leaves DEBUG defined, which trips the `#if defined(DEBUG)`
 * paths in Mesa's u_debug_refcnt.h / u_inlines.h — emitting refs to
 * debug_refcnt_state etc. that only exist in mesa3dgl's copy of mesautil.
 * NDEBUG keeps Mesa's assert() from referencing _debug_assert_fail
 * directly (the driver archive's asserts go through the trampolines). */
#undef DEBUG
#define NDEBUG 1

#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dosextens.h>
#include <proto/oop.h>
#include <proto/layers.h>
#include <proto/alib.h>
#include <proto/graphics.h>
#include <proto/icon.h>
#include <workbench/workbench.h>

#include <string.h>

#include <graphics/rastport.h>
#include <graphics/clip.h>
#include <graphics/layers.h>
#include <hidd/gallium.h>
#include <hidd/gfx.h>

#include "pipe/p_state.h"
#include "vc4_resource.h"
#include "vc4_bufmgr.h"
#include "vc4_screen.h"
#include "renderonly/renderonly.h"

#include "vc4_aros_bridge.h"
#include "gallium_core_api.h"

/* Shared dispatch handle, defined in aros_drm_shim.c. */
extern struct vc4_aros_bridge *aros_drm_bridge;

/* The Mesa vc4 driver's screen entry, statically linked into mesa3dgl. */
struct renderonly;
extern struct pipe_screen *vc4_screen_create(int fd, struct renderonly *ro);

/* renderonly_* stubs to satisfy the linker (renderonly.c isn't in our
 * linklib). Never called at runtime — we pass NULL renderonly to
 * vc4_screen_create. The static-inline helpers in renderonly.h need no
 * stub. */
struct renderonly *renderonly_dup(const struct renderonly *ro)
{
    return NULL;
}

void renderonly_scanout_destroy(struct renderonly_scanout *scanout,
                                struct renderonly *ro)
{
}

struct renderonly_scanout *renderonly_create_gpu_import_for_resource(
    struct pipe_resource *rsc, struct renderonly *ro,
    struct winsys_handle *out_handle)
{
    return NULL;
}

/*
 * V3D CL dump / debug stubs. Used by vc4_cl_dump.c for debug output;
 * always no-ops in release builds.
 */
void *clif_dump_init(unsigned int devinfo, void *out, int v3d_ver)
{
    return (void *)0;
}

void clif_dump_destroy(void *clif)
{
}

void *v3d_spec_load(void *devinfo)
{
    return (void *)0;
}

void *v3d_spec_find_instruction(void *spec, const void *p, const char *name)
{
    return (void *)0;
}

unsigned int v3d_group_get_length(void *group)
{
    return 0;
}

const char *v3d_group_get_name(void *group)
{
    return "";
}

void v3d_print_group(void *clif, void *group, int offset,
                     const void *p, const char *name)
{
}

/*
 * Zero-copy fullscreen scanout state. When the GL surface covers the
 * whole flippable framebuffer, the render resource's BO is rebound to
 * the back page: the GPU renders straight into scanout and presenting
 * is a page flip, not a 5-8 MB DMA copy (~37 MB/s through the uncached
 * alias, which dominated frame time). Single context, like the bridge.
 */
static struct {
    struct pipe_resource *rsc;      /* resource bound to a page (identity only) */
    uint32_t        page_handle;    /* hidd BO handle of the bound page */
    uint32_t        name[2];        /* page GEM names (phys addrs) */
} aros_scanout;

static void aros_scanout_forget(void)
{
    aros_scanout.rsc = NULL;
    aros_scanout.page_handle = 0;
    aros_scanout.name[0] = aros_scanout.name[1] = 0;
}

/* Bind the current back page's BO into the render resource, so the next
 * frame is rendered directly into it. Returns TRUE on success. */
static BOOL aros_scanout_bind_back(struct vc4_resource *rsc,
                                   OOP_Object *bm_obj)
{
    struct vc4_aros_scanout so;
    struct vc4_screen *vscreen = vc4_screen(rsc->base.screen);
    struct vc4_bo *nb;

    if (aros_drm_bridge->get_scanout(aros_drm_bridge->ctx, bm_obj, &so) != 0)
        return FALSE;

    nb = vc4_bo_open_name(vscreen, so.name[so.back]);
    if (!nb)
        return FALSE;

    vc4_bo_unreference(&rsc->bo);
    rsc->bo = nb;
    rsc->slices[0].offset = 0;

    aros_scanout.rsc = &rsc->base;
    aros_scanout.page_handle = nb->handle;
    aros_scanout.name[0] = so.name[0];
    aros_scanout.name[1] = so.name[1];
    return TRUE;
}

/* Detach from the page: give the resource a private BO again so windowed
 * rendering can't scribble into the framebuffer pages. */
static void aros_scanout_unbind(struct vc4_resource *rsc)
{
    struct vc4_screen *vscreen = vc4_screen(rsc->base.screen);
    struct vc4_bo *nb;

    nb = vc4_bo_alloc(vscreen, rsc->slices[0].size, "scanout-exit");
    if (nb)
    {
        vc4_bo_unreference(&rsc->bo);
        rsc->bo = nb;
        rsc->slices[0].offset = 0;
    }
    aros_scanout_forget();
}

/*
 * Zero-copy windowed present state: when the GL window is a single
 * unobscured cliprect, the rendered BO is shown as an HVS overlay
 * plane — the windowed analogue of the fullscreen scanout path above.
 *
 * The present is PIPELINED over a ring of three private pages: each
 * present displays the frame rendered the present BEFORE (whose jobs
 * finished while this frame was being built, so the seqno wait is
 * normally instant) and queues the just-rendered frame for the next
 * present. Rotation: render target -> queued -> on plane -> render
 * target. This overlaps CPU emit with GPU render — frame time becomes
 * max(CPU, GPU) instead of CPU + GPU (the hidd's set_overlay used to
 * wait for full GPU idle, which cost tens of ms per frame in a
 * GPU-bound scene) — at the cost of one frame of display latency.
 */
static struct {
    struct pipe_resource *rsc;
    uint32_t        page_handle;    /* BO bound in rsc (render target) */
    struct vc4_bo  *queued;         /* last-rendered frame; shown next present */
    uint32_t        queued_seqno;   /* submit seqno covering `queued` */
    struct vc4_bo  *onplane;        /* page the overlay currently scans */
    struct vc4_bo  *freep;          /* 3rd page, parked until the ring fills */
    BOOL            shown;          /* plane currently on scanout */
    struct pipe_resource *refused;  /* set_overlay said no for this
                                     * resource (e.g. scaled desktop):
                                     * don't retry every present */
} aros_ovl;

/* Hide the plane but KEEP the page pair: obscure/reveal cycles (another
 * window dragged across, menus opening over the GL window) must not
 * allocate or free BOs — a bo_alloc during the menu-close transition
 * deadlocked against Intuition's own bitmap traffic while we held
 * LockLayerRom. Resuming is just the steady-state present succeeding
 * again. */
/* Render-scale divisor from ENV:VC4_RENDER_SCALE / icon tooltype: the
 * drawable renders at 1/N and presents show it N x through the HVS
 * scaler. dest = src * N stays within the window because the drawable
 * was computed as floor(window/N). This module reads the same sources on
 * the same process as mesa3dgl_support.c does when sizing the drawable,
 * so the two module-local values always agree. */
static ULONG mesa3dgl_render_scale = 1;

/* Per-program override: VC4_RENDER_SCALE tooltype in the program's icon.
 * Mirror of MESA3DGLReadScaleToolType (mesa3dgl_support.c) — the value
 * must be read on both sides of the module split. */
static BOOL vc4_read_scale_tooltype(ULONG *scale)
{
    struct Library *IconBase;
    struct Process *me = (struct Process *)FindTask(NULL);
    struct DiskObject *dob;
    BPTR progdir, olddir;
    TEXT name[108];
    STRPTR val;
    BOOL found = FALSE;

    if (me->pr_Task.tc_Node.ln_Type != NT_PROCESS)
        return FALSE;

    progdir = GetProgramDir();
    if (progdir == BNULL)
        return FALSE;

    if (GetProgramName(name, sizeof(name)) == DOSFALSE || name[0] == '\0')
    {
        /* WB-launched: no CLI name set, the process carries the tool name */
        name[sizeof(name) - 1] = '\0';
        strncpy(name, me->pr_Task.tc_Node.ln_Name, sizeof(name) - 1);
    }

    IconBase = OpenLibrary((STRPTR)"icon.library", 0);
    if (!IconBase)
        return FALSE;

    olddir = CurrentDir(progdir);
    dob = GetDiskObject(FilePart(name));
    if (dob)
    {
        val = FindToolType(dob->do_ToolTypes, "VC4_RENDER_SCALE");
        if (val && val[0] >= '1' && val[0] <= '4' && val[1] == '\0')
        {
            *scale = val[0] - '0';
            found = TRUE;
        }
        FreeDiskObject(dob);
    }
    CurrentDir(olddir);
    CloseLibrary(IconBase);

    return found;
}

static void vc4_read_render_scale(void)
{
    TEXT buf[8];

    mesa3dgl_render_scale = 1;

    if (GetVar((STRPTR)"VC4_RENDER_SCALE", buf, sizeof(buf), 0) > 0
        && buf[0] >= '1' && buf[0] <= '4')
        mesa3dgl_render_scale = buf[0] - '0';

    /* Icon tooltype (per program) overrides the env variable (global) */
    vc4_read_scale_tooltype(&mesa3dgl_render_scale);
}

static void aros_ovl_suspend(void)
{
    if (aros_drm_bridge && aros_ovl.shown)
        aros_drm_bridge->clear_overlay(aros_drm_bridge->ctx, NULL);
    aros_ovl.shown = FALSE;
}

/* Full teardown: resource changed or the screen is going away. */
static void aros_ovl_exit(void)
{
    aros_ovl_suspend();
    if (aros_ovl.queued)
        vc4_bo_unreference(&aros_ovl.queued);
    if (aros_ovl.onplane)
        vc4_bo_unreference(&aros_ovl.onplane);
    if (aros_ovl.freep)
        vc4_bo_unreference(&aros_ovl.freep);
    aros_ovl.queued_seqno = 0;
    aros_ovl.rsc = NULL;
    aros_ovl.page_handle = 0;
}

/*
 * Display a rendered pipe_resource to a RastPort via the bridge,
 * replacing the gallium.library BltPipeResourceRastPort path for vc4.
 * The pipe_resource is dereferenced here (mesa3dgl-side, where Mesa
 * types live) into BO handle/stride/offset/cpp; bridge->display_blit
 * does the per-cliprect blit hidd-side. Fullscreen surfaces use the
 * zero-copy path above (steady-state presents are a page flip).
 */
BOOL aros_drm_blit_resource(struct pipe_resource *src_pres,
                            LONG xSrc, LONG ySrc,
                            struct RastPort *destRP,
                            LONG xDest, LONG yDest,
                            LONG xSize, LONG ySize)
{
    struct vc4_resource *rsc;
    struct Layer *L;
    struct Rectangle renderableLayerRect, result;
    struct ClipRect *CR;
    BOOL copied = FALSE;
    BOOL fullscreen, bound, enter_scanout = FALSE, detach_after_blit = FALSE;
    OOP_Object *scr_bm_obj;
    ULONG bo_handle, stride, offset, cpp;

    /* No vc4 bridge (e.g. softpipe in QEMU — no V3D, so no vc4 screen was
     * created): decline so the caller falls back to BltPipeResourceRastPort.
     * Must come before the vc4_resource cast: src_pres is a softpipe_resource
     * here, not ours. */
    if (!aros_drm_bridge)
        return FALSE;
    if (!src_pres || !destRP)
        return TRUE;

    rsc = (struct vc4_resource *)src_pres;
    if (!rsc->bo)
        return TRUE;

    bo_handle = rsc->bo->handle;
    stride    = rsc->slices[0].stride;
    offset    = rsc->slices[0].offset;
    cpp       = rsc->cpp;

    /* Clamp the blit to the resource's actual allocation. During a window
     * resize glASwapBuffers passes the new framebuffer size while
     * render_resource is still last frame's allocation (the state tracker
     * recreates it on the next draw) — unclamped, the hidd rejects the
     * whole blit as out of range. req_w/req_h keep the caller's original
     * request for the flip-geometry check below. */
    LONG req_w = xSize, req_h = ySize;
    if (xSrc >= (LONG)src_pres->width0 || ySrc >= (LONG)src_pres->height0)
        return TRUE;
    if (xSize > (LONG)src_pres->width0 - xSrc)
        xSize = (LONG)src_pres->width0 - xSrc;
    if (ySize > (LONG)src_pres->height0 - ySrc)
        ySize = (LONG)src_pres->height0 - ySrc;

    if (!(L = destRP->Layer))
        return TRUE;
    if (!IsLayerVisible(L))
        return TRUE;

    /* Drop stale scanout state if the bound resource went away (e.g.
     * the framebuffer was resized and recreated). Pointer compare only;
     * the page BO itself was released with the old resource. */
    if (aros_scanout.rsc && aros_scanout.rsc != src_pres)
        aros_scanout_forget();
    if (aros_ovl.rsc && aros_ovl.rsc != src_pres)
        aros_ovl_exit();
    if (aros_ovl.refused && aros_ovl.refused != src_pres)
        aros_ovl.refused = NULL;

    LockLayerRom(L);

    scr_bm_obj = HIDD_BM_OBJ(destRP->BitMap);

    /* Fullscreen = a single unobscured cliprect, surface at 0,0, no
     * source offset. Geometry against the framebuffer is checked by
     * get_scanout below. */
    fullscreen = (xSrc == 0 && ySrc == 0 && xDest == 0 && yDest == 0 &&
                  L->bounds.MinX == 0 && L->bounds.MinY == 0 &&
                  L->ClipRect && !L->ClipRect->Next && !L->ClipRect->lobs);

    bound = (aros_scanout.rsc == src_pres &&
             rsc->bo->handle == aros_scanout.page_handle);

    if (fullscreen && cpp == 4 && offset == 0 &&
        aros_drm_bridge->version >= 2)
    {
        struct vc4_aros_scanout so;

        if (aros_drm_bridge->get_scanout(aros_drm_bridge->ctx,
                                         scr_bm_obj, &so) == 0 &&
            (ULONG)xSize == so.width && (ULONG)ySize == so.height &&
            stride == so.pitch && (so.width & 15) == 0)
        {
            if (bound &&
                so.name[0] == aros_scanout.name[0] &&
                so.name[1] == aros_scanout.name[1])
            {
                /* Steady state: the frame is already in the back page.
                 * Make it visible and aim the resource at the new back
                 * page. No pixels move. */
                if (aros_drm_bridge->flip_scanout(aros_drm_bridge->ctx,
                                                  scr_bm_obj) == 0 &&
                    aros_scanout_bind_back(rsc, scr_bm_obj))
                {
                    UnlockLayerRom(L);
                    return TRUE;
                }
                /* Flip refused (e.g. flipping got disabled): this
                 * frame's pixels live in the page rsc->bo still points
                 * at, so blit them out below, then detach. */
                aros_scanout_forget();
                detach_after_blit = TRUE;
            }
            else
            {
                /* Enter scanout mode after this frame's blit+flip. */
                enter_scanout = TRUE;
            }
        }
        else if (bound)
        {
            /* Geometry changed under us — blit this frame from its
             * page, then detach. */
            detach_after_blit = TRUE;
        }
    }
    else if (bound)
    {
        /* No longer fullscreen (window mode, obscured, moved): blit this
         * frame from the page it was rendered into, then detach so the
         * next frame uses a private BO. */
        detach_after_blit = TRUE;
    }

    /* Zero-copy windowed present: a fully visible window (single
     * unobscured cliprect) shows the rendered BO as an HVS overlay
     * plane. Falls back to the blit below whenever the conditions
     * lapse (obscured, resized, scaled desktop, no takeover). With a
     * render-scale divisor active, fullscreen surfaces also present
     * through the (scaled) overlay — the drawable no longer matches
     * the framebuffer pages, so the flip path can't serve them. */
    if ((!fullscreen || mesa3dgl_render_scale > 1) && cpp == 4 && offset == 0
        && aros_drm_bridge->version >= 5)
    {
        BOOL windowed = (xSrc == 0 && ySrc == 0 &&
                         xSize == (LONG)src_pres->width0 &&
                         ySize == (LONG)src_pres->height0 &&
                         req_w == xSize && req_h == ySize &&
                         L->ClipRect && !L->ClipRect->Next &&
                         !L->ClipRect->lobs);
        LONG absX = L->bounds.MinX + xDest;
        LONG absY = L->bounds.MinY + yDest;

        if (aros_ovl.rsc == src_pres)
        {
            if (windowed && rsc->bo->handle == aros_ovl.page_handle)
            {
                struct vc4_screen *vscreen = vc4_screen(rsc->base.screen);

                if (!aros_ovl.queued)
                {
                    /* Ring not full yet (2nd present after entry): keep
                     * showing the entry frame, queue this one, render
                     * the next into the parked page. */
                    aros_ovl.queued = rsc->bo;
                    aros_ovl.queued_seqno =
                        aros_drm_bridge->get_seqno(aros_drm_bridge->ctx);
                    rsc->bo = aros_ovl.freep;
                    aros_ovl.freep = NULL;
                    rsc->slices[0].offset = 0;
                    aros_ovl.page_handle = rsc->bo->handle;
                    UnlockLayerRom(L);
                    return TRUE;
                }

                /* Display the PREVIOUS frame; its jobs ran while this
                 * frame was being built, so this wait is normally a
                 * no-op. The just-rendered frame goes into the queue. */
                vc4_wait_seqno(vscreen, aros_ovl.queued_seqno,
                               PIPE_TIMEOUT_INFINITE, "ovl-present");

                if (aros_drm_bridge->set_overlay(aros_drm_bridge->ctx,
                        scr_bm_obj, aros_ovl.queued->handle, stride,
                        absX, absY, xSize, ySize,
                        xSize * mesa3dgl_render_scale,
                        ySize * mesa3dgl_render_scale) == 0)
                {
                    /* Rotate: queued -> on plane; the page leaving the
                     * plane becomes the next render target (the GPU
                     * won't write it before the next submit, long after
                     * the vblank latch). */
                    struct vc4_bo *freed = aros_ovl.onplane;

                    aros_ovl.shown = TRUE;
                    aros_ovl.onplane = aros_ovl.queued;
                    aros_ovl.queued = rsc->bo;
                    aros_ovl.queued_seqno =
                        aros_drm_bridge->get_seqno(aros_drm_bridge->ctx);
                    rsc->bo = freed;
                    rsc->slices[0].offset = 0;
                    aros_ovl.page_handle = rsc->bo->handle;
                    UnlockLayerRom(L);
                    return TRUE;
                }
                /* The hidd refused a present that was fine before
                 * (e.g. mode changed under us): tear down and don't
                 * retry for this resource. */
                aros_ovl_exit();
                aros_ovl.refused = src_pres;
            }
            else if (!windowed)
            {
                /* Obscured/partially hidden: hide the plane but keep
                 * the page pair — the steady-state branch resumes
                 * without any allocation when the window is free
                 * again. Blit the frame below. */
                aros_ovl_suspend();
            }
            else
            {
                /* Mesa replaced the resource's BO under us — our page
                 * bookkeeping is void. */
                aros_ovl_exit();
            }
        }
        else if (windowed && !aros_ovl.rsc && rsc->slices[0].size
                 && aros_ovl.refused != src_pres)
        {
            struct vc4_screen *vscreen = vc4_screen(rsc->base.screen);
            struct vc4_bo *nb1, *nb2 = NULL;
            int oret = -1;

            D(bug("[vc4ovl] enter: bo=%lu %ldx%ld at %ld,%ld stride=%lu\n",
                (unsigned long)rsc->bo->handle, xSize, ySize,
                absX, absY, (unsigned long)stride));
            nb1 = vc4_bo_alloc(vscreen, rsc->slices[0].size, "overlay-page");
            if (nb1)
                nb2 = vc4_bo_alloc(vscreen, rsc->slices[0].size,
                                   "overlay-page");
            D(bug("[vc4ovl] page alloc: %s\n", nb2 ? "ok" : "FAILED"));

            if (nb2)
            {
                /* The entry present shows the frame just rendered, so
                 * it must wait for it — the only full wait left on this
                 * path (steady state waits on the previous frame). */
                vc4_wait_seqno(vscreen,
                    aros_drm_bridge->get_seqno(aros_drm_bridge->ctx),
                    PIPE_TIMEOUT_INFINITE, "ovl-enter");
                oret = aros_drm_bridge->set_overlay(aros_drm_bridge->ctx,
                    scr_bm_obj, rsc->bo->handle, stride,
                    absX, absY, xSize, ySize,
                    xSize * mesa3dgl_render_scale,
                    ySize * mesa3dgl_render_scale);
            }
            D(bug("[vc4ovl] set_overlay=%d\n", oret));

            if (nb2 && oret == 0)
            {
                aros_ovl.rsc = src_pres;
                aros_ovl.shown = TRUE;
                aros_ovl.onplane = rsc->bo;    /* now on the overlay */
                aros_ovl.queued = NULL;
                aros_ovl.queued_seqno = 0;
                aros_ovl.freep = nb2;
                rsc->bo = nb1;
                rsc->slices[0].offset = 0;
                aros_ovl.page_handle = nb1->handle;
                UnlockLayerRom(L);
                return TRUE;
            }
            if (nb1)
                vc4_bo_unreference(&nb1);
            if (nb2)
                vc4_bo_unreference(&nb2);
            /* Refused (scaled desktop / no takeover): remember and stop
             * paying an alloc+refusal on every present. Cleared when
             * the resource changes (resize/mode switch). */
            aros_ovl.refused = src_pres;
        }
    }

    renderableLayerRect.MinX = L->bounds.MinX + xDest;
    renderableLayerRect.MaxX = L->bounds.MinX + xDest + xSize - 1;
    renderableLayerRect.MinY = L->bounds.MinY + yDest;
    renderableLayerRect.MaxY = L->bounds.MinY + yDest + ySize - 1;
    if (renderableLayerRect.MinX < L->bounds.MinX) renderableLayerRect.MinX = L->bounds.MinX;
    if (renderableLayerRect.MaxX > L->bounds.MaxX) renderableLayerRect.MaxX = L->bounds.MaxX;
    if (renderableLayerRect.MinY < L->bounds.MinY) renderableLayerRect.MinY = L->bounds.MinY;
    if (renderableLayerRect.MaxY > L->bounds.MaxY) renderableLayerRect.MaxY = L->bounds.MaxY;

    CR = L->ClipRect;
    for (; CR; CR = CR->Next)
    {
        if (CR->lobs)
            continue;

        if (AndRectRect(&renderableLayerRect, &CR->bounds, &result))
        {
            LONG cr_srcx = xSrc + result.MinX - L->bounds.MinX - xDest;
            LONG cr_srcy = ySrc + result.MinY - L->bounds.MinY - yDest;
            ULONG cr_w   = result.MaxX - result.MinX + 1;
            ULONG cr_h   = result.MaxY - result.MinY + 1;
            ULONG cr_off = offset + cr_srcy * stride + cr_srcx * cpp;
            OOP_Object *bm_obj = HIDD_BM_OBJ(destRP->BitMap);

            aros_drm_bridge->display_blit(aros_drm_bridge->ctx,
                bo_handle, stride, cr_off, cpp,
                bm_obj,
                result.MinX, result.MinY,
                cr_w, cr_h);

            copied = TRUE;
        }
    }

    if (copied)
    {
        /* Notify the bitmap about the damage. */
        struct pHidd_BitMap_UpdateRect urmsg = {
            .mID    = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_UpdateRect),
            .x      = renderableLayerRect.MinX,
            .y      = renderableLayerRect.MinY,
            .width  = renderableLayerRect.MaxX - renderableLayerRect.MinX + 1,
            .height = renderableLayerRect.MaxY - renderableLayerRect.MinY + 1,
        };
        OOP_DoMethod(HIDD_BM_OBJ(destRP->BitMap), (OOP_Msg)&urmsg);
    }

    /* The full-frame blit above also flipped (inside the hidd), so the
     * back page reported by get_scanout is now the page to render the
     * next frame into. */
    if (enter_scanout)
        aros_scanout_bind_back(rsc, scr_bm_obj);
    else if (detach_after_blit)
        aros_scanout_unbind(rsc);

    UnlockLayerRom(L);
    return TRUE;
}

/*
 * Create a pipe_screen. Called from the hidd's CreatePipeScreen method
 * with the module-internal bridge and mesa3dgl's GalliumCoreAPI table
 * (from aHidd_Gallium_CoreAPI). Returns NULL when the table is missing
 * or from a different Mesa generation; the GL context creation then
 * fails cleanly instead of corrupting.
 * Single screen at a time, matching the in-hidd single-fd model.
 */
/* Private util_cpu_caps copy (u_cpu_detect.o is linked into this module;
 * the provider's caps live in mesa3dgl and are a different object). */
extern void util_cpu_detect(void);

/* Live screens created through the bridge. An app toggling between
 * fullscreen and windowed mode may create the new context before
 * destroying the old one; dropping the bridge pointer on the first
 * destroy would kill presents for the survivor. */
static LONG aros_drm_screens;

struct pipe_screen *vc4_aros_create_screen(struct vc4_aros_bridge *bridge,
                                           APTR coreapi)
{
    struct pipe_screen *pscreen = NULL;

    if (!bridge)
        return NULL;

    /* Bind the driver's Mesa-core trampolines to mesa3dgl's
     * GalliumCoreAPI table before any driver code runs. Idempotent;
     * refusing on version/hash/name mismatch is what makes a driver from
     * a different Mesa generation fail cleanly instead of corrupting. */
    {
        int bres = gca_bind((const struct GalliumCoreAPI *)coreapi);
        if (bres > 0)
        {
            bug("[aros_drm_vc4] GalliumCoreAPI bind refused: slot %d is not"
                " '%s' — rebuild mesa3dgl-library and this hidd together\n",
                bres - 1, gca_slot_name((uint32_t)(bres - 1)));
            return NULL;
        }
        if (bres < 0)
        {
            bug("[aros_drm_vc4] GalliumCoreAPI bind refused (code %d:"
                " -1 version, -2 size, -3 hash, -4 count) — rebuild"
                " mesa3dgl-library and this hidd together\n", bres);
            return NULL;
        }
    }

    /* Fill this module's private util_cpu_caps (has_neon selects the
     * tiling path) — the trampolined core never touches our copy. */
    util_cpu_detect();

    /* Same env/tooltype read mesa3dgl does when sizing the drawable;
     * runs on the app's process, so the values agree. */
    vc4_read_render_scale();

    aros_drm_bridge = bridge;

    /* New-app reclaim: an application may exit via exit() without any GL
     * teardown, so glADestroyContext (aros_drm_release_bridge) never runs
     * and the driver-internal pages (overlay/scanout/pools) leak VideoCore
     * memory until the firmware OOMs after a few runs. The aros_drm_screens
     * counter is unreliable for detecting this (it only decrements on clean
     * teardown, so it grows without bound across dirty-exit runs). Instead
     * key on the owning task: a different task means the PREVIOUS app is
     * dead and its leftover BOs are safe to sweep. The same task (one app
     * creating several contexts) does NOT sweep — which also means
     * successive Shell commands, that RunCommand() runs on one task, are
     * not told apart here. */
    {
        struct Task *me = FindTask(NULL);
        static struct Task *aros_bo_owner_task;
        static BOOL aros_bo_owner_valid;

        if (aros_bo_owner_valid && me != aros_bo_owner_task)
        {
            D(bug("[aros_drm_vc4] new run (task %p -> task %p):"
                " reclaiming leaked BOs\n", aros_bo_owner_task, me));
            if (bridge->release_all_bos)
                bridge->release_all_bos(bridge->ctx);
            aros_drm_screens = 0;   /* stale counter from dirty-exit runs */
        }
        aros_bo_owner_task = me;
        aros_bo_owner_valid = TRUE;
    }

    /* Dispatch on driver_id (one mesa3dgl can carry several drivers, one
     * per Pi GPU gen). Only VC4 today; V3D slots into the commented case.
     * fd is any positive value (dispatch goes through the bridge, not fd
     * lookup); NULL renderonly since we own the display path. */
    switch (bridge->driver_id)
    {
    case AROS_GALLIUM_DRIVER_VC4:
        pscreen = vc4_screen_create(1, NULL);
        if (!pscreen)
            bug("[aros_drm_vc4] vc4_screen_create FAILED (bridge ok, falling back)\n");
        break;

    /* case AROS_GALLIUM_DRIVER_V3D:
     *     pscreen = v3d_screen_create(1, NULL);
     *     break; */

    default:
        bug("[aros_drm_vc4] unsupported gallium driver_id %u\n",
            (unsigned int)bridge->driver_id);
        return NULL;
    }

    if (pscreen)
        aros_drm_screens++;
    return pscreen;
}

/* Mesa-typed shim for the hidd's DisplayResourceRP method, so the OOP
 * class file stays free of pipe_resource. TRUE = presented; FALSE = the
 * caller falls back to the per-cliprect DisplayResource loop. */
IPTR vc4_aros_display_rp(APTR resource, LONG srcx, LONG srcy,
                         struct RastPort *rp, LONG dstx, LONG dsty,
                         LONG width, LONG height)
{
    return aros_drm_blit_resource((struct pipe_resource *)resource,
                                  srcx, srcy, rp, dstx, dsty,
                                  width, height) ? TRUE : FALSE;
}

/* Per-screen teardown bookkeeping, called from DestroyPipeScreen after
 * the screen itself is gone. Returns TRUE when this was the LAST live
 * screen, which is the caller's cue to put the hardware back in its
 * idle, empty state. An app may hold two screens at once - a toolkit
 * probing GL capabilities keeps its probe context while building the
 * real one, and switching to fullscreen typically creates the new
 * context before dropping the old - so that reset must not run on the
 * first of them: it would free the survivor's BOs and clear V3D state
 * mid-render. */
BOOL aros_drm_release_bridge(void)
{
    /* Just drop our in-lib bookkeeping pointers — the underlying firmware
     * BOs are freed by the hidd sweep below, not here (unreferencing the
     * vc4_bo structs would touch the just-freed in-lib screen). */
    aros_ovl.queued = NULL;
    aros_ovl.onplane = NULL;
    aros_ovl.freep = NULL;
    aros_ovl.queued_seqno = 0;
    aros_ovl.shown = FALSE;
    aros_ovl.rsc = NULL;
    aros_ovl.page_handle = 0;

    if (aros_drm_screens > 0 && --aros_drm_screens > 0)
        return FALSE;

    /* Last screen gone. The BO sweep itself is the caller's job (it owns
     * the bo_table); here we only drop the pointers into it. */
    aros_scanout_forget();
    aros_drm_bridge = NULL;
    return TRUE;
}
