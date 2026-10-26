/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Scanout without a framebuffer: HVS planes.

    The display compositor hands Show() the bitmap to scan out - the front
    screen or the one it composed - and pans one by setting LeftEdge/TopEdge
    on it. Screen bitmaps live in scanout memory, so showing or panning one
    only rewrites the display list - no blit. A plane scales the bitmap to
    fill the display as its own mode would. The fb stays allocated for the
    firmware's sake and the display timing and, kept black, is the backdrop.
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
#undef HiddPixFmtAttrBase
#define HiddBitMapAttrBase      (xsd->vcsd_attrBases[3])
#define HiddChunkyBMAttrBase    (xsd->vcsd_attrBases[2])
#define HiddPixFmtAttrBase      (xsd->vcsd_attrBases[4])

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

/* How a plane reads its bitmap: RGB of either width, or YUV. */
static void plane_format(struct vcgfx_plane *pl, struct BitmapData *data)
{
    pl->pl_Pxf = vcgfx_px_pxf(data->pxflags);
    pl->pl_Bpp = vcgfx_px_bpp(data->pxflags);
    pl->pl_Yuv = 0;
    if (data->pxflags & VCGFX_PX_BGRA)
        pl->pl_Pxf = HVS6_CTL0_FMT_8888;    /* HVS6 entries then take B,G,R,A */
    if (data->videofmt)
    {
        pl->pl_Yuv = (data->videofmt == vHidd_VideoFmt_NV12) ? HVS_PXF_YUV420_2PLANE
                                                             : HVS_PXF_YUV420_3PLANE;
        pl->pl_Bpp = 1;
    }
}

/* NV12 and I420: through the scaler, so on HVS6 only where it scales. */
BOOL vcgfx_yuv_ok(struct VideoCoreGfx_staticdata *xsd, ULONG videofmt)
{
    return ((videofmt == vHidd_VideoFmt_NV12) || (videofmt == vHidd_VideoFmt_I420))
        && ((xsd->vcsd_HVSGen != VCGFX_HVS_HVS6) || vc4_hvs6_scales(xsd))
        && vcgfx_planes_supported(xsd);
}

/* A 10-bit S2.8 coefficient: 0 to 2.996, or -1 to just under 0 */
#define CSC_COEF(v)     ((ULONG)((v) < 0 ? 1024 + (v) : (v)) & 0x3ff)

/* The three CSC words of a YUV entry: limited range BT.601 or BT.709, in
 * 1/256. Cb and Cr come centred already; Y loses its 16. */
