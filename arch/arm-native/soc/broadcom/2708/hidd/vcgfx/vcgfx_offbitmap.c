/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BCM VideoCore4 Gfx Hidd Offscreen Bitmap Class.

    Screen bitmaps go in scanout memory when screens are HVS planes
    (vcgfx_planes.c): uncached and physically contiguous, so the HVS shows
    them directly and blits against them run on the DMA engine. Other
    bitmaps use the ChunkyBM superclass' system-RAM buffer, with VideoData
    left NULL so every accelerated path skips them.
*/

#define DEBUG 0
#include <aros/debug.h>

#define __OOP_NOATTRBASES__

#include <proto/oop.h>
#include <proto/mbox.h>
#include <proto/utility.h>
#include <exec/memory.h>
#include <graphics/gfx.h>
#include <hidd/gfx.h>
#include <oop/oop.h>
#include <stddef.h>
#include <string.h>

#include "vcgfx_bitmap.h"
#include "vcgfx_hidd.h"

#include LC_LIBDEFS_FILE

#ifdef MBoxBase
#undef MBoxBase
#endif

#define MBoxBase      (&((struct VideoCoreGfxBase *)cl->UserData)->vsd)->vcsd_MBoxBase

#define MNAME_ROOT(x) VideoCoreGfxOffBM__Root__ ## x
#define MNAME_BM(x) VideoCoreGfxOffBM__Hidd_BitMap__ ## x

#include "vcgfx_bitmapclass.c"

/* Included bitmapclass.c sets its own DEBUG; reset to this file's value. */
#undef DEBUG
#define DEBUG 0

/* Helpers below carry 'xsd'; switch MBoxBase to it (as in vcgfx_onbitmap.c). */
#undef MBoxBase
#define MBoxBase      xsd->vcsd_MBoxBase

/* GPU-memory placement of offscreen bitmaps is DISABLED.
 *
 * ALLOCMEM's address masked to physical (& 0x3fffffff) is mapped *cached*
 * by the ARM MMU, but DMA reaches it through the *uncached* 0xC0000000
 * alias. The CPU rendering paths do no cache maintenance, so a CPU-drawn
 * bitmap blitted by DMA shows stale lines — seen on real Pi 3 as 8-pixel
 * vertical stripes in Zune gadgets. (QEMU rejects ALLOCMEM, so it always
 * took the RAM fallback and never hit this.)
 *
 * Re-enabling needs the allocation mapped uncached for the CPU (an MMU
 * attribute change, as the kernel does for the framebuffer). Until then
 * offscreen bitmaps live in cached RAM and FB<->RAM transfers go through
 * vc4_dma_put/get, which handle cache maintenance.
 */
#define VC4_GPUBM_ENABLE 0

#if VC4_GPUBM_ENABLE

/* ALLOCMEM + LOCKMEM, using the tag layout proven by InitCursor.
 * Returns the CPU-physical address (uncached mapping) or NULL; the
 * firmware handle needed for freeing is stored in *handle_out.
 */
