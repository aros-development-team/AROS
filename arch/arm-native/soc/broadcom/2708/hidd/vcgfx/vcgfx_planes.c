/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Hardware screen composition: every screen its own HVS plane.

    graphics.library hands ShowViewPorts() the screens front to back and
    moves one by setting LeftEdge/TopEdge on its bitmap. Each screen bitmap
    lives in scanout memory, so showing or dragging it only rewrites the
    display list - no blit. Every screen scales to fill the display as its
    own mode would, so the display keeps its timing while screens come and
    go. The fb stays allocated for the firmware's sake and, kept black, is
    the backdrop where no screen covers.
*/

#define DEBUG 0
#include <aros/debug.h>

#define __OOP_NOATTRBASES__

#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <exec/memory.h>
#include <hidd/gfx.h>
#include <oop/oop.h>
#include <string.h>

#include "vcgfx_hidd.h"
#include "vcgfx_hvs5.h"
#include "vcgfx_hvs6.h"

#include LC_LIBDEFS_FILE

/* Off: screens go back to Show() and the software compositor. */
#define VCGFX_PLANES        1

/* Arenas are 2MB aligned and sized, so the remap uses block entries only
 * and the kernel never splits a block into a page table it cannot free. */
#define SCANOUT_BLOCK       (2UL << 20)
#define SCANOUT_ARENA       (32UL << 20)
#define SCANOUT_PAGE        4096UL

/* The HVS5 0xC0000000 bus alias reaches the first 1GB only; HVS6 sees
 * 40 bits, but pixel addresses travel as ULONG to the GL drivers. */
#define SCANOUT_LIMIT       0x40000000ULL

#undef HiddBitMapAttrBase
#undef HiddChunkyBMAttrBase
#define HiddBitMapAttrBase      (xsd->vcsd_attrBases[3])
#define HiddChunkyBMAttrBase    (xsd->vcsd_attrBases[2])

static BOOL scanout_ok(struct VideoCoreGfx_staticdata *xsd);

/* Planes need the HVS taken over and scanout memory the kernel can remap. */
BOOL vcgfx_planes_supported(struct VideoCoreGfx_staticdata *xsd)
{
    BOOL ok = FALSE;

    if (!VCGFX_PLANES)
        return FALSE;
    if (xsd->vcsd_HVSGen == VCGFX_HVS_HVS5)
        ok = vc4_hvs5_planes_ok();
    else if (xsd->vcsd_HVSGen == VCGFX_HVS_HVS6)
        ok = vc4_hvs6_planes_ok();
    else if (xsd->vcsd_HVSGen == VCGFX_HVS_VC4)
        ok = vc4_hvs_planes_ok();
    return ok && scanout_ok(xsd);
}

/* We own the display list. */
static BOOL planes_active(struct VideoCoreGfx_staticdata *xsd)
{
    return (xsd->vcsd_HVSGen == VCGFX_HVS_HVS6) ? xsd->vcsd_HVS6.h6_Active
                                                : xsd->vcsd_HVS.hvs_Active;
}

/* Rebuild the list from vcsd_Plane[]; caller holds the mailbox lock. */
static void planes_commit(struct VideoCoreGfx_staticdata *xsd)
{
    if (xsd->vcsd_HVSGen == VCGFX_HVS_HVS6)
        vc4_hvs6_planes_commit(xsd);
    else if (xsd->vcsd_HVSGen == VCGFX_HVS_HVS5)
        vc4_hvs5_planes_commit(xsd);
    else
        vc4_hvs_planes_commit(xsd);
}

/* A screen of this mode can fill the display without a mode set. */
static BOOL plane_fits(struct VideoCoreGfx_staticdata *xsd, ULONG w, ULONG h)
{
    if (xsd->vcsd_HVSGen == VCGFX_HVS_HVS6)
        return vc4_hvs6_plane_fits(xsd, w, h);
    if (xsd->vcsd_HVSGen == VCGFX_HVS_HVS5)
        return vc4_hvs5_plane_fits(xsd, w, h);
    return vc4_hvs_plane_fits(xsd, w, h);
}