void vcgfx_yuv_csc(ULONG colorspace, ULONG csc[3])
{
    LONG y = 298, crr, cbg, crg, cbb;

    if (colorspace == vHidd_ColorSpace_BT709)
    {
        crr = 459;
        cbg = -55;
        crg = -136;
        cbb = 541;
    }
    else
    {
        crr = 409;
        cbg = -100;
        crg = -208;
        cbb = 517;
    }
    csc[0] = (ULONG)(-16 & 0xff) << 16;
    csc[1] = (CSC_COEF(cbg) << 22) | (CSC_COEF(crg) << 12) | (CSC_COEF(y) << 2);
    csc[2] = (CSC_COEF(crr) << 10) | CSC_COEF(cbb);
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

/* Mode policy. A mode set blanks the display, so the timing follows the
 * screen shown only where a plane can't scale it to fill the display, or
 * exactly when it is copied into the fb. */
static void planes_set_timing(struct VideoCoreGfx_staticdata *xsd,
                              struct BitmapData *data, IPTR mode, BOOL exact)
{
    IPTR fbmode = vHidd_ModeID_Invalid;

    if ((mode == vHidd_ModeID_Invalid)
        || (!exact && !timing_follows_front(xsd) && plane_fits(xsd, data->modew, data->modeh)))
        return;

    OOP_GetAttr(xsd->vcsd_FBObj, aHidd_BitMap_ModeID, &fbmode);
    if (mode != fbmode)
    {
        OOP_SetAttrsTags(xsd->vcsd_FBObj, aHidd_BitMap_ModeID, mode, TAG_DONE);
        xsd->vcsd_Backdrop = NULL;
    }
}

/* Copy a rectangle of the mirrored bitmap into the fb; caller holds vcsd_ShowLock. */
static void mirror_copy(struct VideoCoreGfx_staticdata *xsd, OOP_Object *bm,
                        LONG x, LONG y, LONG w, LONG h)
{
    struct BitmapData *fb = OOP_INST_DATA(xsd->vcsd_VideoCoreGfxOnBMClass, xsd->vcsd_FBObj);
    OOP_Object *pf = NULL;
    IPTR bw = 0, bh = 0, stdpf = 0;

    OOP_GetAttr(bm, aHidd_BitMap_Width, &bw);
    OOP_GetAttr(bm, aHidd_BitMap_Height, &bh);
    if (bw > fb->dispwidth)
        bw = fb->dispwidth;
    if (bh > fb->height)
        bh = fb->height;
    if (x < 0)
    {
        w += x;
        x = 0;
    }
    if (y < 0)
    {
        h += y;
        y = 0;
    }
    if (x + w > (LONG)bw)
        w = (LONG)bw - x;
    if (y + h > (LONG)bh)
        h = (LONG)bh - y;
    if ((w <= 0) || (h <= 0) || !fb->VideoData)
        return;

    OOP_GetAttr(xsd->vcsd_FBObj, aHidd_BitMap_PixFmt, (IPTR *)&pf);
    OOP_GetAttr(pf, aHidd_PixFmt_StdPixFmt, &stdpf);
    HIDD_BM_GetImage(bm, fb->VideoData + y * fb->bytesperrow + x * fb->bytesperpix,
                     fb->bytesperrow, x, y, w, h, stdpf);
}

/* vcsd_Plane[] for the list builders: the primary, then the overlay planes
 * by z. Caller holds the mailbox lock. */
static void planes_stack(struct VideoCoreGfx_staticdata *xsd)
{
    ULONG n = 0, i;

    if (xsd->vcsd_Primary.pl_BM)
        xsd->vcsd_Plane[n++] = xsd->vcsd_Primary;
    for (i = 0; i < VCGFX_OVL_PLANES; i++)
    {
        if (xsd->vcsd_OvlPlane[i].pl_BM)
            xsd->vcsd_Plane[n++] = xsd->vcsd_OvlPlane[i];
    }
    xsd->vcsd_NumPlanes = n;
}

/* A list built into a slot nothing reads, for pl_Shown. */
static void planes_try(struct VideoCoreGfx_staticdata *xsd)
{
    if (xsd->vcsd_HVSGen == VCGFX_HVS_HVS6)
        vc4_hvs6_planes_try(xsd);
    else if (xsd->vcsd_HVSGen == VCGFX_HVS_HVS5)
        vc4_hvs5_planes_try(xsd);
    else
        vc4_hvs_planes_try(xsd);
}

/*
 * Show() without a framebuffer: the bitmap given is the primary plane, read
 * from its own pages and panned by its LeftEdge/TopEdge, its mode the
 * display's pixels. One the HVS can't show - not ours, not in scanout
 * memory, or over the budget - is copied into the fb behind instead, and
 * kept up to date by vcgfx_mirror_rect(). The overlay planes stay.
 */
void vcgfx_show(struct VideoCoreGfx_staticdata *xsd, OOP_Object *bm)
{
    OOP_Class *offcl = xsd->vcsd_VideoCoreGfxOffBMClass;
    struct vcgfx_plane *pl = &xsd->vcsd_Primary;
    struct BitmapData *data = NULL;
    IPTR mode = vHidd_ModeID_Invalid;
    BOOL shown = FALSE;

    if (bm)
    {
        OOP_GetAttr(bm, aHidd_BitMap_ModeID, &mode);
        if (OOP_OCLASS(bm) == offcl)
            data = OOP_INST_DATA(offcl, bm);
        if (data && !data->scan[0].sb_Pixels)
            data = NULL;
    }

    ObtainSemaphore(&xsd->vcsd_ShowLock);
    if (data && planes_active(xsd))
        planes_set_timing(xsd, data, mode, FALSE);

    VC4_MBOX_LOCK(xsd);
    xsd->vcsd_PlanesMode = planes_active(xsd);
    xsd->vcsd_Mirror     = NULL;
    pl->pl_BM            = NULL;
    if (data && xsd->vcsd_PlanesMode)
    {
        pl->pl_BM      = bm;
        pl->pl_Data    = data;
        pl->pl_PtrWord = 0;
        pl->pl_Shown   = TRUE;
        pl->pl_SrcX    = 0;
        pl->pl_SrcY    = 0;
        pl->pl_SrcW    = pl->pl_W = data->width;
        pl->pl_SrcH    = pl->pl_H = data->height;
        plane_format(pl, data);
        plane_pos(xsd, bm, &pl->pl_X, &pl->pl_Y);
        xsd->vcsd_DispW = data->modew;
        xsd->vcsd_DispH = data->modeh;
    }
    backdrop_clear(xsd);
    planes_stack(xsd);
    planes_commit(xsd);

    /* Left out of the list for the overlays in front: nothing of it is on display */
    if (pl->pl_BM)
    {
        shown = xsd->vcsd_Plane[0].pl_Shown;
        if (!shown)
        {
            pl->pl_BM = NULL;
            planes_stack(xsd);
        }
    }
    VC4_MBOX_UNLOCK(xsd);

    if (bm && !shown)
    {
        struct BitmapData *fb;

        planes_set_timing(xsd, NULL, mode, TRUE);
        fb = OOP_INST_DATA(xsd->vcsd_VideoCoreGfxOnBMClass, xsd->vcsd_FBObj);
        xsd->vcsd_DispW = fb->dispwidth;
        xsd->vcsd_DispH = fb->height;
        backdrop_clear(xsd);
        xsd->vcsd_Mirror   = bm;
        xsd->vcsd_Backdrop = NULL;
        mirror_copy(xsd, bm, 0, 0, 0x7fff, 0x7fff);
    }
    ReleaseSemaphore(&xsd->vcsd_ShowLock);
}

/* A bitmap was drawn to: the mirrored one passes the change to the fb. */
void vcgfx_mirror_rect(struct VideoCoreGfx_staticdata *xsd, OOP_Object *bm,
                       LONG x, LONG y, LONG w, LONG h)
{
    if (bm != xsd->vcsd_Mirror)
        return;

    ObtainSemaphore(&xsd->vcsd_ShowLock);
    if (bm == xsd->vcsd_Mirror)
        mirror_copy(xsd, bm, x, y, w, h);
    ReleaseSemaphore(&xsd->vcsd_ShowLock);
}

/*
 * CommitPlanes(): the overlay planes, taken as a whole set or not at all.
 * A set fits when the list built from it has room for every plane, the
 * primary too; that list goes to a slot nothing reads to find out. A
 * test only says.
 */
BOOL vcgfx_planes_commit(struct VideoCoreGfx_staticdata *xsd,
                         struct HIDD_PlaneState *states, ULONG count, ULONG flags)
{
    OOP_Class *offcl = xsd->vcsd_VideoCoreGfxOffBMClass;
    struct vcgfx_plane want[VCGFX_OVL_PLANES], keep[VCGFX_OVL_PLANES];
    struct vcgfx_plane built[VCGFX_MAX_PLANES];
    ULONG nbuilt, i;
    BOOL ok = TRUE;

    VC4_MBOX_LOCK(xsd);
    if (!xsd->vcsd_PlanesMode)
    {
        VC4_MBOX_UNLOCK(xsd);
        return FALSE;
    }
    CopyMem(xsd->vcsd_OvlPlane, want, sizeof(want));
    CopyMem(xsd->vcsd_OvlPlane, keep, sizeof(keep));

    for (i = 0; ok && (i < count); i++)
    {
        const struct HIDD_PlaneState *st = &states[i];
        const struct Rectangle *s = &st->Src, *d = &st->Dest;
        struct BitmapData *data;
        struct vcgfx_plane *pl;

        if ((st->Plane < 1) || (st->Plane > VCGFX_OVL_PLANES))
        {
            ok = FALSE;
            break;
        }
        pl = &want[st->Plane - 1];
        pl->pl_BM = NULL;
        if (!st->BitMap)
            continue;

        data = (OOP_OCLASS(st->BitMap) == offcl) ? OOP_INST_DATA(offcl, st->BitMap) : NULL;
        if (!data || !data->scan[0].sb_Pixels || (st->AlphaMode != vHidd_CompAlpha_None)
            || (st->Flags & PSF_COLORKEY)
            || (s->MinX < 0) || (s->MinY < 0) || (s->MaxX < s->MinX) || (s->MaxY < s->MinY)
            || (s->MaxX >= (WORD)data->width) || (s->MaxY >= (WORD)data->height)
            || (d->MaxX < d->MinX) || (d->MaxY < d->MinY))
        {
            ok = FALSE;
            break;
        }
        pl->pl_BM      = st->BitMap;
        pl->pl_Data    = data;
        pl->pl_X       = d->MinX;
        pl->pl_Y       = d->MinY;
        pl->pl_W       = d->MaxX - d->MinX + 1;
        pl->pl_H       = d->MaxY - d->MinY + 1;
        pl->pl_SrcX    = s->MinX;
        pl->pl_SrcY    = s->MinY;
        pl->pl_SrcW    = s->MaxX - s->MinX + 1;
        pl->pl_SrcH    = s->MaxY - s->MinY + 1;
        plane_format(pl, data);
        pl->pl_PtrWord = 0;
        pl->pl_Shown   = TRUE;
    }

    if (ok)
    {
        CopyMem(xsd->vcsd_Plane, built, sizeof(built));
        nbuilt = xsd->vcsd_NumPlanes;

        CopyMem(want, xsd->vcsd_OvlPlane, sizeof(want));
        planes_stack(xsd);
        planes_try(xsd);
        for (i = 0; i < xsd->vcsd_NumPlanes; i++)
        {
            if (!xsd->vcsd_Plane[i].pl_Shown)
                ok = FALSE;
        }

        CopyMem(built, xsd->vcsd_Plane, sizeof(built));
        xsd->vcsd_NumPlanes = nbuilt;
    }

    if (!ok || (flags & fHidd_Commit_Test))
        CopyMem(keep, xsd->vcsd_OvlPlane, sizeof(keep));
    else
    {
        planes_stack(xsd);
        planes_commit(xsd);
    }
    VC4_MBOX_UNLOCK(xsd);

    if (ok && !(flags & (fHidd_Commit_Test | fHidd_Commit_NoWait)))
        vc4_hvs_latch_wait(xsd);
    return ok;
}

/* LeftEdge/TopEdge changed: the bitmap has clamped and stored them. Only
 * the primary pans; the compositor places the overlay planes. */
void vcgfx_plane_moved(struct VideoCoreGfx_staticdata *xsd, OOP_Object *bm)
{
    struct vcgfx_plane *pl = &xsd->vcsd_Primary;
    LONG x, y;

    if (!xsd->vcsd_PlanesMode)
        return;

    VC4_MBOX_LOCK(xsd);
    if (pl->pl_BM == bm)
    {
        plane_pos(xsd, bm, &x, &y);
        if ((x != pl->pl_X) || (y != pl->pl_Y))
        {
            pl->pl_X = x;
            pl->pl_Y = y;
            planes_stack(xsd);
            planes_commit(xsd);
        }
    }
    VC4_MBOX_UNLOCK(xsd);
}

/* A bitmap going away must be off the display, and out of the list the
 * HVS is reading, before its pages are freed. That list may also be one
 * Show() already replaced, so always wait for the last commit. */
void vcgfx_plane_forget(struct VideoCoreGfx_staticdata *xsd, OOP_Object *bm)
{
    BOOL gone = FALSE;
    ULONG i;

    if (!xsd->vcsd_PlanesMode)
        return;

    VC4_MBOX_LOCK(xsd);
    if (xsd->vcsd_Mirror == bm)
        xsd->vcsd_Mirror = NULL;
    if (xsd->vcsd_Primary.pl_BM == bm)
    {
        xsd->vcsd_Primary.pl_BM = NULL;
        gone = TRUE;
    }
    for (i = 0; i < VCGFX_OVL_PLANES; i++)
    {
        if (xsd->vcsd_OvlPlane[i].pl_BM == bm)
        {
            xsd->vcsd_OvlPlane[i].pl_BM = NULL;
            gone = TRUE;
        }
    }
    if (gone)
    {
        planes_stack(xsd);
        planes_commit(xsd);
    }
    VC4_MBOX_UNLOCK(xsd);

    vc4_hvs_latch_wait(xsd);
}

/* Show page 'page' of a bitmap from the next frame on; with wait, return
 * once the old one is off the display. FALSE when no plane shows it. */
BOOL vcgfx_plane_page(OOP_Class *cl, OOP_Object *bm, ULONG page, BOOL wait)
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

    VC4_MBOX_LOCK(xsd);
    data->front = page;
    data->VideoData = data->scan[page].sb_Pixels;

    tags[0].ti_Data = (IPTR)data->VideoData;
    setmsg.mID      = OOP_GetMethodID(IID_Root, moRoot_Set);
    setmsg.attrList = tags;
    OOP_DoSuperMethod(cl, bm, (OOP_Msg)&setmsg);

    pl = plane_find(xsd, bm);
    shown = (pl != NULL);
    if (shown && !((xsd->vcsd_HVSGen == VCGFX_HVS_VC4) && vc4_hvs_plane_flip(xsd, pl)))
        planes_commit(xsd);
    VC4_MBOX_UNLOCK(xsd);

    if (shown && wait)
        vc4_hvs_latch_wait(xsd);
    return shown;
}

/* Is a plane scanning the bitmap out? */
BOOL vcgfx_plane_shows(struct VideoCoreGfx_staticdata *xsd, OOP_Object *bm)
{
    BOOL shown;

    VC4_MBOX_LOCK(xsd);
    shown = (plane_find(xsd, bm) != NULL);
    VC4_MBOX_UNLOCK(xsd);
    return shown;
}
