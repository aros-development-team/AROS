/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    VideoCore VI (V3D) - Gallium HIDD class.

    Unlike vc4gallium, which splits the Mesa driver from its winsys across
    a bridge, the Mesa v3d driver is linked into this module: its types are
    ours to dereference, so presenting a rendered resource needs no ABI
    boundary. What it does need is detiling - V3D renders into UIF-tiled
    memory, and a straight copy of that to a linear framebuffer produces
    the tile pattern instead of a picture. Mesa's own tiling code is in
    the archive, so the detiler is the one it uses itself.
*/

#define DEBUG 0
#include <aros/debug.h>
#include <proto/oop.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/cybergraphics.h>
#include <cybergraphx/cybergraphics.h>
#include <graphics/rastport.h>
#include <graphics/clip.h>
#include <hidd/gallium.h>
#include <hidd/gfx.h>

#include "v3d_intern.h"

/* aros/debug.h leaves DEBUG defined, which makes u_inlines.h's
 * pipe_reference emit refs to debug_reference_slowpath & co - symbols
 * that only exist in mesa3dgl's mesautil, not in this module. NDEBUG
 * keeps Mesa's own asserts out of the object for the same reason. */
#undef DEBUG
#define NDEBUG 1

#include "pipe/p_state.h"
#include "util/u_inlines.h"
#include "broadcom/common/v3d_limits.h"
#include "v3d_screen.h"
#include "v3d_bufmgr.h"
#include "v3d_resource.h"
#include "v3d_tiling.h"

/* The v3d_bo_unreference inline this file instantiates calls
 * util_hash_table_remove by its plain name; the driver archive reaches
 * that function through the GalliumCoreAPI trampolines (objcopy renames
 * its refs to __gca_*), but this object is compiled outside the archive,
 * so forward the plain name to the same trampoline. */
struct util_hash_table;
extern void __gca_util_hash_table_remove(struct util_hash_table *ht,
                                         void *key);
void util_hash_table_remove(struct util_hash_table *ht, void *key)
{
    __gca_util_hash_table_remove(ht, key);
}

#undef HiddGalliumAttrBase
#define HiddGalliumAttrBase (SD(cl)->hiddGalliumAB)

/* What the driver actually renders, in WritePixelArray's terms. The pipe
 * format is a little-endian RGBA word, which reaches memory as B,G,R,A -
 * naming it ARGB instead swaps the channels, which is what turned the
 * triangle's colours inside out. */
#if (AROS_BIG_ENDIAN == 1)
#define V3D_PIXFMT  RECTFMT_RAW
#else
#define V3D_PIXFMT  RECTFMT_BGRA32
#endif

struct pipe_screen;
struct pipe_screen_config;
struct renderonly;
extern struct pipe_screen *v3d_screen_create(int fd,
    const struct pipe_screen_config *config, struct renderonly *ro);

struct GalliumCoreAPI;
extern int gca_bind(const struct GalliumCoreAPI *api);
extern const char *gca_slot_name(unsigned int slot);

/* Set for the drmIoctl override macro, which has no other way to reach
 * the driver's state. */
extern struct V3DData *g_v3d_data;

/* The vcgfx framebuffer's physical address and pitch, or FALSE when the
 * target is some other kind of bitmap. */
static BOOL v3d_fb_target(struct V3DData *sd, struct BitMap *bitmap,
                          IPTR *fb, IPTR *bpr)
{
    OOP_Object *bmobj;

    *fb = 0;
    *bpr = 0;
    if (!bitmap || !sd->hiddVCGfxBMAB || !sd->hiddBitMapAB)
        return FALSE;

    bmobj = HIDD_BM_OBJ(bitmap);
    if (!bmobj)
        return FALSE;

    OOP_GetAttr(bmobj, sd->hiddVCGfxBMAB + aoVCGfxBM_Drawable, fb);
    OOP_GetAttr(bmobj, sd->hiddBitMapAB + aoHidd_BitMap_BytesPerRow, bpr);
    return (*fb && *bpr);
}

/*
 * Zero-copy present state (vc4gallium's proven model, without the bridge:
 * Mesa's types are linked into this module, so the resource is rebound
 * directly).
 *
 * Fullscreen: the render resource's BO is replaced with a wrap of the
 * framebuffer back page - the GPU renders straight into scanout and each
 * present is a page flip, no pixels move. Windowed: the rendered BO goes
 * on an HVS overlay plane, ping-ponging with one private page (the page
 * leaving the plane is off-screen once the Set latches, so two suffice).
 *
 * Single-session state, like the BO table. On a dirty exit (no GL
 * teardown) the pointers go stale; CreatePipeScreen drops them without
 * dereferencing when a new session starts.
 */