static APTR vc4_gpumem_alloc(struct VideoCoreGfx_staticdata *xsd,
                             ULONG size, ULONG *handle_out)
{
    ULONG handle, bus;

    VC4_MBOX_LOCK(xsd);
    xsd->vcsd_MBoxMessage[0] = AROS_LE2LONG(10 * 4);
    xsd->vcsd_MBoxMessage[1] = AROS_LE2LONG(VCTAG_REQ);
    xsd->vcsd_MBoxMessage[2] = AROS_LE2LONG(VCTAG_ALLOCMEM);
    xsd->vcsd_MBoxMessage[3] = AROS_LE2LONG(12);
    xsd->vcsd_MBoxMessage[4] = AROS_LE2LONG(12);
    xsd->vcsd_MBoxMessage[5] = AROS_LE2LONG(size);
    xsd->vcsd_MBoxMessage[6] = AROS_LE2LONG(32);
    xsd->vcsd_MBoxMessage[7] = AROS_LE2LONG(VCMEM_DIRECT);
    xsd->vcsd_MBoxMessage[8] = 0;
    xsd->vcsd_MBoxMessage[9] = 0;

    if (MBoxCall((void*)VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage)
        == (volatile unsigned int *)-1)
    {
        VC4_MBOX_UNLOCK(xsd);
        return NULL;
    }
    handle = AROS_LE2LONG(xsd->vcsd_MBoxMessage[5]);
    if (!handle)
    {
        VC4_MBOX_UNLOCK(xsd);
        return NULL;
    }

    xsd->vcsd_MBoxMessage[0] = AROS_LE2LONG(8 * 4);
    xsd->vcsd_MBoxMessage[1] = AROS_LE2LONG(VCTAG_REQ);
    xsd->vcsd_MBoxMessage[2] = AROS_LE2LONG(VCTAG_LOCKMEM);
    xsd->vcsd_MBoxMessage[3] = AROS_LE2LONG(4);
    xsd->vcsd_MBoxMessage[4] = AROS_LE2LONG(4);
    xsd->vcsd_MBoxMessage[5] = AROS_LE2LONG(handle);
    xsd->vcsd_MBoxMessage[6] = 0;
    xsd->vcsd_MBoxMessage[7] = 0;

    if (MBoxCall((void*)VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage)
        == (volatile unsigned int *)-1)
    {
        VC4_MBOX_UNLOCK(xsd);
        return NULL;
    }
    bus = AROS_LE2LONG(xsd->vcsd_MBoxMessage[5]);
    VC4_MBOX_UNLOCK(xsd);

    /* Firmware returns a bus address with alias bits set. QEMU may return a
     * bare low-RAM address overlapping CPU allocations — reject it (as the
     * cursor buffer does). */
    if ((bus & 0xC0000000) == 0)
    {
        VC4_MBOX_LOCK(xsd);
        xsd->vcsd_MBoxMessage[0] = AROS_LE2LONG(7 * 4);
        xsd->vcsd_MBoxMessage[1] = AROS_LE2LONG(VCTAG_REQ);
        xsd->vcsd_MBoxMessage[2] = AROS_LE2LONG(VCTAG_FREEMEM);
        xsd->vcsd_MBoxMessage[3] = AROS_LE2LONG(4);
        xsd->vcsd_MBoxMessage[4] = AROS_LE2LONG(4);
        xsd->vcsd_MBoxMessage[5] = AROS_LE2LONG(handle);
        xsd->vcsd_MBoxMessage[6] = 0;
        MBoxCall((void*)VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage);
        VC4_MBOX_UNLOCK(xsd);
        return NULL;
    }

    *handle_out = handle;
    return (APTR)(bus & 0x3fffffff);
}

static void vc4_gpumem_free(struct VideoCoreGfx_staticdata *xsd, ULONG handle)
{
    VC4_MBOX_LOCK(xsd);
    xsd->vcsd_MBoxMessage[0] = AROS_LE2LONG(8 * 4);
    xsd->vcsd_MBoxMessage[1] = AROS_LE2LONG(VCTAG_REQ);
    xsd->vcsd_MBoxMessage[2] = AROS_LE2LONG(VCTAG_UNLOCKMEM);
    xsd->vcsd_MBoxMessage[3] = AROS_LE2LONG(4);
    xsd->vcsd_MBoxMessage[4] = AROS_LE2LONG(4);
    xsd->vcsd_MBoxMessage[5] = AROS_LE2LONG(handle);
    xsd->vcsd_MBoxMessage[6] = 0;
    xsd->vcsd_MBoxMessage[7] = 0;
    MBoxCall((void*)VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage);

    xsd->vcsd_MBoxMessage[0] = AROS_LE2LONG(7 * 4);
    xsd->vcsd_MBoxMessage[1] = AROS_LE2LONG(VCTAG_REQ);
    xsd->vcsd_MBoxMessage[2] = AROS_LE2LONG(VCTAG_FREEMEM);
    xsd->vcsd_MBoxMessage[3] = AROS_LE2LONG(4);
    xsd->vcsd_MBoxMessage[4] = AROS_LE2LONG(4);
    xsd->vcsd_MBoxMessage[5] = AROS_LE2LONG(handle);
    xsd->vcsd_MBoxMessage[6] = 0;
    MBoxCall((void*)VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage);
    VC4_MBOX_UNLOCK(xsd);
}

#endif /* VC4_GPUBM_ENABLE */

/* A screen: a 32bpp displayable bitmap in a page of scanout memory. NULL
 * when it is something else, or when there is no memory left for it -
 * the screen then fails to open rather than open invisible. */