/* A lone screen gets its own timing. Not on VideoCore IV, where the
 * firmware keeps the display mode and a mode set only blanks it. */
static BOOL timing_follows_front(struct VideoCoreGfx_staticdata *xsd)
{
    return xsd->vcsd_HVSGen != VCGFX_HVS_VC4;
}

/* The size every screen plane scales to; FALSE where planes do not scale. */
BOOL vcgfx_planes_out_size(struct VideoCoreGfx_staticdata *xsd, ULONG *w, ULONG *h)
{
    if (xsd->vcsd_HVSGen != VCGFX_HVS_HVS6)
    {
        *w = xsd->vcsd_HVS.hvs_SrcW;
        *h = xsd->vcsd_HVS.hvs_SrcH;
        return TRUE;
    }
    if (!vc4_hvs6_scales(xsd))
        return FALSE;
    vc4_hvs6_out_size(xsd, w, h);
    return TRUE;
}

/* Occlusion, applied front to back: a screen as wide as the display hides
 * everything below its top edge. Returns how far down the screens behind
 * it still show. Not a hardware fact, so shared by every HVS generation. */
LONG vcgfx_plane_occlude(LONG px, LONG py, ULONG pw, ULONG ph,
                         LONG left, LONG top, LONG right, LONG bottom)
{
    if ((px <= left) && (px + (LONG)pw >= right)
        && (py < bottom) && (py + (LONG)ph >= bottom))
        return (py > top) ? py : top;
    return bottom;
}

/*********** Overlays ************************************************/

/* The list builder places every overlay; elsewhere the backend has one
 * plane, driven through vc4_hvs_overlay(). */
static BOOL ovl_pooled(struct VideoCoreGfx_staticdata *xsd)
{
    return xsd->vcsd_PlanesMode;
}

/* What the last commit armed: a vblank count, or a frame on HVS6. */
static ULONG ovl_armed(struct VideoCoreGfx_staticdata *xsd)
{
    return (xsd->vcsd_HVSGen == VCGFX_HVS_HVS6) ? xsd->vcsd_HVS6.h6_OvlFrame
                                                : xsd->vcsd_HVS.hvs_FlipArmed;
}

struct vcgfx_ovl *vcgfx_ovl_find(struct VideoCoreGfx_staticdata *xsd, APTR client)
{
    ULONG i;

    for (i = 0; i < VCGFX_MAX_OVERLAYS; i++)
    {
        if (xsd->vcsd_Ovl[i].ov_Client == client)
            return &xsd->vcsd_Ovl[i];
    }
    return NULL;
}

/* One plane: whoever holds it. */
static struct vcgfx_ovl *ovl_holder(struct VideoCoreGfx_staticdata *xsd)
{
    ULONG i;

    for (i = 0; i < VCGFX_MAX_OVERLAYS; i++)
    {
        if (xsd->vcsd_Ovl[i].ov_Client && xsd->vcsd_Ovl[i].ov_Shown)
            return &xsd->vcsd_Ovl[i];
    }
    return NULL;
}

static void ovl_clear(struct vcgfx_ovl *e)
{
    e->ov_Client = NULL;
    e->ov_BM     = NULL;
    e->ov_Shown  = FALSE;
}

