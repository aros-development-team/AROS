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
#include <dos/dos.h>
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
    ULONG           owner;          /* session id of rsc's screen */
} v3d_scan;

/* Four pages, not two: the present shows the PREVIOUS frame, whose jobs
 * retired while this one was built, so it never waits on the frame just
 * submitted. Costs one frame of latency. The fourth lets the Set skip the
 * latch wait: a displaced page parks in `retiring` for one present.
 *
 * An app that stops presenting (draws only on change) would leave its
 * last frame queued forever, so a vblank with no present for a whole
 * frame has v3d_ovl_task put it on.
 *
 * One per session, each its own plane under its own key (its address).
 * Under sd->ovl_lock. */
struct v3d_ovl
{
    struct pipe_resource *rsc;
    struct v3d_bo  *onplane;        /* page the overlay currently scans */
    struct v3d_bo  *queued;         /* rendered; goes on the plane next */
    ULONG           queued_seqno;   /* the jobs that filled `queued` */
    struct v3d_bo  *freep;          /* parked spare, ring not yet full */
    struct v3d_bo  *retiring;       /* displaced last present, latch due */
    BOOL            latch_due;      /* a NOWAIT Set is still unlatched */
    ULONG           armed;          /* the commit it waits for */
    ULONG           page_handle;    /* BO bound in rsc (render target) */
    BOOL            shown;
    struct pipe_resource *refused;  /* overlay said no (scaled desktop /
                                     * no takeover): don't retry per frame */
    ULONG           busy_skip;      /* presents before the next entry try
                                     * while the planes are taken */
    OOP_Object     *bm;
    LONG            x, y;           /* where the last present put it */
    ULONG           w, h, stride;
    volatile ULONG  gen;            /* bumped per queued frame */
    volatile ULONG  vbl_gen;        /* gen at the last vblank */
    volatile ULONG  flush_gen;      /* gen the task was woken for */
    BOOL            flushed;        /* the task emptied the queue */
    BOOL            stale;          /* plane still up for a dead session */
};

static struct v3d_ovl v3d_ovls[V3D_MAX_SESSIONS];

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

/* GL apps with a live session. An app can hold more than one screen, and
 * contention is between apps, so each task counts once. */
static ULONG v3d_live_apps(struct V3DData *sd)
{
    struct V3DSession *s = sd->sessions;
    ULONG i, j, n = 0;

    ObtainSemaphore(&sd->bo_lock);
    for (i = 0; i < V3D_MAX_SESSIONS; i++)
    {
        if (!s[i].vs_Used || s[i].vs_Dead)
            continue;
        for (j = 0; j < i; j++)
        {
            if (s[j].vs_Used && !s[j].vs_Dead && (s[j].vs_Task == s[i].vs_Task))
                break;
        }
        if (j == i)
            n++;
    }
    ReleaseSemaphore(&sd->bo_lock);
    return n;
}

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
    v3d_scan.owner = v3d_session_of(sd, rsc->base.screen);
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

/* Hand vcgfx a descriptor under the session's key; it returns the
 * outcome and the commit to wait for in it. */
static void v3d_ovl_set(struct V3DData *sd, struct v3d_ovl *ov,
                        OOP_Object *bmobj, struct vc4gfx_overlay *desc)
{
    struct TagItem ovltags[] =
    {
        { sd->hiddVCGfxBMAB + aoVCGfxBM_Overlay, (IPTR)desc },
        { TAG_DONE, 0 }
    };

    desc->ovl_Client = ov;
    desc->ovl_Result = 0;
    OOP_SetAttrs(bmobj, ovltags);
}

/* Retire a NOWAIT Set; normally its vblank has already passed. */
static void v3d_ovl_latch_wait(struct V3DData *sd, struct v3d_ovl *ov)
{
    struct vc4gfx_overlay desc = { 0 };

    if (!ov->latch_due)
        return;
    if (ov->bm && sd->hiddVCGfxBMAB)
    {
        desc.ovl_Flags = VC4GFX_OVL_LATCHWAIT;
        desc.ovl_Armed = ov->armed;
        v3d_ovl_set(sd, ov, ov->bm, &desc);
    }
    ov->latch_due = FALSE;
}