static OOP_Object *vc4_screen_new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg,
                                  BOOL *handled)
{
    struct VideoCoreGfx_staticdata *xsd = XSD(cl);
    OOP_Object *sync = NULL, *pf = NULL;
    IPTR width, height, bytesperpix = 0;
    HIDDT_ModeID modeid;
    struct vcgfx_scanbuf sb = { 0 };
    ULONG pitch;

    *handled = FALSE;
    if (!GetTagData(aHidd_BitMap_Displayable, FALSE, msg->attrList)
        || !vcgfx_planes_supported(xsd))
        return NULL;

    width  = GetTagData(aHidd_BitMap_Width,  0, msg->attrList);
    height = GetTagData(aHidd_BitMap_Height, 0, msg->attrList);
    modeid = (HIDDT_ModeID)GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);

    if ((modeid == vHidd_ModeID_Invalid)
        || !HIDD_DMEnum_GetMode(xsd->vcsd_DMEnum, modeid, &sync, &pf) || !pf)
        return NULL;
    if (!width && sync)
        OOP_GetAttr(sync, aHidd_Sync_HDisp, &width);
    if (!height && sync)
        OOP_GetAttr(sync, aHidd_Sync_VDisp, &height);
    OOP_GetAttr(pf, aHidd_PixFmt_BytesPerPixel, &bytesperpix);
    if (!width || !height || (bytesperpix != 4))
        return NULL;

    *handled = TRUE;

    /* The BitMap superclass's own 16-pixel row alignment. */
    pitch = ((width + 15) & ~15) * 4;
    if (!vcgfx_scanout_alloc(xsd, &sb, pitch * height))
    {
        bug("[VideoCoreGfx] no scanout memory for a %ux%u screen\n",
            (unsigned)width, (unsigned)height);
        return NULL;
    }

    {
        struct TagItem extra_tags[] =
        {
            { aHidd_ChunkyBM_Buffer,    (IPTR)sb.sb_Pixels     },
            { aHidd_BitMap_BytesPerRow, pitch                  },
            { TAG_MORE,                 (IPTR)msg->attrList    }
        };
        struct pRoot_New new_msg;

        new_msg.mID = msg->mID;
        new_msg.attrList = extra_tags;

        o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&new_msg);
    }
    if (!o)
    {
        vcgfx_scanout_free(xsd, &sb);
        return NULL;
    }

    {
        struct BitmapData *data = OOP_INST_DATA(cl, o);

        memset(data, 0, sizeof(struct BitmapData));
        data->VideoData   = sb.sb_Pixels;
        data->width       = width;
        data->height      = height;
        data->bytesperrow = pitch;
        data->bytesperpix = 4;
        data->bpp         = 32;
        data->disp        = -1;
        data->scan[0]     = sb;
        data->modew       = width;
        data->modeh       = height;
        if (sync)
        {
            IPTR mw = 0, mh = 0;

            OOP_GetAttr(sync, aHidd_Sync_HDisp, &mw);
            OOP_GetAttr(sync, aHidd_Sync_VDisp, &mh);
            if (mw && mh)
            {
                data->modew = mw;
                data->modeh = mh;
            }
        }
    }

    D(bug("[VideoCoreGfx] OffBitMap::New: %ux%u screen @ 0x%p, pitch %u\n",
        (unsigned)width, (unsigned)height, sb.sb_Pixels, (unsigned)pitch));
    return o;
}

/* A scan-out bitmap (aHidd_BitMap_Scanout): a client's surface the
 * compositor may put on a plane. Its buffers are pages of scanout memory
 * in a format an HVS plane reads: 32bpp as screens are, RGB15/16, or
 * NV12/I420 video with its chroma planes behind the luma in each page.
 * ChunkyBM refuses scan-out bitmaps, so it is told this one is none. */