static struct
{
    struct pipe_resource *rsc;      /* resource bound to a page (identity) */
    ULONG           page_handle;    /* shim handle of the bound page */
    ULONG           name[2];        /* page names (phys addrs) */
    struct v3d_bo  *page_bo[2];     /* held refs: rebinding must not close
                                     * and reopen a page (with its MMU
                                     * unmap/map and TLB flushes) per frame */
} v3d_scan;

static struct
{
    struct pipe_resource *rsc;
    struct v3d_bo  *onplane;        /* page the overlay currently scans */
    ULONG           page_handle;    /* BO bound in rsc (render target) */
    BOOL            shown;
    struct pipe_resource *refused;  /* overlay said no (scaled desktop /
                                     * no takeover): don't retry per frame */
    OOP_Object     *bm;
} v3d_ovl;

/* Raw drop, for the dirty-exit reset where the Mesa objects are gone. */
static void v3d_scan_forget(void)
{
    v3d_scan.rsc = NULL;
    v3d_scan.page_handle = 0;
    v3d_scan.name[0] = v3d_scan.name[1] = 0;
    v3d_scan.page_bo[0] = v3d_scan.page_bo[1] = NULL;
}

/* Normal teardown: give the held page refs back first. */
static void v3d_scan_release(void)
{
    if (v3d_scan.page_bo[0])
        v3d_bo_unreference(&v3d_scan.page_bo[0]);
    if (v3d_scan.page_bo[1])
        v3d_bo_unreference(&v3d_scan.page_bo[1]);
    v3d_scan_forget();
}

struct v3d_scanout_info
{
    ULONG name[2];      /* page phys addrs, ascending (stable identity) */
    UBYTE back;         /* index of the current back page */
    ULONG pitch, width, height;
};

/* Resolve the vcgfx flip pages and publish them as GEM_OPEN names. */
static BOOL v3d_get_scanout(struct V3DData *sd, OOP_Object *bmobj,
                            struct v3d_scanout_info *out)
{
    IPTR front = 0, back = 0, pitch = 0, width = 0, height = 0;

    if (!bmobj || !sd->hiddVCGfxBMAB || !sd->hiddBitMapAB)
        return FALSE;

    OOP_GetAttr(bmobj, sd->hiddVCGfxBMAB + aoVCGfxBM_Drawable, &front);
    OOP_GetAttr(bmobj, sd->hiddVCGfxBMAB + aoVCGfxBM_BackDrawable, &back);
    OOP_GetAttr(bmobj, sd->hiddBitMapAB + aoHidd_BitMap_BytesPerRow, &pitch);
    OOP_GetAttr(bmobj, sd->hiddBitMapAB + aoHidd_BitMap_Width, &width);
    OOP_GetAttr(bmobj, sd->hiddBitMapAB + aoHidd_BitMap_Height, &height);

    if (!front || !back || !pitch || !width || !height)
        return FALSE;

    out->name[0] = (front < back) ? (ULONG)front : (ULONG)back;
    out->name[1] = (front < back) ? (ULONG)back : (ULONG)front;
    out->back    = ((ULONG)back == out->name[1]) ? 1 : 0;
    out->pitch   = (ULONG)pitch;
    out->width   = (ULONG)width;
    out->height  = (ULONG)height;

    ObtainSemaphore(&sd->bo_lock);
    sd->scanout_phys[0] = out->name[0];
    sd->scanout_phys[1] = out->name[1];
    sd->scanout_size = (ULONG)pitch * (ULONG)height;
    ReleaseSemaphore(&sd->bo_lock);
    return TRUE;
}

/* Flip the framebuffer pages; the hidd waits for the vblank latch.
 * Success iff the back page changed. */
static BOOL v3d_flip_fb(struct V3DData *sd, OOP_Object *bmobj)
{
    struct TagItem fliptags[] =
    {
        { sd->hiddVCGfxBMAB + aoVCGfxBM_Flip, TRUE },
        { TAG_DONE, 0 }
    };
    IPTR before = 0, after = 0;

    OOP_GetAttr(bmobj, sd->hiddVCGfxBMAB + aoVCGfxBM_BackDrawable, &before);
    if (!before)
        return FALSE;
    OOP_SetAttrs(bmobj, fliptags);
    OOP_GetAttr(bmobj, sd->hiddVCGfxBMAB + aoVCGfxBM_BackDrawable, &after);
    return (after && after != before);
}