/* Show or move a client's overlay. FALSE = not shown, the client blits. */
BOOL vcgfx_ovl_show(struct VideoCoreGfx_staticdata *xsd, APTR client,
                    OOP_Object *bm, const struct vc4gfx_overlay *desc)
{
    struct vcgfx_ovl *e = vcgfx_ovl_find(xsd, client), *held;
    ULONG armed;
    BOOL shown;

    if (!e && !(e = vcgfx_ovl_find(xsd, NULL)))
        return FALSE;

    if (ovl_pooled(xsd))
    {
        VC4_MBOX_LOCK(xsd);
        e->ov_Client = client;
        e->ov_BM     = bm;
        e->ov_BMData = OOP_INST_DATA(OOP_OCLASS(bm), bm);
        e->ov_Desc   = *desc;
        e->ov_Shown  = FALSE;
        planes_commit(xsd);
        armed = e->ov_Armed = ovl_armed(xsd);
        if (client == VCGFX_OVL_GL)
            xsd->vcsd_OvlGLArmed = armed;
        shown = e->ov_Shown;
        if (!shown)
            ovl_clear(e);
        VC4_MBOX_UNLOCK(xsd);

        if (shown && !(desc->ovl_Flags & VC4GFX_OVL_NOWAIT))
            vc4_hvs_latch_until(xsd, armed);
        return shown;
    }

    /* One plane: a video layer keeps it from GL, and takes it from GL. */
    held = ovl_holder(xsd);
    if (held && (held != e))
    {
        if (client == VCGFX_OVL_GL)
            return FALSE;
        VC4_MBOX_LOCK(xsd);
        ovl_clear(held);
        VC4_MBOX_UNLOCK(xsd);
    }

    shown = vc4_hvs_overlay(xsd, desc, bm);
    VC4_MBOX_LOCK(xsd);
    if (shown)
    {
        e->ov_Client = client;
        e->ov_BM     = bm;
        e->ov_BMData = OOP_INST_DATA(OOP_OCLASS(bm), bm);
        e->ov_Desc   = *desc;
        e->ov_Shown  = TRUE;
    }
    else if (e->ov_Client)
        ovl_clear(e);
    VC4_MBOX_UNLOCK(xsd);

    /* Whatever a refused video layer left on the plane goes */
    if (!shown && (client != VCGFX_OVL_GL))
        vc4_hvs_overlay(xsd, NULL, NULL);
    return shown;
}

void vcgfx_ovl_hide(struct VideoCoreGfx_staticdata *xsd, APTR client)
{
    struct vcgfx_ovl *e = vcgfx_ovl_find(xsd, client), *held;
    BOOL was;

    if (ovl_pooled(xsd))
    {
        if (!e)
            return;
        VC4_MBOX_LOCK(xsd);
        was = e->ov_Shown;
        ovl_clear(e);
        if (was)
            planes_commit(xsd);
        /* Its pages may be on the list a screen change just replaced */
        if (client == VCGFX_OVL_GL)
            xsd->vcsd_OvlGLArmed = ovl_armed(xsd);
        VC4_MBOX_UNLOCK(xsd);
        return;
    }

    /* Not while a video layer holds it, but GL may have left a refused
     * overlay with the backend. */
    held = ovl_holder(xsd);
    if (!held || (held == e))
        vc4_hvs_overlay(xsd, NULL, NULL);
    if (e)
    {
        VC4_MBOX_LOCK(xsd);
        ovl_clear(e);
        VC4_MBOX_UNLOCK(xsd);
    }
}

/* 1 = shown. VC4GFX_OVL_BUSY = video layers hold the one plane, or the
 * budget; GL tries again later rather than giving up for good. */
ULONG vcgfx_ovl_state(struct VideoCoreGfx_staticdata *xsd, APTR client)
{
    struct vcgfx_ovl *e = vcgfx_ovl_find(xsd, client), *held;

    if (e && e->ov_Shown)
        return 1;
    held = ovl_holder(xsd);
    if (held && (held->ov_Client != VCGFX_OVL_GL))
        return VC4GFX_OVL_BUSY;
    return 0;
}

/* Until the client's last change is on display. With a list per change,
 * not until whatever another client did since. */
void vcgfx_ovl_latch_wait(struct VideoCoreGfx_staticdata *xsd, APTR client)
{
    struct vcgfx_ovl *e;

    if (!ovl_pooled(xsd))
        vc4_hvs_latch_wait(xsd);
    else if (client == VCGFX_OVL_GL)
        vc4_hvs_latch_until(xsd, xsd->vcsd_OvlGLArmed);
    else if ((e = vcgfx_ovl_find(xsd, client)) && e->ov_Shown)
        vc4_hvs_latch_until(xsd, e->ov_Armed);
}