static OOP_Object *vc4_scanout_new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg,
                                   BOOL *handled)
{
    struct VideoCoreGfx_staticdata *xsd = XSD(cl);
    struct vcgfx_scanbuf sb[VCGFX_SCAN_BUFFERS];
    OOP_Object *pf;
    struct TagItem *tags, *tag;
    IPTR width, height, stdpf = 0;
    ULONG videofmt, pxflags = 0, bpp, pitch, size, nbufs, i;

    *handled = FALSE;
    if (!GetTagData(aHidd_BitMap_Scanout, FALSE, msg->attrList))
        return NULL;
    *handled = TRUE;

    if (!vcgfx_planes_supported(xsd))
        return NULL;

    videofmt = GetTagData(aHidd_BitMap_VideoFormat, vHidd_VideoFmt_None, msg->attrList);
    width    = GetTagData(aHidd_BitMap_Width,  0, msg->attrList);
    height   = GetTagData(aHidd_BitMap_Height, 0, msg->attrList);
    pf       = (OOP_Object *)GetTagData(aHidd_BitMap_PixFmt, 0, msg->attrList);
    if (pf)
        OOP_GetAttr(pf, aHidd_PixFmt_StdPixFmt, &stdpf);
    if (!width || !height)
        return NULL;

    if (videofmt)
    {
        /* 4:2:0: whole chroma samples only */
        if (!vcgfx_yuv_ok(xsd, videofmt) || (width & 1) || (height & 1))
            return NULL;
        bpp   = 1;
        pitch = (width + 31) & ~31;
        size  = pitch * height * 3 / 2;
    }
    else
    {
        if (stdpf == vcgfx_native32(xsd))
            pxflags = 0;
        else if (stdpf == vHidd_StdPixFmt_RGB16_LE)
            pxflags = VC4GFX_OVL_RGB565;
        else if (stdpf == vHidd_StdPixFmt_RGB15_LE)
            pxflags = VC4GFX_OVL_RGB555;
        else
            return NULL;
        bpp   = vc4_ovl_bpp(pxflags);
        pitch = ((width + 15) & ~15) * bpp;
        size  = pitch * height;
    }
    nbufs = GetTagData(aHidd_BitMap_Buffers, 1, msg->attrList);
    if (nbufs < 1)
        nbufs = 1;
    if (nbufs > VCGFX_SCAN_BUFFERS)
        nbufs = VCGFX_SCAN_BUFFERS;

    memset(sb, 0, sizeof(sb));
    for (i = 0; i < nbufs; i++)
    {
        if (!vcgfx_scanout_alloc(xsd, &sb[i], size))
        {
            while (i--)
                vcgfx_scanout_free(xsd, &sb[i]);
            return NULL;
        }
    }

    if ((tags = CloneTagItems(msg->attrList)))
    {
        if ((tag = FindTagItem(aHidd_BitMap_Scanout, tags)))
            tag->ti_Tag = TAG_IGNORE;
        if ((tag = FindTagItem(aHidd_BitMap_Buffers, tags)))
            tag->ti_Tag = TAG_IGNORE;
        {
            struct TagItem extra_tags[] =
            {
                { aHidd_ChunkyBM_Buffer,    (IPTR)sb[0].sb_Pixels },
                { aHidd_BitMap_BytesPerRow, pitch                 },
                { TAG_MORE,                 (IPTR)tags            }
            };
            struct pRoot_New new_msg;

            new_msg.mID = msg->mID;
            new_msg.attrList = extra_tags;

            o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&new_msg);
        }
        FreeTagItems(tags);
    }
    else
        o = NULL;

    if (!o)
    {
        for (i = 0; i < nbufs; i++)
            vcgfx_scanout_free(xsd, &sb[i]);
        return NULL;
    }

    {
        struct BitmapData *data = OOP_INST_DATA(cl, o);

        memset(data, 0, sizeof(struct BitmapData));
        data->VideoData   = sb[0].sb_Pixels;
        data->width       = width;
        data->height      = height;
        data->bytesperrow = pitch;
        data->bytesperpix = bpp;
        data->bpp         = bpp * 8;
        data->disp        = -1;
        data->pxflags     = pxflags;
        data->videofmt    = videofmt;
        data->colorspace  = GetTagData(aHidd_BitMap_ColorSpace, vHidd_ColorSpace_BT601, msg->attrList);
        if (videofmt == vHidd_VideoFmt_NV12)
        {
            data->planeoff[0] = pitch * height;
            data->cpitch      = pitch;
        }
        else if (videofmt == vHidd_VideoFmt_I420)
        {
            /* The CGX layout: U after Y, V after U, both pitch / 2 */
            data->planeoff[0] = pitch * height;
            data->planeoff[1] = data->planeoff[0] + (pitch / 2) * (height / 2);
            data->cpitch      = pitch / 2;
        }
        data->nbufs       = nbufs;
        data->back        = (nbufs > 1) ? 1 : 0;
        for (i = 0; i < nbufs; i++)
            data->scan[i] = sb[i];
    }

    D(bug("[VideoCoreGfx] OffBitMap::New: %ux%u scan-out bitmap, %u buffers, pitch %u\n",
        (unsigned)width, (unsigned)height, (unsigned)nbufs, (unsigned)pitch));
    return o;
}