/* Take the plane down; `onplane` shows until the returned commit latches. */
static void v3d_ovl_clear(struct V3DData *sd, struct v3d_ovl *ov, OOP_Object *bmobj)
{
    struct vc4gfx_overlay desc = { 0 };

    desc.ovl_Flags = VC4GFX_OVL_HIDE;
    v3d_ovl_set(sd, ov, bmobj, &desc);
    ov->armed = desc.ovl_Armed;
}

/* Show a BO on the session's HVS plane at fb coords x,y. A NOWAIT Set
 * returns before the vblank latch. */
static ULONG v3d_show_overlay(struct V3DData *sd, struct v3d_ovl *ov,
                             OOP_Object *bmobj, struct v3d_bo *bo, ULONG stride,
                             LONG x, LONG y, ULONG w, ULONG h)
{
    struct vc4gfx_overlay desc = { 0 };
    ULONG paddr = 0;

    if (!bmobj || !sd->hiddVCGfxBMAB || !w || !h)
        return 0;

    ObtainSemaphore(&sd->bo_lock);
    if (bo->handle < V3D_MAX_BOS && sd->bo_table[bo->handle].refcount
        && stride * h <= sd->bo_table[bo->handle].size)
        paddr = sd->bo_table[bo->handle].paddr;
    ReleaseSemaphore(&sd->bo_lock);
    if (!paddr)
        return 0;

    desc.ovl_Phys   = paddr;
    desc.ovl_Pitch  = stride;
    desc.ovl_Width  = w;
    desc.ovl_Height = h;
    desc.ovl_X      = x;
    desc.ovl_Y      = y;
    /* Always 1:1. The field is the Pi 3 scaled-plane request (HVS4
     * implements it, HVS5 does not), and desc is not zeroed, so leaving
     * it out would hand the hidd stack garbage. */
    desc.ovl_DestW  = 0;
    desc.ovl_DestH  = 0;
    desc.ovl_Flags  = V3D_OVL_NOWAIT ? VC4GFX_OVL_NOWAIT : 0;

    v3d_ovl_set(sd, ov, bmobj, &desc);
    if (desc.ovl_Result == 1)
        ov->armed = desc.ovl_Armed;
    return desc.ovl_Result;
}

/* Hide the plane but keep the pages: obscure/reveal cycles must not
 * allocate or free BOs. */
static void v3d_ovl_suspend(struct V3DData *sd, struct v3d_ovl *ov)
{
    /* Clearing over an unlatched Set would leave `retiring` unretired. */
    v3d_ovl_latch_wait(sd, ov);

    if (ov->shown && ov->bm)
    {
        v3d_ovl_clear(sd, ov, ov->bm);
        /* `onplane` shows until the clear latches. Not waited for here
         * (obscure must stay cheap); whoever frees the pages waits. */
        ov->latch_due = TRUE;
    }
    ov->shown = FALSE;

    /* Retire the queued frame: it goes stale while the plane is hidden
     * (the blit path is drawing meanwhile), and showing it on reveal
     * would put one old image on screen. Park it as the spare instead,
     * so the ring refills exactly as it does at entry. */
    if (ov->queued)
    {
        if (ov->freep)
            v3d_bo_unreference(&ov->queued);
        else
            ov->freep = ov->queued;
        ov->queued = NULL;
        ov->queued_seqno = 0;
    }
}

static void v3d_ovl_vblank(APTR data);

static struct vc4gfx_vblank v3d_ovl_vbl;
static BOOL v3d_ovl_vbl_armed;      /* vcgfx may call v3d_ovl_vblank() */