/* A bitmap is going away: no overlay may refer to it. */
void vcgfx_ovl_drop_bm(struct VideoCoreGfx_staticdata *xsd, OOP_Object *bm)
{
    ULONG i;

    VC4_MBOX_LOCK(xsd);
    for (i = 0; i < VCGFX_MAX_OVERLAYS; i++)
    {
        if (xsd->vcsd_Ovl[i].ov_BM == bm)
        {
            xsd->vcsd_Ovl[i].ov_BM    = NULL;
            xsd->vcsd_Ovl[i].ov_Shown = FALSE;
        }
    }
    VC4_MBOX_UNLOCK(xsd);
}

/*********** Scanout memory ******************************************/

static void arena_free(struct vcgfx_arena *a)
{
    /* Back to cacheable for the next owner. */
    KrnMapGlobal(a->ar_Base, a->ar_Base, a->ar_Size, MAP_Readable | MAP_Writable);
    FreeMem(a->ar_Raw, a->ar_RawSize);
    FreeMem(a, sizeof(struct vcgfx_arena));
}

static struct vcgfx_arena *arena_new(ULONG want)
{
    struct vcgfx_arena *a;
    struct MemChunk *mc;
    IPTR base;

    want = (want + SCANOUT_BLOCK - 1) & ~(SCANOUT_BLOCK - 1);
    if (want < SCANOUT_ARENA)
        want = SCANOUT_ARENA;

    if (!(a = AllocMem(sizeof(struct vcgfx_arena), MEMF_ANY | MEMF_CLEAR)))
        return NULL;

    a->ar_RawSize = want + SCANOUT_BLOCK;
    if (!(a->ar_Raw = AllocMem(a->ar_RawSize, MEMF_ANY)))
    {
        FreeMem(a, sizeof(struct vcgfx_arena));
        return NULL;
    }

    base = ((IPTR)a->ar_Raw + SCANOUT_BLOCK - 1) & ~(IPTR)(SCANOUT_BLOCK - 1);

    /* Clean first, or a dirty line of the previous owner lands on the
     * pixels once the range is non-cacheable. */
    CacheClearE((APTR)base, want, CACRF_ClearD);

    if (((UQUAD)base + want > SCANOUT_LIMIT)
        || !KrnMapGlobal((APTR)base, (APTR)base, want,
                         MAP_Readable | MAP_Writable | MAP_WriteThrough))
    {
        bug("[VideoCoreGfx] no scanout arena at 0x%p: above 1GB or not mappable\n",
            (void *)base);
        FreeMem(a->ar_Raw, a->ar_RawSize);
        FreeMem(a, sizeof(struct vcgfx_arena));
        return NULL;
    }

    a->ar_Base = (APTR)base;
    a->ar_Size = want;

    mc = (struct MemChunk *)base;
    mc->mc_Next  = NULL;
    mc->mc_Bytes = want;
    a->ar_MH.mh_Node.ln_Type = NT_MEMORY;
    a->ar_MH.mh_First = mc;
    a->ar_MH.mh_Lower = (APTR)base;
    a->ar_MH.mh_Upper = (APTR)(base + want);
    a->ar_MH.mh_Free  = want;

    return a;
}

/* Probed once by taking the first arena, which then serves the first
 * screens. The arm32 kernel's KrnMapGlobal() is still a stub that maps
 * nothing, so there the screens stay on Show() until it is implemented. */
static BOOL scanout_ok(struct VideoCoreGfx_staticdata *xsd)
{
    struct vcgfx_arena *a;

    if (!xsd->vcsd_ScanoutOK)
    {
        ObtainSemaphore(&xsd->vcsd_ScanoutLock);
        if (!xsd->vcsd_ScanoutOK)
        {
            if ((a = arena_new(SCANOUT_ARENA)))
            {
                AddTail((struct List *)&xsd->vcsd_Arenas, (struct Node *)a);
                xsd->vcsd_ScanoutOK = 1;
            }
            else
                xsd->vcsd_ScanoutOK = -1;
        }
        ReleaseSemaphore(&xsd->vcsd_ScanoutLock);
    }
    return xsd->vcsd_ScanoutOK > 0;
}