/*********** BitMap::New() *************************************/

OOP_Object *MNAME_ROOT(New)(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    BOOL handled;
    OOP_Object *screen;

    D(bug("[VideoCoreGfx] VideoCoreGfx.OffBitMap::New()\n"));

    screen = vc4_scanout_new(cl, o, msg, &handled);
    if (handled)
        return screen;
    screen = vc4_screen_new(cl, o, msg, &handled);
    if (handled)
        return screen;

#if VC4_GPUBM_ENABLE
    {
    struct VideoCoreGfx_staticdata *xsd = XSD(cl);
    OOP_Object *friend_bm, *pf = NULL;
    IPTR width, height, bytesperpix = 0;
    HIDDT_ModeID modeid;
    ULONG pitch = 0, size = 0, handle = 0;
    APTR gpu_ptr = NULL;

    width     = GetTagData(aHidd_BitMap_Width,  0, msg->attrList);
    height    = GetTagData(aHidd_BitMap_Height, 0, msg->attrList);
    friend_bm = (OOP_Object *)GetTagData(aHidd_BitMap_Friend, 0, msg->attrList);
    modeid    = (HIDDT_ModeID)GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);

    /* Pixel cell size: from the mode for displayable bitmaps, from the
     * friend's pixfmt otherwise. If it can't be determined, the bitmap
     * stays in system RAM. */
    if (modeid != vHidd_ModeID_Invalid)
    {
        OOP_Object *sync = NULL, *modepf = NULL;

        if (HIDD_DMEnum_GetMode(xsd->vcsd_DMEnum, modeid, &sync, &modepf)
            && modepf)
        {
            pf = modepf;
            if (!width && sync)
                OOP_GetAttr(sync, aHidd_Sync_HDisp, &width);
            if (!height && sync)
                OOP_GetAttr(sync, aHidd_Sync_VDisp, &height);
        }
    }
    else if (friend_bm)
        OOP_GetAttr(friend_bm, aHidd_BitMap_PixFmt, (IPTR *)&pf);

    if (pf)
        OOP_GetAttr(pf, aHidd_PixFmt_BytesPerPixel, &bytesperpix);

    if (width && height && bytesperpix == 4)
    {
        pitch = ((width * 4) + 31) & ~31;
        size = pitch * height;
        if (size >= VC4_GPUBM_THRESHOLD)
            gpu_ptr = vc4_gpumem_alloc(xsd, size, &handle);
    }

    if (gpu_ptr)
    {
        struct TagItem extra_tags[] =
        {
            { aHidd_ChunkyBM_Buffer,    (IPTR)gpu_ptr          },
            { aHidd_BitMap_BytesPerRow, pitch                  },
            { TAG_MORE,                 (IPTR)msg->attrList    }
        };
        struct pRoot_New new_msg;

        new_msg.mID = msg->mID;
        new_msg.attrList = extra_tags;

        o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&new_msg);
        if (o)
        {
            struct BitmapData *data = OOP_INST_DATA(cl, o);

            memset(data, 0, sizeof(struct BitmapData));
            data->VideoData   = (UBYTE *)gpu_ptr;
            data->width       = width;
            data->height      = height;
            data->bytesperrow = pitch;
            data->bytesperpix = 4;
            data->bpp         = 32;
            data->gpuhandle   = handle;

            D(bug("[VideoCoreGfx] OffBitMap::New: %dx%d GPU mem @ 0x%p, pitch %d\n",
                (int)width, (int)height, gpu_ptr, (int)pitch));
        }
        else
            vc4_gpumem_free(xsd, handle);

        return o;
    }
    }
#endif /* VC4_GPUBM_ENABLE */

    /* Plain ChunkyBM system-RAM bitmap. VideoData stays NULL, so the
     * accelerated bitmap methods fall through to the super class. */
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
        memset(OOP_INST_DATA(cl, o), 0, sizeof(struct BitmapData));

    return o;
}

/**********  Bitmap::Dispose()  ***********************************/