static void v3d_ovl_set_vblank(struct V3DData *sd, OOP_Object *bmobj,
                               struct vc4gfx_vblank *vbl)
{
    struct TagItem vbltags[] =
    {
        { sd->hiddVCGfxBMAB + aoVCGfxBM_VBlank, (IPTR)vbl },
        { TAG_DONE, 0 }
    };

    OOP_SetAttrs(bmobj, vbltags);
    v3d_ovl_vbl_armed = (vbl != NULL);
}

/* The vblank callback is up while any session has an overlay; changing
 * it needs a live bitmap. */
static void v3d_ovl_vbl_update(struct V3DData *sd, OOP_Object *bmobj)
{
    BOOL want = FALSE;
    ULONG i;

    for (i = 0; i < V3D_MAX_SESSIONS; i++)
        want |= (v3d_ovls[i].rsc != NULL);
    if (!bmobj || (want == v3d_ovl_vbl_armed))
        return;

    v3d_ovl_vbl.vbl_Func = v3d_ovl_vblank;
    v3d_ovl_vbl.vbl_Data = sd;
    v3d_ovl_set_vblank(sd, bmobj, want ? &v3d_ovl_vbl : NULL);
}

/* Raw drop, for a session whose Mesa objects are gone: its pages are
 * swept with its BOs, and the plane is left for the next present. So is
 * the vblank callback, as taking it off needs a live bitmap: with nothing
 * queued it does nothing, and v3d_ovl_shutdown() keeps us loaded until a
 * present has taken it off. */
static void v3d_ovl_forget(struct v3d_ovl *ov)
{
    ov->rsc = NULL;
    ov->onplane = NULL;
    ov->queued = NULL;
    ov->freep = NULL;
    ov->retiring = NULL;
    ov->queued_seqno = 0;
    ov->latch_due = FALSE;
    ov->page_handle = 0;
    ov->shown = FALSE;
    ov->refused = NULL;
    ov->bm = NULL;
    ov->stale = TRUE;
}

/* Take down a plane nobody owns any more, through a bitmap that is alive. */
static void v3d_ovl_hide(struct V3DData *sd, struct v3d_ovl *ov, OOP_Object *bmobj)
{
    v3d_ovl_clear(sd, ov, bmobj);
    ov->stale = FALSE;
    v3d_ovl_vbl_update(sd, bmobj);
}

/* The plane is held or the budget used: try entering again only now and
 * then. */
static BOOL v3d_ovl_held(struct v3d_ovl *ov)
{
    if (!ov->busy_skip)
        return FALSE;
    ov->busy_skip--;
    return TRUE;
}

/* Full teardown: resource changed or the session is going away. */
static void v3d_ovl_exit(struct V3DData *sd, struct v3d_ovl *ov)
{
    OOP_Object *bm = ov->bm;

    v3d_ovl_suspend(sd, ov);
    /* The clear must latch before the pages go back to Mesa. */
    v3d_ovl_latch_wait(sd, ov);
    if (ov->onplane)
        v3d_bo_unreference(&ov->onplane);
    if (ov->queued)
        v3d_bo_unreference(&ov->queued);
    if (ov->freep)
        v3d_bo_unreference(&ov->freep);
    if (ov->retiring)
        v3d_bo_unreference(&ov->retiring);
    ov->queued_seqno = 0;
    ov->latch_due = FALSE;
    ov->rsc = NULL;
    ov->page_handle = 0;
    ov->bm = NULL;
    v3d_ovl_vbl_update(sd, bm);
}

/* Record a newly queued frame and where it goes. */
static void v3d_ovl_queued_at(struct v3d_ovl *ov, LONG x, LONG y, ULONG w, ULONG h,
                              ULONG stride)
{
    ov->x = x;
    ov->y = y;
    ov->w = w;
    ov->h = h;
    ov->stride = stride;
    ov->gen++;
}

/* Put the queued frame on, as the next present would have. That present
 * then takes the refill path. Caller holds ovl_lock. */