/* Page aligned, cleared, physically contiguous and below SCANOUT_LIMIT. */
BOOL vcgfx_scanout_alloc(struct VideoCoreGfx_staticdata *xsd,
                         struct vcgfx_scanbuf *sb, ULONG size)
{
    struct vcgfx_arena *a;
    ULONG rawsize = size + SCANOUT_PAGE;
    APTR raw = NULL;

    ObtainSemaphore(&xsd->vcsd_ScanoutLock);
    ForeachNode(&xsd->vcsd_Arenas, a)
    {
        if ((raw = Allocate(&a->ar_MH, rawsize)))
            break;
    }
    if (!raw && (a = arena_new(rawsize)))
    {
        AddTail((struct List *)&xsd->vcsd_Arenas, (struct Node *)a);
        raw = Allocate(&a->ar_MH, rawsize);
    }
    ReleaseSemaphore(&xsd->vcsd_ScanoutLock);

    if (!raw)
        return FALSE;

    sb->sb_Raw     = raw;
    sb->sb_RawSize = rawsize;
    sb->sb_Pixels  = (UBYTE *)(((IPTR)raw + SCANOUT_PAGE - 1) & ~(IPTR)(SCANOUT_PAGE - 1));
    memset(sb->sb_Pixels, 0, size);
    return TRUE;
}

void vcgfx_scanout_free(struct VideoCoreGfx_staticdata *xsd,
                        struct vcgfx_scanbuf *sb)
{
    struct vcgfx_arena *a;

    if (!sb->sb_Raw)
        return;

    ObtainSemaphore(&xsd->vcsd_ScanoutLock);
    ForeachNode(&xsd->vcsd_Arenas, a)
    {
        if ((sb->sb_Raw >= a->ar_MH.mh_Lower) && (sb->sb_Raw < a->ar_MH.mh_Upper))
        {
            Deallocate(&a->ar_MH, sb->sb_Raw, sb->sb_RawSize);
            if (a->ar_MH.mh_Free == a->ar_Size)
            {
                Remove((struct Node *)a);
                arena_free(a);
            }
            break;
        }
    }
    ReleaseSemaphore(&xsd->vcsd_ScanoutLock);

    sb->sb_Raw    = NULL;
    sb->sb_Pixels = NULL;
}

/*********** Planes **************************************************/

static struct vcgfx_plane *plane_find(struct VideoCoreGfx_staticdata *xsd,
                                      OOP_Object *bm)
{
    ULONG i;

    for (i = 0; i < xsd->vcsd_NumPlanes; i++)
    {
        if (xsd->vcsd_Plane[i].pl_BM == bm)
            return &xsd->vcsd_Plane[i];
    }
    return NULL;
}

static void plane_pos(struct VideoCoreGfx_staticdata *xsd, OOP_Object *bm,
                      LONG *x, LONG *y)
{
    IPTR l = 0, t = 0;

    OOP_GetAttr(bm, aHidd_BitMap_LeftEdge, &l);
    OOP_GetAttr(bm, aHidd_BitMap_TopEdge, &t);
    *x = (LONG)l;
    *y = (LONG)t;
}

/* The backdrop is the fb; a mode set leaves it holding anything. */
static void backdrop_clear(struct VideoCoreGfx_staticdata *xsd)
{
    struct BitmapData *fb;

    if (!xsd->vcsd_FBObj)
        return;

    fb = OOP_INST_DATA(xsd->vcsd_VideoCoreGfxOnBMClass, xsd->vcsd_FBObj);
    if (fb->VideoData && (fb->VideoData != xsd->vcsd_Backdrop))
    {
        memset(fb->VideoData, 0, fb->bytesperrow * fb->height);
        xsd->vcsd_Backdrop = fb->VideoData;
    }
}

#define PLANES_WARN_BUDGET  (1 << 0)
#define PLANES_WARN_CLASS   (1 << 1)
#define PLANES_WARN_SCANOUT (1 << 2)