/* Bind the current back page into the render resource: the next frame
 * is rendered directly into scanout. Both page BOs are opened once and
 * the refs held in v3d_scan - the steady state is pointer swaps, not a
 * GEM close/open (each of which is an MMU unmap/map plus TLB flushes)
 * per frame. */
static BOOL v3d_scan_bind_back(struct V3DData *sd, struct v3d_resource *rsc,
                               OOP_Object *bmobj)
{
    struct v3d_scanout_info so;
    struct v3d_bo *nb;
    ULONG i;

    if (!v3d_get_scanout(sd, bmobj, &so))
        return FALSE;

    if (so.name[0] != v3d_scan.name[0] || so.name[1] != v3d_scan.name[1])
        v3d_scan_release();     /* other pages (mode switch) */

    for (i = 0; i < 2; i++)
        if (!v3d_scan.page_bo[i])
            v3d_scan.page_bo[i] =
                v3d_bo_open_name(v3d_screen(rsc->base.screen), so.name[i]);

    nb = v3d_scan.page_bo[so.back];
    if (!nb)
    {
        v3d_scan_release();
        return FALSE;
    }

    v3d_bo_reference(nb);
    v3d_bo_unreference(&rsc->bo);
    rsc->bo = nb;
    rsc->slices[0].offset = 0;

    v3d_scan.rsc = &rsc->base;
    v3d_scan.page_handle = nb->handle;
    v3d_scan.name[0] = so.name[0];
    v3d_scan.name[1] = so.name[1];
    return TRUE;
}

/* Detach from the page: a private BO again, so windowed rendering can't
 * scribble into the framebuffer. */
static void v3d_scan_unbind(struct v3d_resource *rsc)
{
    struct v3d_bo *nb = v3d_bo_alloc(v3d_screen(rsc->base.screen),
                                     rsc->slices[0].size, "scanout-exit");

    if (nb)
    {
        v3d_bo_unreference(&rsc->bo);
        rsc->bo = nb;
        rsc->slices[0].offset = 0;
    }
    v3d_scan_release();
}

/* Show a BO on the HVS overlay plane at fb coords x,y. The hidd waits
 * for the vblank latch, so the page leaving the plane is off-screen when
 * this returns. */
static BOOL v3d_show_overlay(struct V3DData *sd, OOP_Object *bmobj,
                             struct v3d_bo *bo, ULONG stride,
                             LONG x, LONG y, ULONG w, ULONG h)
{
    struct vc4gfx_overlay desc;
    struct TagItem ovltags[] =
    {
        { sd->hiddVCGfxBMAB + aoVCGfxBM_Overlay, (IPTR)&desc },
        { TAG_DONE, 0 }
    };
    IPTR active = 0;
    ULONG paddr = 0;

    if (!bmobj || !sd->hiddVCGfxBMAB || !w || !h)
        return FALSE;

    ObtainSemaphore(&sd->bo_lock);
    if (bo->handle < V3D_MAX_BOS && sd->bo_table[bo->handle].refcount
        && stride * h <= sd->bo_table[bo->handle].size)
        paddr = sd->bo_table[bo->handle].paddr;
    ReleaseSemaphore(&sd->bo_lock);
    if (!paddr)
        return FALSE;

    desc.ovl_Phys   = paddr;
    desc.ovl_Pitch  = stride;
    desc.ovl_Width  = w;
    desc.ovl_Height = h;
    desc.ovl_X      = x;
    desc.ovl_Y      = y;
    desc.ovl_DestW  = w;
    desc.ovl_DestH  = h;

    OOP_SetAttrs(bmobj, ovltags);
    OOP_GetAttr(bmobj, sd->hiddVCGfxBMAB + aoVCGfxBM_Overlay, &active);
    return active != 0;
}

/* Hide the plane but keep the page pair: obscure/reveal cycles must not
 * allocate or free BOs. */
static void v3d_ovl_suspend(struct V3DData *sd)
{
    struct TagItem ovltags[] =
    {
        { sd->hiddVCGfxBMAB + aoVCGfxBM_Overlay, 0 },
        { TAG_DONE, 0 }
    };

    if (v3d_ovl.shown && v3d_ovl.bm)
        OOP_SetAttrs(v3d_ovl.bm, ovltags);
    v3d_ovl.shown = FALSE;
}