VOID MNAME_ROOT(Dispose)(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct VideoCoreGfx_staticdata *xsd = XSD(cl);
    struct BitmapData *data = OOP_INST_DATA(cl, o);
    struct vcgfx_scanbuf scan[VCGFX_SCAN_BUFFERS];
    ULONG i;
#if VC4_GPUBM_ENABLE
    ULONG handle = data->gpuhandle;
#endif

    ObtainSemaphore(&xsd->vcsd_OvlLock);
    vcgfx_ovl_drop_bm(xsd, o);
    ReleaseSemaphore(&xsd->vcsd_OvlLock);

    /* Off the display before the pages go. */
    if (data->scan[0].sb_Pixels)
        vcgfx_plane_forget(xsd, o);
    CopyMem(data->scan, scan, sizeof(scan));

    OOP_DoSuperMethod(cl, o, msg);

    for (i = 0; i < VCGFX_SCAN_BUFFERS; i++)
        vcgfx_scanout_free(xsd, &scan[i]);
#if VC4_GPUBM_ENABLE
    if (handle)
        vc4_gpumem_free(xsd, handle);
#endif
}

/**********  Bitmap::Set()  ***************************************/

/* LeftEdge/TopEdge are how graphics.library drags a screen: the superclass
 * clamps and stores them, the plane follows. */
IPTR MNAME_ROOT(Set)(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);
    struct TagItem *tag, *tstate = msg->attrList;
    BOOL moved = FALSE;
    ULONG idx;
    IPTR result;

    while ((tag = NextTagItem(&tstate)))
    {
        /* Flips and overlays need a screen the HVS can show. */
        if (data->scan[0].sb_Pixels && IS_VideoCoreGfxBM_ATTR(tag->ti_Tag, idx)
            && vcbm_set_private(cl, o, idx, tag->ti_Data))
            return TRUE;
        if (IS_BM_ATTR(tag->ti_Tag, idx)
            && ((idx == aoHidd_BitMap_LeftEdge) || (idx == aoHidd_BitMap_TopEdge)))
            moved = TRUE;
    }

    result = OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (moved)
        vcgfx_plane_moved(XSD(cl), o);
    return result;
}

/**********  Scan-out buffers  ************************************/

BOOL MNAME_BM(LockBuffer)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_LockBuffer *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);

    if (!data->nbufs || (msg->buffer >= data->nbufs) || !msg->addressReturn || !msg->pitchReturn)
        return FALSE;

    /* Scanout memory is write-through: what the CPU writes is what the plane reads */
    *msg->addressReturn = data->scan[msg->buffer].sb_Pixels;
    *msg->pitchReturn   = data->bytesperrow;
    if (msg->planeOffsets)
    {
        msg->planeOffsets[0] = data->planeoff[0];
        msg->planeOffsets[1] = data->planeoff[1];
    }
    return TRUE;
}

/* What a plane shows is already there; the compositor blits a surface on
 * no plane from its front buffer, so tell it when that one changed. */
VOID MNAME_BM(UnlockBuffer)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_UnlockBuffer *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);
    OOP_Object *display = NULL;
    const struct Rectangle *d = msg->dirty;

    /* Video is never blitted: off a plane it shows nothing */
    if (!data->nbufs || data->videofmt || (msg->buffer != data->front)
        || vcgfx_plane_shows(XSD(cl), o))
        return;

    OOP_GetAttr(o, aHidd_BitMap_Display, (IPTR *)&display);
    if (!display)
        return;
    if (d)
        HIDD_Display_UpdateRect(display, o, d->MinX, d->MinY,
                                d->MaxX - d->MinX + 1, d->MaxY - d->MinY + 1);
    else
        HIDD_Display_UpdateRect(display, o, 0, 0, data->width, data->height);
}

/* The displaced buffer joins the ring: the next one after the new front.
 * On no plane the compositor re-blits the bitmap from its new front. */
BOOL MNAME_BM(SwapBuffers)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_SwapBuffers *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);
    OOP_Object *display = NULL;

    if (!data->nbufs || (msg->newFront >= data->nbufs))
        return FALSE;
    if (msg->newFront == data->front)
        return TRUE;

    data->back = (msg->newFront + 1) % data->nbufs;
    if (!vcgfx_plane_page(cl, o, msg->newFront, !(msg->flags & fHidd_Swap_NoWait))
        && !data->videofmt)
    {
        OOP_GetAttr(o, aHidd_BitMap_Display, (IPTR *)&display);
        if (display)
            HIDD_Display_UpdateRect(display, o, 0, 0, data->width, data->height);
    }
    return TRUE;
}