/* A screen that gets no plane is not shown at all: say so, once a reason. */
static void planes_warn(struct VideoCoreGfx_staticdata *xsd, ULONG why,
                        const char *msg)
{
    if (xsd->vcsd_PlanesWarned & why)
        return;
    xsd->vcsd_PlanesWarned |= why;
    bug("[VideoCoreGfx] %s\n", msg);
}

/* Mode policy. A mode set blanks the display, so the timing follows the
 * front screen only when it is alone, or too big to scale down into it. */
static void planes_set_timing(struct VideoCoreGfx_staticdata *xsd,
                              struct vcgfx_plane *front, ULONG n)
{
    IPTR mode = vHidd_ModeID_Invalid, fbmode = vHidd_ModeID_Invalid;

    if (!xsd->vcsd_FBObj
        || (!((n == 1) && timing_follows_front(xsd))
            && plane_fits(xsd, front->pl_Data->modew, front->pl_Data->modeh)))
        return;

    OOP_GetAttr(front->pl_BM, aHidd_BitMap_ModeID, &mode);
    OOP_GetAttr(xsd->vcsd_FBObj, aHidd_BitMap_ModeID, &fbmode);
    if ((mode != vHidd_ModeID_Invalid) && (mode != fbmode))
    {
        OOP_SetAttrsTags(xsd->vcsd_FBObj, aHidd_BitMap_ModeID, mode, TAG_DONE);
        xsd->vcsd_Backdrop = NULL;
    }
}

ULONG vcgfx_show_viewports(struct VideoCoreGfx_staticdata *xsd,
                           struct HIDD_ViewPortData *vpd)
{
    OOP_Class *offcl = xsd->vcsd_VideoCoreGfxOffBMClass;
    struct vcgfx_plane shown[VCGFX_MAX_PLANES];
    struct HIDD_ViewPortData *v;
    ULONG n = 0, i;

    if (!vcgfx_planes_supported(xsd))
        return FALSE;

    /* Decided at the first call, either way: graphics.library never takes
     * a framebuffer back out of a BitMap that Show() put it in. */
    if (!xsd->vcsd_PlanesMode
        && (xsd->vcsd_PlanesRefused || !planes_active(xsd)))
    {
        if (!xsd->vcsd_PlanesRefused)
            bug("[VideoCoreGfx] no display list of our own: screens stay on Show()\n");
        xsd->vcsd_PlanesRefused = TRUE;
        return FALSE;
    }

    for (v = vpd; v; v = v->Next)
    {
        struct BitmapData *data;

        if (n == VCGFX_MAX_PLANES)
        {
            planes_warn(xsd, PLANES_WARN_BUDGET,
                        "more screens than planes: those behind are not shown");
            break;
        }
        if (OOP_OCLASS(v->Bitmap) != offcl)
        {
            planes_warn(xsd, PLANES_WARN_CLASS,
                        "a screen of another bitmap class is not shown");
            continue;
        }
        data = OOP_INST_DATA(offcl, v->Bitmap);
        if (!data->VideoData || !data->scan[0].sb_Pixels)
        {
            planes_warn(xsd, PLANES_WARN_SCANOUT,
                        "a screen that is not 32-bit or not in scanout memory is not shown");
            continue;
        }

        shown[n].pl_BM      = v->Bitmap;
        shown[n].pl_Data    = data;
        shown[n].pl_PtrWord = 0;
        plane_pos(xsd, v->Bitmap, &shown[n].pl_X, &shown[n].pl_Y);
        n++;
    }

    if (n)
        planes_set_timing(xsd, &shown[0], n);

    VC4_MBOX_LOCK(xsd);
    if (!xsd->vcsd_PlanesMode)
    {
        bug("[VideoCoreGfx] screens are HVS planes from now on\n");
        xsd->vcsd_PlanesMode = TRUE;
    }
    for (i = 0; i < n; i++)
        xsd->vcsd_Plane[i] = shown[n - 1 - i];
    xsd->vcsd_NumPlanes = n;
    backdrop_clear(xsd);
    planes_commit(xsd);
    VC4_MBOX_UNLOCK(xsd);