/* Full teardown: resource changed or the session is going away. */
static void v3d_ovl_exit(struct V3DData *sd)
{
    v3d_ovl_suspend(sd);
    if (v3d_ovl.onplane)
        v3d_bo_unreference(&v3d_ovl.onplane);
    v3d_ovl.rsc = NULL;
    v3d_ovl.page_handle = 0;
    v3d_ovl.bm = NULL;
}

OOP_Object *HiddV3D__Root__New(OOP_Class *cl, OOP_Object *o,
                               struct pRoot_New *msg)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
        SD(cl)->coreapi = (APTR)GetTagData(aHidd_Gallium_CoreAPI, 0,
                                           msg->attrList);
    return o;
}

APTR HiddV3D__Hidd_Gallium__CreatePipeScreen(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Gallium_CreatePipeScreen *msg)
{
    struct V3DData *sd = SD(cl);
    struct pipe_screen *screen;
    int bres;

    if (!sd->powered)
    {
        D(bug("[V3D] GPU not available\n"));
        return NULL;
    }

    if (!sd->coreapi)
    {
        bug("[V3D] GalliumCoreAPI table missing (old mesa3dgl?)\n");
        return NULL;
    }

    /* Bind the Mesa-core trampolines before any driver code runs. */
    bres = gca_bind((const struct GalliumCoreAPI *)sd->coreapi);
    if (bres > 0)
    {
        bug("[V3D] GalliumCoreAPI bind refused: slot %d is not '%s'\n",
            bres - 1, gca_slot_name((unsigned int)(bres - 1)));
        return NULL;
    }
    if (bres < 0)
    {
        bug("[V3D] GalliumCoreAPI bind refused (code %d)\n", bres);
        return NULL;
    }

    g_v3d_data = sd;

    /* A previous app that exited without GL teardown left the present
     * state pointing at freed Mesa objects. Drop the pointers without
     * dereferencing - the leaked BOs sit in the bo_table and go with the
     * next session sweep. */
    ObtainSemaphore(&sd->bo_lock);
    if (sd->screen_count == 0)
    {
        v3d_scan_forget();
        v3d_ovl.rsc = NULL;
        v3d_ovl.onplane = NULL;
        v3d_ovl.page_handle = 0;
        v3d_ovl.shown = FALSE;
        v3d_ovl.refused = NULL;
        v3d_ovl.bm = NULL;
    }
    ReleaseSemaphore(&sd->bo_lock);

    /* fd is a dummy: the ioctl override never looks at it. No driconf and
     * no renderonly - we present through the display driver ourselves. */
    screen = v3d_screen_create(0, NULL, NULL);
    if (!screen)
    {
        bug("[V3D] v3d_screen_create failed\n");
        return NULL;
    }

    ObtainSemaphore(&sd->bo_lock);
    sd->screen_count++;
    ReleaseSemaphore(&sd->bo_lock);

    D(bug("[V3D] pipe_screen at %p\n", screen));
    return screen;
}

VOID HiddV3D__Hidd_Gallium__DestroyPipeScreen(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Gallium_DestroyPipeScreen *msg)
{
    struct V3DData *sd = SD(cl);
    BOOL last;

    if (msg->screen)
    {
        struct pipe_screen *screen = (struct pipe_screen *)msg->screen;

        /* Quiesce first: context teardown can flush a trailing job, and
         * the screen is about to free the memory it renders from. */
        v3d_wait_idle(sd);

        /* Drop the present state while every screen is still alive - the
         * overlay page and the scanout wrap belong to one of them, and
         * which one cannot be told from here. Re-entry is one blitted
         * frame for a survivor. */
        v3d_ovl_exit(sd);
        v3d_scan_release();

        screen->destroy(screen);
    }

    /* Session teardown only with the LAST screen gone - a GL app holds
     * more than one (the capability probe's, then the real one), and a
     * sweep under a live screen would free its BOs mid-render. */
    ObtainSemaphore(&sd->bo_lock);
    last = (--sd->screen_count <= 0);
    if (last)
        sd->screen_count = 0;
    ReleaseSemaphore(&sd->bo_lock);
    if (!last)
        return;

    /* Sweep what Mesa leaked and drop the per-session state, so the next
     * GL session starts clean instead of inheriting a stale latch or a
     * blown recovery fuse - and give a fuse-disabled GPU a fresh chance. */
    v3d_release_all_bos(sd);
    sd->bin_running = FALSE;
    sd->render_running = FALSE;
    sd->pending_rcl_start = 0;
    sd->pending_rcl_end = 0;
    sd->finished_seqno = sd->seqno;
    sd->bin_end = 0;
    sd->render_end = 0;
    sd->recoveries = 0;
    if (sd->powered)
    {
        /* W1C any leftover completion latches so the next session's
         * first wait can't be satisfied by this session's events. */
        *(volatile ULONG *)(sd->core0_base + V3D_CTL_INT_CLR) = 0xffffffff;
    }
    else if (v3d_block_reset())
    {
        if (v3d_hw_init(sd))
            bug("[V3D] GPU revived for the next session\n");
    }
}

