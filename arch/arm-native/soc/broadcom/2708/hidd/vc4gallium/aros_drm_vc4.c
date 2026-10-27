/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    vc4 driver glue for the mesa3dgl-side gallium path: screen creation,
    the display/present path (which dereferences vc4_resource / vc4_bo /
    vc4_screen, so it must live here on the Mesa side, not in the
    Mesa-type-free hidd), and the vc4/v3d linker stubs the statically
    linked Mesa driver references.

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
#include <proto/oop.h>
#include <proto/layers.h>
#include <proto/alib.h>
#include <proto/graphics.h>

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
 * Display a rendered pipe_resource to a RastPort via the bridge,
 * replacing the gallium.library BltPipeResourceRastPort path for vc4.
 * The pipe_resource is dereferenced here (mesa3dgl-side, where Mesa
 * types live) into BO handle/stride/offset/cpp; bridge->display_blit
 * does the per-cliprect blit hidd-side.
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
     * whole blit as out of range. */
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

    LockLayerRom(L);

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
/* Private cpu caps copy (u_cpu_detect.o is linked into this module; the
 * provider's caps live in mesa3dgl and are a different object). Mesa 26
 * put them behind util_get_cpu_caps(), whose once-init this calls: the
 * accessor itself is ATTRIBUTE_CONST and would be optimised away. */
#if AROS_MESA_MAJOR >= 26
extern void _util_cpu_detect_once(void);
#define aros_util_cpu_detect() _util_cpu_detect_once()
#else
extern void util_cpu_detect(void);
#define aros_util_cpu_detect() util_cpu_detect()
#endif

struct pipe_screen *vc4_aros_create_screen(struct vc4_aros_bridge *bridge,
                                           APTR coreapi, int fd)
{
    struct pipe_screen *pscreen = NULL;

    if (!bridge)
        return NULL;
    if (bridge->version != VC4_AROS_BRIDGE_VERSION)
    {
        bug("[aros_drm_vc4] bridge version %lu, expected %d - rebuild"
            " mesa3dgl-library and this hidd together\n",
            (unsigned long)bridge->version, VC4_AROS_BRIDGE_VERSION);
        return NULL;
    }

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
    aros_util_cpu_detect();

    aros_drm_bridge = bridge;

    /* Dispatch on driver_id (one mesa3dgl can carry several drivers, one
     * per Pi GPU gen). Only VC4 today; V3D slots into the commented case.
     * fd names the session to the hidd; NULL renderonly since we own the
     * display path. */
    switch (bridge->driver_id)
    {
    case AROS_GALLIUM_DRIVER_VC4:
        /* vc4_screen.h declares it; Mesa 26 takes a screen config too. */
#if AROS_MESA_MAJOR >= 26
        pscreen = vc4_screen_create(fd, NULL, NULL);
#else
        pscreen = vc4_screen_create(fd, NULL);
#endif
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

/* The last screen is gone. The BO sweep itself is the caller's job (it
 * owns the bo_table). */
void aros_drm_release_bridge(void)
{
    aros_drm_bridge = NULL;
}