    return TRUE;
}

/* LeftEdge/TopEdge changed: the bitmap has clamped and stored them. */
void vcgfx_plane_moved(struct VideoCoreGfx_staticdata *xsd, OOP_Object *bm)
{
    struct vcgfx_plane *pl;
    LONG x, y;

    if (!xsd->vcsd_PlanesMode)
        return;

    VC4_MBOX_LOCK(xsd);
    if ((pl = plane_find(xsd, bm)))
    {
        plane_pos(xsd, bm, &x, &y);
        if ((x != pl->pl_X) || (y != pl->pl_Y))
        {
            pl->pl_X = x;
            pl->pl_Y = y;
            planes_commit(xsd);
        }
    }
    VC4_MBOX_UNLOCK(xsd);
}

/* A bitmap going away must be off the display, and out of the list the
 * HVS is reading, before its pages are freed. That list may also be one
 * ShowViewPorts() already replaced, so always wait for the last commit. */
void vcgfx_plane_forget(struct VideoCoreGfx_staticdata *xsd, OOP_Object *bm)
{
    struct vcgfx_plane *pl;
    BOOL gone = FALSE;

    if (!xsd->vcsd_PlanesMode)
        return;

    VC4_MBOX_LOCK(xsd);
    if ((pl = plane_find(xsd, bm)))
    {
        ULONG i = pl - xsd->vcsd_Plane;

        for (; i + 1 < xsd->vcsd_NumPlanes; i++)
            xsd->vcsd_Plane[i] = xsd->vcsd_Plane[i + 1];
        xsd->vcsd_NumPlanes--;
        gone = TRUE;
    }
    if (gone)
        planes_commit(xsd);
    VC4_MBOX_UNLOCK(xsd);

    vc4_hvs_latch_wait(xsd);
}

/* The page a GL client renders the next frame into. Allocated on first
 * use: most screens never flip. */
ULONG vcgfx_plane_backpage(struct VideoCoreGfx_staticdata *xsd,
                           struct BitmapData *data)
{
    ULONG back = 0;

    if (!data->scan[0].sb_Pixels)
        return 0;

    ObtainSemaphore(&xsd->vcsd_ScanoutLock);
    if (data->scan[1].sb_Pixels
        || vcgfx_scanout_alloc(xsd, &data->scan[1], data->bytesperrow * data->height))
        back = (ULONG)(IPTR)data->scan[data->front ^ 1].sb_Pixels;
    ReleaseSemaphore(&xsd->vcsd_ScanoutLock);

    return back;
}

/* Swap the pages and show the new front from the next frame on. Returns
 * once the old one is off the display. */
BOOL vcgfx_plane_flip(OOP_Class *cl, OOP_Object *bm)
{
    struct VideoCoreGfx_staticdata *xsd = XSD(cl);
    struct BitmapData *data = OOP_INST_DATA(cl, bm);
    struct TagItem tags[] =
    {
        { aHidd_ChunkyBM_Buffer, 0 },
        { TAG_DONE,              0 }
    };
    struct pRoot_Set setmsg;
    struct vcgfx_plane *pl;
    BOOL shown;

    if (!data->scan[1].sb_Pixels)
        return FALSE;

    VC4_MBOX_LOCK(xsd);
    data->front ^= 1;
    data->VideoData = data->scan[data->front].sb_Pixels;

    tags[0].ti_Data = (IPTR)data->VideoData;
    setmsg.mID      = OOP_GetMethodID(IID_Root, moRoot_Set);
    setmsg.attrList = tags;
    OOP_DoSuperMethod(cl, bm, (OOP_Msg)&setmsg);

    pl = plane_find(xsd, bm);
    shown = (pl != NULL);
    if (shown && !((xsd->vcsd_HVSGen == VCGFX_HVS_VC4) && vc4_hvs_plane_flip(xsd, pl)))
        planes_commit(xsd);
    VC4_MBOX_UNLOCK(xsd);

    if (shown)
        vc4_hvs_latch_wait(xsd);
    return TRUE;
}