/*
 * Present one rectangle of a rendered resource into a bitmap.
 *
 * The GPU has to be idle before the pixels are read, and the read itself
 * goes through Mesa's detiler unless the slice is already raster - both
 * are correctness, not optimisation. WritePixelArray then places the
 * linear rows, which works for any bitmap; routing a linear resource
 * straight into the vcgfx framebuffer through DMA is the obvious next
 * step, and the reason this is a separate method from the flip path.
 */
VOID HiddV3D__Hidd_Gallium__DisplayResource(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Gallium_DisplayResource *msg)
{
    struct V3DData *sd = SD(cl);
    struct v3d_resource *rsc = (struct v3d_resource *)msg->resource;
    struct v3d_resource_slice *slice;
    struct RastPort *rp;
    UBYTE *base, *src;
    ULONG stride;
    APTR detiled = NULL;

    if (!rsc || !rsc->bo || !msg->bitmap
        || msg->width == 0 || msg->height == 0)
    {
        bug("[V3D] present: nothing to do (rsc=%p bo=%p bm=%p %ux%u)\n",
            rsc, rsc ? rsc->bo : NULL, msg->bitmap,
            (unsigned)msg->width, (unsigned)msg->height);
        return;
    }

    slice = &rsc->slices[0];
    v3d_wait_idle(sd);
    v3d_flush_caches(sd);

    /* bo->map is populated lazily - a target the GPU rendered into has
     * never been mapped, so reading the field directly finds NULL and
     * every frame goes silently missing. */
    base = v3d_bo_map(rsc->bo);
    if (!base)
    {
        bug("[V3D] present: cannot map bo %u\n",
            (unsigned)rsc->bo->handle);
        return;
    }

    if (slice->tiling == VC5_TILING_RASTER)
    {
        src = base + slice->offset
            + msg->srcy * slice->stride + msg->srcx * rsc->cpp;
        stride = slice->stride;

    }
    else
    {
        /* Tiled: pull the rectangle out through Mesa's own detiler, which
         * is the only thing that knows the UIF layout for this format. */
        struct pipe_box box;

        stride = msg->width * rsc->cpp;
        detiled = AllocVec(stride * msg->height, MEMF_ANY);
        if (!detiled)
            return;

        box.x = msg->srcx;
        box.y = msg->srcy;
        box.z = 0;
        box.width = msg->width;
        box.height = msg->height;
        box.depth = 1;

        v3d_load_tiled_image(detiled, stride, base + slice->offset,
                             slice->stride, slice->tiling, rsc->cpp,
                             slice->padded_height, &box);
        src = detiled;
    }

    /* Once per geometry change: enough to tell a present that never
     * happens from one landing in the wrong place. */
    {
        static ULONG w = 0, h = 0, n = 0;

        if (n < 20 && (msg->width != w || msg->height != h))
        {
            w = msg->width; h = msg->height; n++;
            bug("[V3D] present %ux%u cpp=%u tiling=%u stride=%u -> %u,%u\n",
                (unsigned)msg->width, (unsigned)msg->height,
                (unsigned)rsc->cpp, (unsigned)slice->tiling,
                (unsigned)stride, (unsigned)msg->dstx, (unsigned)msg->dsty);
        }
    }

    rp = CreateRastPort();
    if (rp)
    {
        rp->BitMap = msg->bitmap;
        WritePixelArray(src, 0, 0, stride, rp, msg->dstx, msg->dsty,
                        msg->width, msg->height, V3D_PIXFMT);
        FreeRastPort(rp);
    }

    if (detiled)
        FreeVec(detiled);
}

