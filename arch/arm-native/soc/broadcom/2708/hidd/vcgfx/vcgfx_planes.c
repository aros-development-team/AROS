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

#include LC_LIBDEFS_FILE

/* Off: screens go back to Show() and the software compositor. */
#define VCGFX_PLANES        1

/* Arenas are 2MB aligned and sized, so the remap uses block entries only
 * and the kernel never splits a block into a page table it cannot free. */
#define SCANOUT_BLOCK       (2UL << 20)
#define SCANOUT_ARENA       (32UL << 20)
#define SCANOUT_PAGE        4096UL

/* The HVS5 0xC0000000 bus alias reaches the first 1GB only. */
#define SCANOUT_LIMIT       0x40000000ULL

#undef HiddBitMapAttrBase
#undef HiddChunkyBMAttrBase
#define HiddBitMapAttrBase      (xsd->vcsd_attrBases[3])
#define HiddChunkyBMAttrBase    (xsd->vcsd_attrBases[2])

BOOL vcgfx_planes_supported(struct VideoCoreGfx_staticdata *xsd)
{
    return VCGFX_PLANES && (xsd->vcsd_HVSGen == VCGFX_HVS_HVS5)
        && vc4_hvs5_planes_ok();
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
        && (xsd->vcsd_PlanesRefused || !xsd->vcsd_HVS.hvs_Active))
    {
        if (!xsd->vcsd_PlanesRefused)
            bug("[VideoCoreGfx] no display list of our own: screens stay on Show()\n");
        xsd->vcsd_PlanesRefused = TRUE;
        return FALSE;
    }

    for (v = vpd; v && (n < VCGFX_MAX_PLANES); v = v->Next)
    {
        struct BitmapData *data;

        if (OOP_OCLASS(v->Bitmap) != offcl)
            continue;
        data = OOP_INST_DATA(offcl, v->Bitmap);
        if (!data->VideoData || !data->scan[0].sb_Pixels)
        {
            bug("[VideoCoreGfx] screen bitmap 0x%p is not in scanout memory\n",
                v->Bitmap);
            continue;
        }

        shown[n].pl_BM   = v->Bitmap;
        shown[n].pl_Data = data;
        plane_pos(xsd, v->Bitmap, &shown[n].pl_X, &shown[n].pl_Y);
        n++;
    }

    /* A mode set blanks the display, so the timing follows the front
     * screen only when it is alone, or too big to scale down into it. */
    if (n && xsd->vcsd_FBObj
        && ((n == 1) || !vc4_hvs5_plane_fits(xsd, shown[0].pl_Data->modew,
                                             shown[0].pl_Data->modeh)))
    {
        IPTR mode = vHidd_ModeID_Invalid, fbmode = vHidd_ModeID_Invalid;

        OOP_GetAttr(shown[0].pl_BM, aHidd_BitMap_ModeID, &mode);
        OOP_GetAttr(xsd->vcsd_FBObj, aHidd_BitMap_ModeID, &fbmode);
        if ((mode != vHidd_ModeID_Invalid) && (mode != fbmode))
        {
            OOP_SetAttrsTags(xsd->vcsd_FBObj, aHidd_BitMap_ModeID, mode, TAG_DONE);
            xsd->vcsd_Backdrop = NULL;
        }
    }

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
    vc4_hvs5_planes_commit(xsd);
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
            vc4_hvs5_planes_commit(xsd);
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
    if (xsd->vcsd_HVS.hvs_OvlOwner == bm)
    {
        xsd->vcsd_HVS.hvs_OvlActive = FALSE;
        xsd->vcsd_HVS.hvs_OvlOwner  = NULL;
        gone = TRUE;
    }
    if ((pl = plane_find(xsd, bm)))
    {
        ULONG i = pl - xsd->vcsd_Plane;

        for (; i + 1 < xsd->vcsd_NumPlanes; i++)
            xsd->vcsd_Plane[i] = xsd->vcsd_Plane[i + 1];
        xsd->vcsd_NumPlanes--;
        gone = TRUE;
    }
    if (gone)
        vc4_hvs5_planes_commit(xsd);
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

    shown = (plane_find(xsd, bm) != NULL);
    if (shown)
        vc4_hvs5_planes_commit(xsd);
    VC4_MBOX_UNLOCK(xsd);

    if (shown)
        vc4_hvs_latch_wait(xsd);
    return TRUE;
}