static void v3d_ovl_flush(struct V3DData *sd, struct v3d_ovl *ov)
{
    struct v3d_bo *off;

    if (!ov->queued || !ov->shown || ov->gen != ov->flush_gen)
        return;

    v3d_hw_wait_seqno(sd, ov->queued_seqno);
    v3d_ovl_latch_wait(sd, ov);
    if (v3d_show_overlay(sd, ov, ov->bm, ov->queued, ov->stride,
                         ov->x, ov->y, ov->w, ov->h) != 1)
        return;

    /* `retiring` latched above: it becomes the spare the refill takes. */
    off = ov->onplane;
    ov->onplane = ov->queued;
    ov->queued = NULL;
    ov->queued_seqno = 0;
    ov->freep = ov->retiring;
    ov->retiring = off;
    ov->latch_due = V3D_OVL_NOWAIT;
    ov->flushed = TRUE;
}

/* Interrupt context, every vblank while an overlay is up: a session with
 * a frame queued and no present for a whole frame gets the task. */
static void v3d_ovl_vblank(APTR data)
{
    struct V3DData *sd = data;
    BOOL wake = FALSE;
    ULONG i;

    for (i = 0; i < V3D_MAX_SESSIONS; i++)
    {
        struct v3d_ovl *ov = &v3d_ovls[i];
        ULONG gen = ov->gen;

        if (gen != ov->vbl_gen)
            ov->vbl_gen = gen;
        else if (gen != ov->flush_gen && ov->queued)
        {
            ov->flush_gen = gen;
            wake = TRUE;
        }
    }
    if (wake && sd->ovl_task)
        Signal(sd->ovl_task, SIGBREAKF_CTRL_F);
}

static void v3d_ovl_task(struct V3DData *sd)
{
    ULONG i;

    for (;;)
    {
        ULONG sigs = Wait(SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F);

        if (sigs & SIGBREAKF_CTRL_C)
            break;
        ObtainSemaphore(&sd->ovl_lock);
        for (i = 0; i < V3D_MAX_SESSIONS; i++)
            v3d_ovl_flush(sd, &v3d_ovls[i]);
        ReleaseSemaphore(&sd->ovl_lock);
    }

    /* Forbid until exit: the stopper may unload this code once woken */
    Forbid();
    sd->ovl_task = NULL;
    if (sd->ovl_waiter)
        Signal(sd->ovl_waiter, SIGF_SINGLE);
}

/* For Expunge. While vcgfx may still call the vblank callback we stay;
 * otherwise the overlay task goes. */