/*
 * Whole-window present. Decides between the three ways a frame reaches
 * the screen, in falling order of preference:
 *
 *   flip     - fullscreen surface matching the flippable framebuffer:
 *              the resource renders straight into the back page, so the
 *              present is OOP_SetAttrs(Flip) and a rebind. Zero copy.
 *   overlay  - fully visible window: the rendered BO goes on the HVS
 *              plane at the window's position; the resource ping-pongs
 *              with one private page. Zero copy.
 *   blit     - everything else (partially obscured, entry/exit frames):
 *              per-cliprect WritePixelArray, raster source.
 *
 * TRUE = handled here. FALSE sends gallium.library to the per-cliprect
 * DisplayResource loop - the only cases are a resource this method
 * cannot read linearly (UIF-tiled, non-32bpp) or a target without a
 * layer.
 */
IPTR HiddV3D__Hidd_Gallium__DisplayResourceRP(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Gallium_DisplayResourceRP *msg)
{
    struct V3DData *sd = SD(cl);
    struct v3d_resource *rsc = (struct v3d_resource *)msg->resource;
    struct RastPort *rp = msg->rastport;
    LONG xSrc = msg->srcx, ySrc = msg->srcy;
    LONG xDest = msg->dstx, yDest = msg->dsty;
    LONG xSize = msg->width, ySize = msg->height;
    LONG req_w, req_h;
    struct Layer *L;
    struct Rectangle renderableLayerRect, result;
    struct ClipRect *CR;
    OOP_Object *scr_bm_obj;
    ULONG stride;
    BOOL fullscreen, bound, windowed;
    BOOL enter_scanout = FALSE, detach_after_blit = FALSE;
    BOOL copied = FALSE;
    UBYTE *base;

    if (!sd->powered || !rsc || !rsc->bo || !rp)
        return FALSE;
    /* Tiled or exotic sources go through the detiling DisplayResource. */
    if (rsc->slices[0].tiling != VC5_TILING_RASTER || rsc->cpp != 4
        || rsc->slices[0].offset != 0)
        return FALSE;
    if (!(L = rp->Layer))
        return FALSE;
    if (!IsLayerVisible(L))
        return TRUE;

    /* Clamp to the resource: during a resize the new window size arrives
     * while the resource is still last frame's allocation. req_w/req_h
     * keep the caller's request for the geometry checks. */
    req_w = xSize;
    req_h = ySize;
    if (xSrc >= (LONG)rsc->base.width0 || ySrc >= (LONG)rsc->base.height0)
        return TRUE;
    if (xSize > (LONG)rsc->base.width0 - xSrc)
        xSize = (LONG)rsc->base.width0 - xSrc;
    if (ySize > (LONG)rsc->base.height0 - ySrc)
        ySize = (LONG)rsc->base.height0 - ySrc;

    stride = rsc->slices[0].stride;

    LockLayerRom(L);
    scr_bm_obj = HIDD_BM_OBJ(rp->BitMap);

    /* Drop stale state when the bound resource went away (resize). */
    if (v3d_scan.rsc && v3d_scan.rsc != &rsc->base)
        v3d_scan_release();
    if (v3d_ovl.rsc && v3d_ovl.rsc != &rsc->base)
        v3d_ovl_exit(sd);
    if (v3d_ovl.refused && v3d_ovl.refused != &rsc->base)
        v3d_ovl.refused = NULL;

    fullscreen = (xSrc == 0 && ySrc == 0 && xDest == 0 && yDest == 0 &&
                  L->bounds.MinX == 0 && L->bounds.MinY == 0 &&
                  L->ClipRect && !L->ClipRect->Next && !L->ClipRect->lobs);
    bound = (v3d_scan.rsc == &rsc->base &&
             rsc->bo->handle == v3d_scan.page_handle);

    if (fullscreen)
    {
        struct v3d_scanout_info so;
        ULONG t0, t1;

        if (v3d_get_scanout(sd, scr_bm_obj, &so)
            && (ULONG)xSize == so.width && (ULONG)ySize == so.height
            && stride == so.pitch)
        {
            /* The frame must be complete before it goes on scanout. */
            t0 = *(volatile ULONG *)V3D_SYSTIMER_CLO;
            v3d_wait_idle(sd);
            t1 = *(volatile ULONG *)V3D_SYSTIMER_CLO;

            if (bound && so.name[0] == v3d_scan.name[0]
                && so.name[1] == v3d_scan.name[1])
            {
                /* Steady state: the frame is already in the back page.
                 * Make it visible and rebind at the new back page. */
                if (v3d_flip_fb(sd, scr_bm_obj)
                    && v3d_scan_bind_back(sd, rsc, scr_bm_obj))
                {
                    /* Heartbeat with the frame-phase breakdown that
                     * decides what to optimise: emit = app+Mesa CPU time
                     * since the last present returned, render = the
                     * wait_idle for this frame's jobs, vsync = flip's
                     * latch wait. emit+render > one vblank period is
                     * what locks a 60 Hz panel to 30 FPS. */
                    static ULONG n = 0, emit_acc, rend_acc, vs_acc, prev;
                    ULONG t2 = *(volatile ULONG *)V3D_SYSTIMER_CLO;

                    if (prev)
                        emit_acc += t0 - prev;
                    rend_acc += t1 - t0;
                    vs_acc   += t2 - t1;
                    prev = t2;
                    n++;
                    if (n <= 2 || (n & 255) == 0)
                    {
                        ULONG f = (n <= 2) ? 1 : 256;

                        bug("[V3D] present flip #%u: %ldx%ld emit=%luus "
                            "render-wait=%luus vsync-wait=%luus\n",
                            (unsigned)n, (long)xSize, (long)ySize,
                            (unsigned long)(emit_acc / f),
                            (unsigned long)(rend_acc / f),
                            (unsigned long)(vs_acc / f));
                        emit_acc = rend_acc = vs_acc = 0;
                    }
                    UnlockLayerRom(L);
                    return TRUE;
                }
                /* Flip refused: this frame's pixels sit in the page
                 * rsc->bo still points at - blit them out, detach. */
                bug("[V3D] present: flip refused, leaving scanout mode\n");
                v3d_scan_release();
                detach_after_blit = TRUE;
            }
            else
            {
                /* Enter scanout mode: this frame is still in a private
                 * BO, so blit it below, then bind the back page for the
                 * next one. */
                bug("[V3D] present: entering scanout mode %ldx%ld "
                    "pitch=%u pages %08x/%08x\n", (long)xSize, (long)ySize,
                    (unsigned)so.pitch, so.name[0], so.name[1]);
                if (v3d_ovl.rsc == &rsc->base)
                    v3d_ovl_exit(sd);
                enter_scanout = TRUE;
            }
        }
        else if (bound)
        {
            /* Geometry changed under us. */
            bug("[V3D] present: leaving scanout mode (geometry changed)\n");
            detach_after_blit = TRUE;
        }
    }
    else if (bound)
    {
        bug("[V3D] present: leaving scanout mode (windowed/obscured)\n");
        detach_after_blit = TRUE;
    }

    /* Zero-copy windowed present: a fully visible window shows the
     * rendered BO as an HVS overlay plane. */
    if (!fullscreen && !bound && !detach_after_blit)
    {
        LONG absX = L->bounds.MinX + xDest;
        LONG absY = L->bounds.MinY + yDest;

        windowed = (xSrc == 0 && ySrc == 0 &&
                    xSize == (LONG)rsc->base.width0 &&
                    ySize == (LONG)rsc->base.height0 &&
                    req_w == xSize && req_h == ySize &&
                    L->ClipRect && !L->ClipRect->Next &&
                    !L->ClipRect->lobs);

        if (v3d_ovl.rsc == &rsc->base)
        {
            if (windowed && rsc->bo->handle == v3d_ovl.page_handle)
            {
                v3d_wait_idle(sd);
                if (v3d_show_overlay(sd, scr_bm_obj, rsc->bo, stride,
                                     absX, absY, xSize, ySize))
                {
                    /* Ping-pong: shown page goes on the plane, the page
                     * leaving it (off-screen since the Set latched)
                     * becomes the next render target. */
                    struct v3d_bo *off = v3d_ovl.onplane;
                    static ULONG n = 0;

                    if (++n <= 2 || (n & 511) == 0)
                        bug("[V3D] present overlay #%u: %ldx%ld at %ld,%ld\n",
                            (unsigned)n, (long)xSize, (long)ySize,
                            (long)absX, (long)absY);

                    v3d_ovl.onplane = rsc->bo;
                    v3d_ovl.shown = TRUE;
                    v3d_ovl.bm = scr_bm_obj;
                    rsc->bo = off;
                    rsc->slices[0].offset = 0;
                    v3d_ovl.page_handle = rsc->bo->handle;
                    UnlockLayerRom(L);
                    return TRUE;
                }
                /* Refused a present that worked before (mode change):
                 * tear down, don't retry for this resource. */
                bug("[V3D] present: overlay refused mid-session, "
                    "back to blitting\n");
                v3d_ovl_exit(sd);
                v3d_ovl.refused = &rsc->base;
            }
            else if (!windowed)
            {
                /* Obscured: hide the plane, keep the pages, blit. */
                v3d_ovl_suspend(sd);
            }
            else
            {
                /* Mesa replaced the BO under us - bookkeeping void. */
                v3d_ovl_exit(sd);
            }
        }
        else if (windowed && !v3d_ovl.rsc && rsc->slices[0].size
                 && v3d_ovl.refused != &rsc->base)
        {
            struct v3d_bo *nb = v3d_bo_alloc(v3d_screen(rsc->base.screen),
                                             rsc->slices[0].size,
                                             "overlay-page");

            if (nb)
            {
                v3d_wait_idle(sd);
                if (v3d_show_overlay(sd, scr_bm_obj, rsc->bo, stride,
                                     absX, absY, xSize, ySize))
                {
                    bug("[V3D] present: overlay mode %ldx%ld at %ld,%ld\n",
                        (long)xSize, (long)ySize, (long)absX, (long)absY);
                    v3d_ovl.rsc = &rsc->base;
                    v3d_ovl.onplane = rsc->bo;
                    v3d_ovl.shown = TRUE;
                    v3d_ovl.bm = scr_bm_obj;
                    rsc->bo = nb;
                    rsc->slices[0].offset = 0;
                    v3d_ovl.page_handle = nb->handle;
                    UnlockLayerRom(L);
                    return TRUE;
                }
                v3d_bo_unreference(&nb);
                /* Scaled desktop or no takeover: remember and stop
                 * paying an alloc+refusal every present. */
                bug("[V3D] present: overlay unavailable, blitting\n");
                v3d_ovl.refused = &rsc->base;
            }
        }
    }

    /* Blit path: entry/exit frames and everything the fast paths
     * declined. Raster source, so a per-cliprect WritePixelArray. */
    {
        /* Log on geometry change only - steady blitting (obscured
         * window) would flood the serial log per frame. */
        static struct { LONG w, h, x, y; ULONG count; } blast;

        if (blast.count < 50
            && (xSize != blast.w || ySize != blast.h
                || xDest != blast.x || yDest != blast.y))
        {
            blast.count++;
            blast.w = xSize; blast.h = ySize;
            blast.x = xDest; blast.y = yDest;
            bug("[V3D] present blit: %ldx%ld at %ld,%ld%s%s\n",
                (long)xSize, (long)ySize, (long)xDest, (long)yDest,
                enter_scanout ? " (scanout entry)" : "",
                detach_after_blit ? " (scanout exit)" : "");
        }
    }
    v3d_wait_idle(sd);

    base = v3d_bo_map(rsc->bo);
    if (!base)
    {
        UnlockLayerRom(L);
        return TRUE;
    }

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

    for (CR = L->ClipRect; CR; CR = CR->Next)
    {
        if (CR->lobs)
            continue;

        if (AndRectRect(&renderableLayerRect, &CR->bounds, &result))
        {
            LONG cr_srcx = xSrc + result.MinX - L->bounds.MinX - xDest;
            LONG cr_srcy = ySrc + result.MinY - L->bounds.MinY - yDest;
            struct RastPort *bltrp = CreateRastPort();

            if (bltrp)
            {
                bltrp->BitMap = rp->BitMap;
                WritePixelArray(base + cr_srcy * stride + cr_srcx * 4,
                                0, 0, stride, bltrp,
                                result.MinX, result.MinY,
                                result.MaxX - result.MinX + 1,
                                result.MaxY - result.MinY + 1,
                                V3D_PIXFMT);
                FreeRastPort(bltrp);
                copied = TRUE;
            }
        }
    }

    if (copied)
    {
        struct pHidd_BitMap_UpdateRect urmsg =
        {
            .mID    = OOP_GetMethodID(IID_Hidd_BitMap,
                                      moHidd_BitMap_UpdateRect),
            .x      = renderableLayerRect.MinX,
            .y      = renderableLayerRect.MinY,
            .width  = renderableLayerRect.MaxX - renderableLayerRect.MinX + 1,
            .height = renderableLayerRect.MaxY - renderableLayerRect.MinY + 1,
        };

        OOP_DoMethod(scr_bm_obj, (OOP_Msg)&urmsg);
    }

    /* This frame is on screen; switch the resource for the next one. */
    if (enter_scanout)
        v3d_scan_bind_back(sd, rsc, scr_bm_obj);
    else if (detach_after_blit)
        v3d_scan_unbind(rsc);

    UnlockLayerRom(L);
    return TRUE;
}
