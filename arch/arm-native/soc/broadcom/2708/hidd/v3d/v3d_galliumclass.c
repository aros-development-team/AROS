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
#include "pipe/p_screen.h"
#include "util/u_inlines.h"
#include "broadcom/common/v3d_limits.h"
#include "v3d_screen.h"
#include "v3d_bufmgr.h"
#include "v3d_resource.h"
#include "v3d_tiling.h"
#include "frontend/winsys_handle.h"
#include "drm-uapi/drm_fourcc.h"

/* Compiled outside the driver archive, so objcopy does not rename the plain
 * names the v3d_bo_unreference inline reaches for. Forward each to its
 * trampoline; a drifted callee list shows up as an undefined plain name. */
extern void __gca__mesa_hash_table_remove_key(struct hash_table *ht,
                                              const void *key);
void _mesa_hash_table_remove_key(struct hash_table *ht, const void *key)
{
    __gca__mesa_hash_table_remove_key(ht, key);
}

extern int __gca_mtx_lock(mtx_t *mtx);
int mtx_lock(mtx_t *mtx)
{
    return __gca_mtx_lock(mtx);
}

extern int __gca_mtx_unlock(mtx_t *mtx);
int mtx_unlock(mtx_t *mtx)
{
    return __gca_mtx_unlock(mtx);
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

/* The session a pipe screen belongs to, 0 for none. */
static ULONG v3d_session_of(struct V3DData *sd, APTR screen)
{
    ULONG i, sid = 0;

    ObtainSemaphore(&sd->bo_lock);
    for (i = 0; i < V3D_MAX_SESSIONS; i++)
    {
        if (sd->sessions[i].vs_Used && (sd->sessions[i].vs_Screen == screen))
        {
            sid = i + 1;
            break;
        }
    }
    ReleaseSemaphore(&sd->bo_lock);
    return sid;
}

/* Forget every job in flight. Caller holds job_lock, the GPU is idle or
 * about to be reset. */
static void v3d_pipeline_reset(struct V3DData *sd)
{
    sd->recoveries = 0;
    sd->bin_running = FALSE;
    sd->render_running = FALSE;
    sd->rcl_head = 0;
    sd->rcl_count = 0;
    sd->bin_flushed_seqno = sd->seqno;
    sd->finished_seqno = sd->seqno;
    sd->bin_end = 0;
    sd->render_end = 0;
}

/* Sessions whose task died without destroying its screen: their BOs go. */
static void v3d_sweep_dead(struct V3DData *sd)
{
    ULONG i;
    BOOL dead = FALSE;

    ObtainSemaphore(&sd->bo_lock);
    for (i = 0; (i < V3D_MAX_SESSIONS) && !dead; i++)
        dead = sd->sessions[i].vs_Used && sd->sessions[i].vs_Dead;
    ReleaseSemaphore(&sd->bo_lock);
    if (!dead)
        return;

    v3d_wait_idle(sd);

    for (i = 0; i < V3D_MAX_SESSIONS; i++)
    {
        /* Claim it, so a concurrent sweep leaves it alone. */
        ObtainSemaphore(&sd->bo_lock);
        dead = sd->sessions[i].vs_Used && sd->sessions[i].vs_Dead;
        if (dead)
            sd->sessions[i].vs_Dead = FALSE;
        ReleaseSemaphore(&sd->bo_lock);
        if (!dead)
            continue;

        v3d_release_session(sd, i + 1);
        bug("[V3D] session %u: its task ended without a GL teardown, swept\n",
            (unsigned)(i + 1));

        ObtainSemaphore(&sd->bo_lock);
        sd->sessions[i].vs_Used = FALSE;
        sd->sessions[i].vs_Dead = FALSE;
        ReleaseSemaphore(&sd->bo_lock);
    }
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
    ULONG i;
    int bres;

    /* The teardown that resets the recovery fuse rarely runs, so a new
     * session revives a fused-off GPU itself. */
    if (!sd->powered)
    {
        ObtainSemaphore(&sd->job_lock);
        v3d_pipeline_reset(sd);
        ReleaseSemaphore(&sd->job_lock);
        if (v3d_block_reset() && v3d_hw_init(sd))
            bug("[V3D] GPU revived for this session\n");
    }

    if (!sd->powered)
    {
        bug("[V3D] GPU stays down - GL falls back to softpipe\n");
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

    v3d_sweep_dead(sd);

    /* A session per screen, named to the shim by the fd. */
    ObtainSemaphore(&sd->bo_lock);
    for (i = 0; (i < V3D_MAX_SESSIONS) && sd->sessions[i].vs_Used; i++)
        ;
    if (i < V3D_MAX_SESSIONS)
    {
        sd->sessions[i].vs_Used   = TRUE;
        sd->sessions[i].vs_Dead   = FALSE;
        sd->sessions[i].vs_Screen = NULL;
        sd->sessions[i].vs_Task   = FindTask(NULL);
    }
    ReleaseSemaphore(&sd->bo_lock);
    if (i == V3D_MAX_SESSIONS)
    {
        bug("[V3D] %u GL screens open already - GL falls back to softpipe\n",
            (unsigned)V3D_MAX_SESSIONS);
        return NULL;
    }

    /* There is no renderonly - we present ourselves. The config must be a
     * real object: v3d_screen_create derefs it for driconf. */
    {
        static const struct pipe_screen_config v3d_no_config = { 0 };

        screen = v3d_screen_create((int)(V3D_SCREENFD_TAG | (i + 1)),
                                   &v3d_no_config, NULL);
    }
    if (!screen)
    {
        bug("[V3D] v3d_screen_create failed\n");
        v3d_release_session(sd, i + 1);
        ObtainSemaphore(&sd->bo_lock);
        sd->sessions[i].vs_Used = FALSE;
        ReleaseSemaphore(&sd->bo_lock);
        return NULL;
    }
    ObtainSemaphore(&sd->bo_lock);
    sd->sessions[i].vs_Screen = screen;
    ReleaseSemaphore(&sd->bo_lock);

    D(bug("[V3D] pipe_screen at %p\n", screen));
    return screen;
}

VOID HiddV3D__Hidd_Gallium__DestroyPipeScreen(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Gallium_DestroyPipeScreen *msg)
{
    struct V3DData *sd = SD(cl);
    struct pipe_screen *screen = (struct pipe_screen *)msg->screen;
    ULONG sid, i;

    if (!screen)
        return;

    sid = v3d_session_of(sd, screen);

    /* Quiesce first: context teardown can flush a trailing job, and
     * the screen is about to free the memory it renders from. */
    v3d_wait_idle(sd);

    screen->destroy(screen);

    if (sid)
    {
        v3d_release_session(sd, sid);
        ObtainSemaphore(&sd->bo_lock);
        sd->sessions[sid - 1].vs_Used = FALSE;
        ReleaseSemaphore(&sd->bo_lock);
    }
    v3d_sweep_dead(sd);

    ObtainSemaphore(&sd->bo_lock);
    for (i = 0; (i < V3D_MAX_SESSIONS) && !sd->sessions[i].vs_Used; i++)
        ;
    ReleaseSemaphore(&sd->bo_lock);
    if (i < V3D_MAX_SESSIONS)
        return;

    /* The last GL screen is gone: sweep what is left, and start the next
     * one clean instead of inheriting a stale latch or a blown recovery
     * fuse - and give a fuse-disabled GPU a fresh chance. */
    v3d_release_all_bos(sd);
    /* Only a real teardown frees the arenas: nothing uses them now. */
    v3d_mem_release(sd);
    ObtainSemaphore(&sd->job_lock);
    v3d_pipeline_reset(sd);
    ReleaseSemaphore(&sd->job_lock);
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
 * linear rows, which works for any bitmap.
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

    if (slice->tiling == V3D_TILING_RASTER)
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
 * Whole-window present: a per-cliprect WritePixelArray from the raster
 * resource. GL that renders straight into display memory goes through
 * gallium.library's scan-out framebuffers (ResourceFromBitMap) instead.
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
    struct Layer *L;
    struct Rectangle renderableLayerRect, result;
    struct ClipRect *CR;
    OOP_Object *scr_bm_obj;
    ULONG stride;
    BOOL copied = FALSE;
    UBYTE *base;

    if (!sd->powered || !rsc || !rsc->bo || !rp)
        return FALSE;
    /* Tiled or exotic sources go through the detiling DisplayResource. */
    if (rsc->slices[0].tiling != V3D_TILING_RASTER || rsc->cpp != 4
        || rsc->slices[0].offset != 0)
        return FALSE;
    if (!(L = rp->Layer))
        return FALSE;
    if (!IsLayerVisible(L))
        return TRUE;

    /* Clamp to the resource: during a resize the new window size arrives
     * while the resource is still last frame's allocation. */
    if (xSrc >= (LONG)rsc->base.width0 || ySrc >= (LONG)rsc->base.height0)
        return TRUE;
    if (xSize > (LONG)rsc->base.width0 - xSrc)
        xSize = (LONG)rsc->base.width0 - xSrc;
    if (ySize > (LONG)rsc->base.height0 - ySrc)
        ySize = (LONG)rsc->base.height0 - ySrc;

    stride = rsc->slices[0].stride;

    LockLayerRom(L);
    scr_bm_obj = HIDD_BM_OBJ(rp->BitMap);

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
            bug("[V3D] present blit: %ldx%ld at %ld,%ld\n",
                (long)xSize, (long)ySize, (long)xDest, (long)yDest);
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

    UnlockLayerRom(L);
    return TRUE;
}

/*
 * Alias buffer msg->buffer of a scan-out bitmap (vcgfx, in its scanout
 * memory) as a linear render target of the pipe screen: its address is
 * published as a GEM_OPEN name and imported by Mesa with the bitmap's
 * pitch, so GL renders straight into what a plane scans out.
 */
APTR HiddV3D__Hidd_Gallium__ResourceFromBitMap(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Gallium_ResourceFromBitMap *msg)
{
    struct V3DData *sd = SD(cl);
    struct pipe_screen *pscreen = (struct pipe_screen *)msg->screen;
    OOP_Object *bm = msg->bitmap, *pf = NULL;
    IPTR scanout = 0, width = 0, height = 0, stdpf = 0;
    struct pHidd_BitMap_LockBuffer lmsg;
    struct pHidd_BitMap_UnlockBuffer umsg;
    struct Rectangle none = { 0, 0, -1, -1 };
    struct winsys_handle wh;
    struct pipe_resource templ;
    enum pipe_format format;
    UBYTE *addr = NULL;
    ULONG pitch = 0, name, i;

    if (!pscreen || !pscreen->resource_from_handle || !bm
        || !sd->hiddBitMapAB || !sd->hiddPixFmtAB)
        return NULL;

    OOP_GetAttr(bm, sd->hiddBitMapAB + aoHidd_BitMap_Scanout, &scanout);
    OOP_GetAttr(bm, sd->hiddBitMapAB + aoHidd_BitMap_Width, &width);
    OOP_GetAttr(bm, sd->hiddBitMapAB + aoHidd_BitMap_Height, &height);
    OOP_GetAttr(bm, sd->hiddBitMapAB + aoHidd_BitMap_PixFmt, (IPTR *)&pf);
    if (pf)
        OOP_GetAttr(pf, sd->hiddPixFmtAB + aoHidd_PixFmt_StdPixFmt, &stdpf);
    if (!scanout || !width || !height)
        return NULL;

    switch (stdpf)
    {
    case vHidd_StdPixFmt_BGRA32:
        format = PIPE_FORMAT_B8G8R8A8_UNORM;
        break;
    case vHidd_StdPixFmt_BGR032:
        format = PIPE_FORMAT_B8G8R8X8_UNORM;
        break;
    case vHidd_StdPixFmt_RGB16_LE:
        format = PIPE_FORMAT_B5G6R5_UNORM;
        break;
    default:
        return NULL;
    }

    lmsg.mID           = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_LockBuffer);
    lmsg.buffer        = msg->buffer;
    lmsg.addressReturn = &addr;
    lmsg.pitchReturn   = &pitch;
    lmsg.planeOffsets  = NULL;
    if (!OOP_DoMethod(bm, (OOP_Msg)&lmsg) || !addr || !pitch)
        return NULL;
    umsg.mID    = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_UnlockBuffer);
    umsg.buffer = msg->buffer;
    umsg.dirty  = &none;
    OOP_DoMethod(bm, (OOP_Msg)&umsg);

    /* Scanout memory is identity mapped: the address is the page's */
    name = (ULONG)(IPTR)addr;
    ObtainSemaphore(&sd->bo_lock);
    for (i = 0; i < V3D_EXT_NAMES; i++)
        if (sd->ext_name[i] == name)
            break;
    if (i == V3D_EXT_NAMES)
    {
        i = sd->ext_next;
        sd->ext_next = (sd->ext_next + 1) % V3D_EXT_NAMES;
    }
    sd->ext_name[i] = name;
    sd->ext_size[i] = pitch * height;
    ReleaseSemaphore(&sd->bo_lock);

    memset(&wh, 0, sizeof(wh));
    wh.type     = WINSYS_HANDLE_TYPE_SHARED;
    wh.handle   = name;
    wh.stride   = pitch;
    wh.modifier = DRM_FORMAT_MOD_LINEAR;

    memset(&templ, 0, sizeof(templ));
    templ.target     = PIPE_TEXTURE_2D;
    templ.format     = format;
    templ.width0     = width;
    templ.height0    = height;
    templ.depth0     = 1;
    templ.array_size = 1;
    templ.bind       = PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW
                     | PIPE_BIND_SCANOUT | PIPE_BIND_LINEAR | PIPE_BIND_SHARED;
    templ.usage      = PIPE_USAGE_DEFAULT;

    return pscreen->resource_from_handle(pscreen, &templ, &wh,
                                         PIPE_HANDLE_USAGE_FRAMEBUFFER_WRITE);
}