BOOL v3d_ovl_shutdown(struct V3DData *sd)
{
    if (v3d_ovl_vbl_armed)
        return FALSE;
    if (sd->ovl_task)
    {
        sd->ovl_waiter = FindTask(NULL);
        SetSignal(0, SIGF_SINGLE);
        Signal(sd->ovl_task, SIGBREAKF_CTRL_C);
        while (sd->ovl_task)
            Wait(SIGF_SINGLE);
        sd->ovl_waiter = NULL;
    }
    return TRUE;
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

/* Sessions whose task died without destroying its screen: their Mesa
 * objects are gone, so the present state they own is dropped, not
 * released, and then their BOs go. */
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

        ObtainSemaphore(&sd->ovl_lock);
        if (v3d_scan.rsc && (v3d_scan.owner == i + 1))
            v3d_scan_forget();
        if (v3d_ovls[i].rsc)
            v3d_ovl_forget(&v3d_ovls[i]);
        ReleaseSemaphore(&sd->ovl_lock);

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

    /* Only the present state this screen owns: another app keeps its
     * overlay or its scanout pages. */
    ObtainSemaphore(&sd->ovl_lock);
    if (sid && v3d_ovls[sid - 1].rsc)
        v3d_ovl_exit(sd, &v3d_ovls[sid - 1]);
    if (v3d_scan.rsc && (v3d_scan.owner == sid))
        v3d_scan_release();
    ReleaseSemaphore(&sd->ovl_lock);

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
    BOOL fullscreen, bound, windowed, scan_free, multi = FALSE;
    struct v3d_ovl *ov;
    ULONG sid, i;
    BOOL enter_scanout = FALSE, detach_after_blit = FALSE;
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
    ObtainSemaphore(&sd->ovl_lock);
    scr_bm_obj = HIDD_BM_OBJ(rp->BitMap);
    sid = v3d_session_of(sd, rsc->base.screen);
    ov = sid ? &v3d_ovls[sid - 1] : NULL;

    for (i = 0; i < V3D_MAX_SESSIONS; i++)
    {
        if (v3d_ovls[i].stale && !v3d_ovls[i].rsc)
            v3d_ovl_hide(sd, &v3d_ovls[i], scr_bm_obj);
    }

    /* Drop this session's stale state when its bound resource went away
     * (resize). Another session's binding stays: it keeps the plane or
     * the pages, and this one blits. */
    if (v3d_scan.rsc && (v3d_scan.owner == sid) && v3d_scan.rsc != &rsc->base)
        v3d_scan_release();
    if (ov && ov->rsc && ov->rsc != &rsc->base)
        v3d_ovl_exit(sd, ov);
    if (ov && ov->refused && ov->refused != &rsc->base)
        ov->refused = NULL;
    scan_free = !v3d_scan.rsc || (v3d_scan.owner == sid);

    /* Each session has a plane of its own, but the framebuffer pages are
     * one: with several apps, nobody flips. */
    multi = (v3d_live_apps(sd) > 1);
    if (multi)
        scan_free = FALSE;

    fullscreen = (xSrc == 0 && ySrc == 0 && xDest == 0 && yDest == 0 &&
                  L->bounds.MinX == 0 && L->bounds.MinY == 0 &&
                  L->ClipRect && !L->ClipRect->Next && !L->ClipRect->lobs);
    bound = (v3d_scan.rsc == &rsc->base &&
             rsc->bo->handle == v3d_scan.page_handle);

    /* Flip is tried first only when the overlay is not preferred, or
     * when the overlay has already refused this resource - so a display
     * the HVS plane cannot serve still gets a zero-copy path. */
    if (fullscreen && scan_free
        && (!V3D_OVERLAY_ENABLE || !V3D_PREFER_OVERLAY
            || !ov || ov->refused == &rsc->base))
    {
        struct v3d_scanout_info so;
#if V3D_PROFILE
        ULONG t0, t1;
#endif

        if (v3d_get_scanout(sd, scr_bm_obj, &so)
            && (ULONG)xSize == so.width && (ULONG)ySize == so.height
            && stride == so.pitch)
        {
            /* The frame must be complete before it goes on scanout. */
#if V3D_PROFILE
            t0 = V3D_NOW_US();
#endif
            v3d_wait_idle(sd);
#if V3D_PROFILE
            t1 = V3D_NOW_US();
#endif

            if (bound && so.name[0] == v3d_scan.name[0]
                && so.name[1] == v3d_scan.name[1])
            {
                /* Steady state: the frame is already in the back page.
                 * Make it visible and rebind at the new back page. */
                if (v3d_flip_fb(sd, scr_bm_obj)
                    && v3d_scan_bind_back(sd, rsc, scr_bm_obj))
                {
                    static ULONG n;
#if V3D_PROFILE
                    /* emit = app plus Mesa on the CPU since the last
                     * present, render = the wait for this frame's jobs,
                     * vsync = the latch wait. */
                    static ULONG emit_acc, rend_acc, vs_acc, prev, job_acc;
                    ULONG t2 = V3D_NOW_US();

                    if (prev)
                        emit_acc += t0 - prev;
                    rend_acc += t1 - t0;
                    vs_acc   += t2 - t1;
                    prev = t2;
                    job_acc += sd->prof_submits;
                    sd->prof_submits = 0;
#endif
                    /* Twice only, and unconditional: enough to tell from
                     * a serial log that the zero-copy path engaged. */
                    if (++n <= 2)
                        bug("[V3D] present flip #%u: %ldx%ld\n",
                            (unsigned)n, (long)xSize, (long)ySize);
#if V3D_PROFILE
                    if (n % V3D_PROF_PERIOD == 0)
                    {
                        ULONG f = V3D_PROF_PERIOD;

                        V3D_PROF("[V3DProf] flip: %lu frames emit=%luus "
                                 "render-wait=%luus vsync-wait=%luus "
                                 "jobs=%lu.%02lu/f\n",
                                 (unsigned long)f,
                                 (unsigned long)(emit_acc / f),
                                 (unsigned long)(rend_acc / f),
                                 (unsigned long)(vs_acc / f),
                                 (unsigned long)(job_acc / f),
                                 (unsigned long)(job_acc * 100 / f % 100));
                        emit_acc = rend_acc = vs_acc = job_acc = 0;
                    }
#endif
                    ReleaseSemaphore(&sd->ovl_lock);
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
                if (ov && ov->rsc == &rsc->base)
                    v3d_ovl_exit(sd, ov);
                enter_scanout = TRUE;
            }
        }
        else if (bound)
        {
            /* Geometry changed under us. */
            bug("[V3D] present: leaving scanout mode (geometry changed)\n");
            detach_after_blit = TRUE;
        }
        else
        {
            /* Once: missing flip pages are a vcgfx question, a geometry
             * mismatch a mode/drawable one. */
            static BOOL said = FALSE;

            if (!said)
            {
                said = TRUE;
                if (!v3d_get_scanout(sd, scr_bm_obj, &so))
                    bug("[V3D] present: no flip pages on this bitmap "
                        "(scaled or single-buffered mode) - trying the "
                        "overlay\n");
                else
                    bug("[V3D] present: drawable %ldx%ld stride=%u does not "
                        "match framebuffer %ux%u pitch=%u - trying the "
                        "overlay\n",
                        (long)xSize, (long)ySize, (unsigned)stride,
                        (unsigned)so.width, (unsigned)so.height,
                        (unsigned)so.pitch);
            }
        }
    }
    else if (bound)
    {
        bug("[V3D] present: leaving scanout mode (windowed/obscured)\n");
        detach_after_blit = TRUE;
    }

    /*
     * Zero-copy overlay present. Serves a fully visible window, and
     * equally a fullscreen surface the flip path declined - the common
     * case on a non-native mode. Gating it on !fullscreen sent exactly
     * that case to the CPU blit instead.
     */
    if (ov && !bound && !detach_after_blit && !enter_scanout)
    {
        LONG absX = L->bounds.MinX + xDest;
        LONG absY = L->bounds.MinY + yDest;

        windowed = (xSrc == 0 && ySrc == 0 &&
                    xSize == (LONG)rsc->base.width0 &&
                    ySize == (LONG)rsc->base.height0 &&
                    req_w == xSize && req_h == ySize &&
                    L->ClipRect && !L->ClipRect->Next &&
                    !L->ClipRect->lobs);

        if (ov->rsc == &rsc->base)
        {
            if (windowed && rsc->bo->handle == ov->page_handle)
            {
                ULONG shown;
#if V3D_PROFILE
                ULONG t0 = V3D_NOW_US(), t1;
#endif
                if (!ov->queued)
                {
                    /* The ring only empties at entry, after a suspend,
                     * or when v3d_ovl_task put an idle app's frame on.
                     * Counting up otherwise means presents are being
                     * dropped. */
                    static ULONG refills = 0;

                    if (!ov->flushed
                        && (++refills <= 8 || (refills & 63) == 0))
                        bug("[V3D] present: overlay ring refill #%u "
                            "(a present showed nothing new)\n",
                            (unsigned)refills);
                    ov->flushed = FALSE;

                    /* Ring not full yet (the present right after entry,
                     * or after a reveal): queue the frame just rendered,
                     * take the parked page as the next target, and leave
                     * the plane showing what it already has. */
                    ov->queued = rsc->bo;
                    ov->queued_seqno = sd->seqno;
                    v3d_ovl_queued_at(ov, absX, absY, xSize, ySize, stride);
                    rsc->bo = ov->freep;
                    ov->freep = NULL;
                    rsc->slices[0].offset = 0;
                    ov->page_handle = rsc->bo->handle;
                    ReleaseSemaphore(&sd->ovl_lock);
                    UnlockLayerRom(L);
                    return TRUE;
                }

                /* Show the PREVIOUS frame. Its jobs ran while this one
                 * was being built, so this wait is normally a no-op -
                 * that is the whole point of the third page. */
                v3d_hw_wait_seqno(sd, ov->queued_seqno);

                /* Before the next Set, or this waits for its latch
                 * instead; `retiring` is off-screen after this. */
                v3d_ovl_latch_wait(sd, ov);
#if V3D_PROFILE
                t1 = V3D_NOW_US();
#endif
                shown = v3d_show_overlay(sd, ov, scr_bm_obj, ov->queued, stride,
                                         absX, absY, xSize, ySize);
                if (shown == 1)
                {
                    /* Rotate: the page leaving the plane parks in
                     * `retiring`; the one parked last time is rendered. */
                    struct v3d_bo *off = ov->onplane;
                    static ULONG n = 0;

                    /* Twice, at startup, and never again: one serial
                     * line is ~8ms at 115200 - two frames' worth - and
                     * a periodic log turns into a periodic stutter in
                     * anything with time-based animation. */
                    if (++n <= 2)
                        bug("[V3D] present overlay #%u: %ldx%ld at %ld,%ld\n",
                            (unsigned)n, (long)xSize, (long)ySize,
                            (long)absX, (long)absY);
#if V3D_PROFILE
                    /* Same phases as the flip path - the overlay's Set
                     * waits for the latch too, so the numbers compare. */
                    {
                        static ULONG emit_acc, rend_acc, lat_acc, prev,
                                     job_acc;
                        ULONG t2 = V3D_NOW_US();

                        if (prev)
                            emit_acc += t0 - prev;
                        rend_acc += t1 - t0;
                        lat_acc  += t2 - t1;
                        prev = t2;
                        job_acc += sd->prof_submits;
                        sd->prof_submits = 0;

                        if (n % V3D_PROF_PERIOD == 0)
                        {
                            ULONG f = V3D_PROF_PERIOD;

                            V3D_PROF("[V3DProf] overlay: %lu frames "
                                     "emit=%luus render-wait=%luus "
                                     "latch-wait=%luus jobs=%lu.%02lu/f "
                                     "%ldx%ld\n",
                                     (unsigned long)f,
                                     (unsigned long)(emit_acc / f),
                                     (unsigned long)(rend_acc / f),
                                     (unsigned long)(lat_acc / f),
                                     (unsigned long)(job_acc / f),
                                     (unsigned long)(job_acc * 100 / f % 100),
                                     (long)xSize, (long)ySize);
                            emit_acc = rend_acc = lat_acc = job_acc = 0;
                        }
                    }
#endif

                    ov->onplane = ov->queued;
                    ov->queued = rsc->bo;
                    ov->queued_seqno = sd->seqno;
                    v3d_ovl_queued_at(ov, absX, absY, xSize, ySize, stride);
                    ov->shown = TRUE;
                    ov->bm = scr_bm_obj;
                    ov->latch_due = V3D_OVL_NOWAIT;
                    rsc->bo = ov->retiring;
                    ov->retiring = off;
                    rsc->slices[0].offset = 0;
                    ov->page_handle = rsc->bo->handle;
                    ReleaseSemaphore(&sd->ovl_lock);
                    UnlockLayerRom(L);
                    return TRUE;
                }
                if (shown == VC4GFX_OVL_BUSY)
                {
                    /* The planes are taken: blit and keep the
                     * pages, as when obscured. */
                    v3d_ovl_suspend(sd, ov);
                }
                else
                {
                    /* Refused a present that worked before (mode
                     * change): tear down, don't retry for this resource. */
                    bug("[V3D] present: overlay refused mid-session, "
                        "back to blitting\n");
                    v3d_ovl_exit(sd, ov);
                    ov->refused = &rsc->base;
                }
            }
            else if (!windowed)
            {
                /* Obscured: hide the plane, keep the pages, blit. */
                v3d_ovl_suspend(sd, ov);
            }
            else
            {
                /* Mesa replaced the BO under us - bookkeeping void. */
                v3d_ovl_exit(sd, ov);
            }
        }
        else if (V3D_OVERLAY_ENABLE && windowed && !ov->rsc
                 && rsc->slices[0].size && ov->refused != &rsc->base
                 && !v3d_ovl_held(ov))
        {
            /* Three more pages: the next render target, a parked spare,
             * and a latch-free seed for `retiring`. */
            struct v3d_bo *nb = v3d_bo_alloc(v3d_screen(rsc->base.screen),
                                             rsc->slices[0].size,
                                             "overlay-page");
            struct v3d_bo *nb2 = nb
                ? v3d_bo_alloc(v3d_screen(rsc->base.screen),
                               rsc->slices[0].size, "overlay-page")
                : NULL;
            struct v3d_bo *nb3 = nb2
                ? v3d_bo_alloc(v3d_screen(rsc->base.screen),
                               rsc->slices[0].size, "overlay-page")
                : NULL;

            if (nb && nb2 && nb3)
            {
                ULONG shown;

                /* Entry frame only: nothing is queued yet, so this one
                 * has to be waited for before it goes on the plane. */
                v3d_wait_idle(sd);
                shown = v3d_show_overlay(sd, ov, scr_bm_obj, rsc->bo, stride,
                                         absX, absY, xSize, ySize);
                if (shown == 1)
                {
                    bug("[V3D] present: overlay mode %ldx%ld at %ld,%ld\n",
                        (long)xSize, (long)ySize, (long)absX, (long)absY);
                    ov->rsc = &rsc->base;
                    ov->stale = FALSE;
                    ov->onplane = rsc->bo;
                    ov->queued = NULL;
                    ov->queued_seqno = 0;
                    ov->freep = nb2;
                    ov->retiring = nb3;
                    ov->latch_due = FALSE;
                    ov->shown = TRUE;
                    ov->bm = scr_bm_obj;
                    rsc->bo = nb;
                    rsc->slices[0].offset = 0;
                    ov->page_handle = nb->handle;

                    if (!sd->ovl_task)
                        sd->ovl_task = NewCreateTask(TASKTAG_PC, v3d_ovl_task,
                                                     TASKTAG_NAME, "v3d overlay",
                                                     TASKTAG_PRI, 1,
                                                     TASKTAG_ARG1, sd,
                                                     TAG_DONE);
                    v3d_ovl_vbl_update(sd, scr_bm_obj);
                    ReleaseSemaphore(&sd->ovl_lock);
                    UnlockLayerRom(L);
                    return TRUE;
                }
                v3d_bo_unreference(&nb);
                v3d_bo_unreference(&nb2);
                v3d_bo_unreference(&nb3);
                if (shown == VC4GFX_OVL_BUSY)
                {
                    /* The planes are taken: try again later */
                    ov->busy_skip = 30;
                }
                else
                {
                    /* Scaled desktop or no takeover: remember and stop
                     * paying an alloc+refusal every present. */
                    bug("[V3D] present: overlay unavailable, blitting\n");
                    ov->refused = &rsc->base;
                }
            }
            else if (nb)
            {
                /* Not all three pages came back: blit this session. */
                v3d_bo_unreference(&nb);
                if (nb2)
                    v3d_bo_unreference(&nb2);
                bug("[V3D] present: no memory for the overlay ring, "
                    "blitting\n");
                ov->refused = &rsc->base;
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
        ReleaseSemaphore(&sd->ovl_lock);
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

    ReleaseSemaphore(&sd->ovl_lock);
    UnlockLayerRom(L);
    return TRUE;
}

