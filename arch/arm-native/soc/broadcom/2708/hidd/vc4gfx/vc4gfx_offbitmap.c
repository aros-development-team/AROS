/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BCM VideoCore4 Gfx Hidd Offscreen Bitmap Class.

    Large 32bpp offscreen bitmaps are placed in firmware (GPU) memory:
    uncached and physically contiguous, so CopyBox/PutImage/GetImage
    against the framebuffer run entirely on the DMA engine with no
    cache maintenance. Small bitmaps, non-32bpp formats and allocation
    failures fall back to the ChunkyBM superclass' system-RAM buffer,
    with VideoData left NULL so every accelerated path skips them.
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

#include "vc4gfx_bitmap.h"
#include "vc4gfx_hidd.h"

#include LC_LIBDEFS_FILE

#ifdef MBoxBase
#undef MBoxBase
#endif

#define MBoxBase      (&((struct VideoCoreGfxBase *)cl->UserData)->vsd)->vcsd_MBoxBase

#define MNAME_ROOT(x) VideoCoreGfxOffBM__Root__ ## x
#define MNAME_BM(x) VideoCoreGfxOffBM__Hidd_BitMap__ ## x

#include "vc4gfx_bitmapclass.c"

/* Included bitmapclass.c sets its own DEBUG; reset to this file's value. */
#undef DEBUG
#define DEBUG 0

/* Helpers below carry 'xsd'; switch MBoxBase to it (as in vc4gfx_onbitmap.c). */
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

/*********** BitMap::New() *************************************/

OOP_Object *MNAME_ROOT(New)(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    D(bug("[VideoCoreGfx] VideoCoreGfx.OffBitMap::New()\n"));

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

        if (HIDD_Gfx_GetMode(xsd->vcsd_VideoCoreGfxInstance, modeid, &sync, &modepf)
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
#if VC4_GPUBM_ENABLE
    struct VideoCoreGfx_staticdata *xsd = XSD(cl);
    struct BitmapData *data = OOP_INST_DATA(cl, o);
    ULONG handle = data->gpuhandle;

    OOP_DoSuperMethod(cl, o, msg);

    if (handle)
        vc4_gpumem_free(xsd, handle);
#else
    OOP_DoSuperMethod(cl, o, msg);
#endif
}
