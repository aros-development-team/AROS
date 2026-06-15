/*
    Copyright (C) 2013-2017, The AROS Development Team. All rights reserved.

    Desc: BCM VideoCore4 Gfx Hidd Class.
*/

#define DEBUG 0
#include <aros/debug.h>

#define __OOP_NOATTRBASES__

#include <aros/asmcall.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <aros/symbolsets.h>
#include <devices/inputevent.h>
#include <exec/alerts.h>
#include <exec/memory.h>
#include <graphics/displayinfo.h>
#include <graphics/view.h>
#include <hardware/custom.h>
#include <hidd/hidd.h>
#include <hidd/gfx.h>
#include <hidd/gallium.h>
#include <oop/oop.h>
#include <clib/alib_protos.h>
#include <string.h>

#include <proto/mbox.h>

#include "vc4gfx_hidd.h"
#include "vc4gfx_hardware.h"
#include "vc4gfx_neon.h"

#ifdef MBoxBase
#undef MBoxBase
#endif
#define MBoxBase      xsd->vcsd_MBoxBase

#include LC_LIBDEFS_FILE

#define MNAME_ROOT(x) VideoCoreGfx__Root__ ## x
#define MNAME_GFX(x) VideoCoreGfx__Hidd_Gfx__ ## x

#define SYNCTAGS_SIZE (11 * sizeof(struct TagItem))

APTR FNAME_SUPPORT(GenModeArray)(OOP_Class *cl, OOP_Object *o, struct List *modelist, struct TagItem *fmts)
{
    APTR                modearray = NULL;
    struct TagItem      *ma_syncs = NULL, *ma_synctags = NULL;
    int                 i, fmtcount = 0, modecount = 0;
    struct DisplayMode  *modecurrent;

    /* quickly count fmts and modes */
    while (fmts[fmtcount].ti_Tag == aHidd_Gfx_PixFmtTags)
        fmtcount++;

    ForeachNode(modelist, modecurrent)
    {
        modecount++;
    }

    D(bug("[VideoCoreGfx] %s: %d PixFmts, %d SyncModes\n", __PRETTY_FUNCTION__, fmtcount, modecount));

    /* build our table .. */
    if (fmtcount && modecount)
    {
        if ((modearray = AllocVec((fmtcount * sizeof(struct TagItem)) + ((modecount + 1) * sizeof(struct TagItem)) + (modecount * SYNCTAGS_SIZE), MEMF_PUBLIC)) != NULL)
        {
            struct TagItem *ma_fmts = (struct TagItem  *)modearray;

            D(bug("[VideoCoreGfx] %s: PixFmt's @ 0x%p\n", __PRETTY_FUNCTION__, modearray));

            for (i = 0; i < fmtcount; i ++)
            {
                D(bug("[VideoCoreGfx] %s: PixFmt #%d @ 0x%p\n", __PRETTY_FUNCTION__, i, fmts[i].ti_Data));
                ma_fmts[i].ti_Tag = aHidd_Gfx_PixFmtTags;
                ma_fmts[i].ti_Data = fmts[i].ti_Data;
            }
            ma_syncs = (struct TagItem *)&ma_fmts[fmtcount];
            D(bug("[VideoCoreGfx] %s: SyncMode's @ 0x%p\n", __PRETTY_FUNCTION__, ma_syncs));
            ma_synctags = (struct TagItem  *)&ma_syncs[modecount + 1];
            i = 0;
            ForeachNode(modelist, modecurrent)
            {
                D(bug("[VideoCoreGfx] %s: SyncMode #%d Tags @ 0x%p\n", __PRETTY_FUNCTION__, i, ma_synctags));

                ma_syncs[i].ti_Tag = aHidd_Gfx_SyncTags;
                ma_syncs[i].ti_Data = (IPTR)ma_synctags;

                ma_synctags[0].ti_Tag = aHidd_Sync_PixelClock;
                ma_synctags[0].ti_Data = modecurrent->dm_clock * 1000;
                ma_synctags[1].ti_Tag = aHidd_Sync_HDisp;
                ma_synctags[1].ti_Data = modecurrent->dm_hdisp;
                ma_synctags[2].ti_Tag = aHidd_Sync_HSyncStart;
                ma_synctags[2].ti_Data = modecurrent->dm_hstart;
                ma_synctags[3].ti_Tag = aHidd_Sync_HSyncEnd;
                ma_synctags[3].ti_Data = modecurrent->dm_hend;
                ma_synctags[4].ti_Tag = aHidd_Sync_HTotal;
                ma_synctags[4].ti_Data = modecurrent->dm_htotal;
                ma_synctags[5].ti_Tag = aHidd_Sync_VDisp;
                ma_synctags[5].ti_Data = modecurrent->dm_vdisp;
                ma_synctags[6].ti_Tag = aHidd_Sync_VSyncStart;
                ma_synctags[6].ti_Data = modecurrent->dm_vstart;
                ma_synctags[7].ti_Tag = aHidd_Sync_VSyncEnd;
                ma_synctags[7].ti_Data = modecurrent->dm_vend;
                ma_synctags[8].ti_Tag = aHidd_Sync_VTotal;
                ma_synctags[8].ti_Data = modecurrent->dm_vtotal;
                ma_synctags[9].ti_Tag = aHidd_Sync_Description;
                ma_synctags[9].ti_Data = (IPTR)modecurrent->dm_descr;
                ma_synctags[10].ti_Tag = TAG_DONE;

                ma_synctags = (struct TagItem  *)&ma_synctags[11];
                i++;
            }
            ma_syncs[i].ti_Tag = TAG_DONE;
        }
    }
#if defined(DEBUGMODEARRAY)
    if (modearray)
    {
        ma_syncs = (struct TagItem *)modearray;
        while (ma_syncs->ti_Tag != TAG_DONE)
        {
            D(bug("[VideoCoreGfx] %s: 0x%p: %08x, %08x\n", __PRETTY_FUNCTION__, ma_syncs, ma_syncs->ti_Tag, ma_syncs->ti_Data));
            ma_syncs++;
        }
    }
#endif
    return (APTR)modearray;
}

void FNAME_SUPPORT(DestroyModeArray)(struct List *modelist, APTR modearray)
{
    D(bug("[VideoCoreGfx] %s()\n", __PRETTY_FUNCTION__));
}

OOP_Object *MNAME_ROOT(New)(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    //struct VideoCoreGfx_staticdata *xsd = XSD(cl);
    OOP_Object                  *self = NULL;

    struct TagItem gfxmsg_tags[] =
    {
        { aHidd_Gfx_ModeTags    , (IPTR)NULL   },
        { aHidd_Name            , (IPTR)"vc4gfx.hidd"     },
        { aHidd_HardwareName    , (IPTR)"VideoCore4 Display Adaptor"   },
        { aHidd_ProducerName    , (IPTR)"Broadcom Corporation"  },
        { TAG_MORE              , (IPTR)msg->attrList       }
    };
    struct pRoot_New            gfxmsg_New;

    struct List                 vc_modelist;
    APTR                        vc_modearray, vc_pixfmts;

    EnterFunc(bug("VideoCoreGfx::New()\n"));

    NewList(&vc_modelist);

    FNAME_SUPPORT(SDTV_SyncGen)(&vc_modelist, cl);
    FNAME_SUPPORT(HDMI_SyncGen)(&vc_modelist, cl);
    vc_pixfmts = FNAME_SUPPORT(GenPixFmts)(cl);

    if ((vc_modearray = FNAME_SUPPORT(GenModeArray)(cl, o, &vc_modelist, (struct TagItem *)vc_pixfmts)) != NULL)
    {
        D(bug("[VideoCoreGfx] VideoCoreGfx::New: Generated Mode Array @ 0x%p\n", vc_modearray));

        gfxmsg_tags[0].ti_Data = (IPTR)vc_modearray;

        gfxmsg_New.mID = msg->mID;
        gfxmsg_New.attrList = gfxmsg_tags;
        msg = &gfxmsg_New;

        D(bug("[VideoCoreGfx] VideoCoreGfx::New: Creating object [cl:0x%p, o:0x%p, msg:0x%p]\n", cl, o, msg));

        if ((self = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg)) != NULL)
        {
            D(bug("[VideoCoreGfx] VideoCoreGfx::New: Storing reference to self in staticdata\n"));
            XSD(cl)->vcsd_VideoCoreGfxInstance = self;
        }
        FNAME_SUPPORT(DestroyModeArray)(&vc_modelist, vc_modearray);
    }

    ReturnPtr("VideoCoreGfx::New: Obj", OOP_Object *, self);
}

VOID MNAME_ROOT(Dispose)(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    D(bug("[VideoCoreGfx] VideoCoreGfx::Dispose()\n"));

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID MNAME_ROOT(Get)(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;
    BOOL found = FALSE;

//    D(bug("[VideoCoreGfx] VideoCoreGfx::Get()\n"));

    if (IS_GFX_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
        case aoHidd_Gfx_MemorySize:
            *msg->storage = (IPTR)(XSD(cl)->vcsd_GPUMemManage.mhe_MemHeader.mh_Upper - XSD(cl)->vcsd_GPUMemManage.mhe_MemHeader.mh_Lower);
            found = TRUE;
            break;

        case aoHidd_Gfx_HWSpriteTypes:
            *msg->storage = XSD(cl)->vcsd_CurBuf ? vHidd_SpriteType_DirectColor : 0;
            found = TRUE;
            break;
        }
    }
    if (!found)
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

int FNAME_SUPPORT(InitCursor)(struct VideoCoreGfx_staticdata *xsd)
{
    /* 64x64 ARGB cursor buffer in GPU memory, locked for the driver's
     * lifetime (firmware re-reads it on each SETCURSORINFO). VCMEM_DIRECT
     * (uncached 0xC alias) lets the CPU write pixels; VCMEM_NORMAL is
     * rejected for ARM-side requests.
     * ALLOCMEM value buffer: size/align/flags in, handle out.
     */
    VC4_MBOX_LOCK(xsd);
    xsd->vcsd_MBoxMessage[0] = AROS_LE2LONG(10 * 4);
    xsd->vcsd_MBoxMessage[1] = AROS_LE2LONG(VCTAG_REQ);
    xsd->vcsd_MBoxMessage[2] = AROS_LE2LONG(VCTAG_ALLOCMEM);
    xsd->vcsd_MBoxMessage[3] = AROS_LE2LONG(12);
    xsd->vcsd_MBoxMessage[4] = AROS_LE2LONG(12);
    xsd->vcsd_MBoxMessage[5] = AROS_LE2LONG(VC4_CURSOR_BUF_BYTES);
    xsd->vcsd_MBoxMessage[6] = AROS_LE2LONG(16);
    xsd->vcsd_MBoxMessage[7] = AROS_LE2LONG(VCMEM_DIRECT);
    xsd->vcsd_MBoxMessage[8] = 0;
    xsd->vcsd_MBoxMessage[9] = 0;

    if (MBoxCall((void*)VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage)
        == (volatile unsigned int *)-1)
    {
        VC4_MBOX_UNLOCK(xsd);
        return FALSE;
    }
    xsd->vcsd_CurBufHandle = AROS_LE2LONG(xsd->vcsd_MBoxMessage[5]);
    if (!xsd->vcsd_CurBufHandle)
    {
        VC4_MBOX_UNLOCK(xsd);
        return FALSE;
    }

    /* LOCKMEM: handle in, bus address out (same value slot). */
    xsd->vcsd_MBoxMessage[0] = AROS_LE2LONG(8 * 4);
    xsd->vcsd_MBoxMessage[1] = AROS_LE2LONG(VCTAG_REQ);
    xsd->vcsd_MBoxMessage[2] = AROS_LE2LONG(VCTAG_LOCKMEM);
    xsd->vcsd_MBoxMessage[3] = AROS_LE2LONG(4);
    xsd->vcsd_MBoxMessage[4] = AROS_LE2LONG(4);
    xsd->vcsd_MBoxMessage[5] = AROS_LE2LONG(xsd->vcsd_CurBufHandle);
    xsd->vcsd_MBoxMessage[6] = 0;
    xsd->vcsd_MBoxMessage[7] = 0;

    if (MBoxCall((void*)VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage)
        == (volatile unsigned int *)-1)
    {
        VC4_MBOX_UNLOCK(xsd);
        return FALSE;
    }

    xsd->vcsd_CurBufBus = AROS_LE2LONG(xsd->vcsd_MBoxMessage[5]);
    VC4_MBOX_UNLOCK(xsd);
    xsd->vcsd_CurVisible = FALSE;

    /* A valid bus address has the GPU alias bits set (0xC/0x4). QEMU may
     * return a bare low-RAM address that overlaps CPU allocations; treat
     * anything outside the alias range as "no HW cursor".
     */
    if ((xsd->vcsd_CurBufBus & 0xC0000000) == 0)
    {
        xsd->vcsd_CurBuf = NULL;
        return FALSE;
    }
    xsd->vcsd_CurBuf = (APTR)(xsd->vcsd_CurBufBus & 0x3fffffff);

    if (xsd->vcsd_CurBuf == NULL)
        return FALSE;

    D(bug("[VideoCoreGfx] HW cursor buffer @ 0x%p (bus 0x%08x)\n",
        xsd->vcsd_CurBuf, xsd->vcsd_CurBufBus));
    return TRUE;
}

VOID MNAME_GFX(NominalDimensions)(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_NominalDimensions *msg)
{
    struct VideoCoreGfx_staticdata *xsd = XSD(cl);

    /* Return the panel's native HDMI resolution if known, else fall
     * through to the superclass (AROS_NOMINAL_*).
     */
    if (xsd->vcsd_NativeWidth && xsd->vcsd_NativeHeight)
    {
        if (msg->width)
            *msg->width  = xsd->vcsd_NativeWidth;
        if (msg->height)
            *msg->height = xsd->vcsd_NativeHeight;
        if (msg->depth)
            *msg->depth  = 24;
        return;
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

BOOL MNAME_GFX(SetCursorShape)(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorShape *msg)
{
    struct VideoCoreGfx_staticdata *xsd = XSD(cl);
    IPTR width = 0, height = 0;

    if (!xsd->vcsd_CurBuf)
        return FALSE;

    if (msg->shape == NULL)
    {
        /* Hide and forget the current shape. */
        xsd->vcsd_CurVisible = FALSE;
        VC4_MBOX_LOCK(xsd);
        xsd->vcsd_MBoxMessage[0] = AROS_LE2LONG(8 * 4);
        xsd->vcsd_MBoxMessage[1] = AROS_LE2LONG(VCTAG_REQ);
        xsd->vcsd_MBoxMessage[2] = AROS_LE2LONG(VCTAG_SETCURSORSTATE);
        xsd->vcsd_MBoxMessage[3] = AROS_LE2LONG(16);
        xsd->vcsd_MBoxMessage[4] = AROS_LE2LONG(16);
        xsd->vcsd_MBoxMessage[5] = AROS_LE2LONG(0);     /* enable=0 */
        xsd->vcsd_MBoxMessage[6] = AROS_LE2LONG(0);
        xsd->vcsd_MBoxMessage[7] = AROS_LE2LONG(0);
        xsd->vcsd_MBoxMessage[8] = AROS_LE2LONG(0);
        xsd->vcsd_MBoxMessage[9] = 0;
        MBoxCall((void*)VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage);
        VC4_MBOX_UNLOCK(xsd);
        return TRUE;
    }

    OOP_GetAttr(msg->shape, aHidd_BitMap_Width,  &width);
    OOP_GetAttr(msg->shape, aHidd_BitMap_Height, &height);
    if (width == 0 || height == 0 ||
        width > VC4_CURSOR_MAX_W || height > VC4_CURSOR_MAX_H)
        return FALSE;

    HIDD_BM_GetImage(msg->shape, (UBYTE *)xsd->vcsd_CurBuf,
                     width * 4, 0, 0, width, height, vHidd_StdPixFmt_BGRA32);

    xsd->vcsd_CurWidth  = width;
    xsd->vcsd_CurHeight = height;
    xsd->vcsd_CurHotX   = msg->xoffset;
    xsd->vcsd_CurHotY   = msg->yoffset;

    /* SETCURSORINFO: width, height, format(0), buf, hot_x, hot_y */
    VC4_MBOX_LOCK(xsd);
    xsd->vcsd_MBoxMessage[0] = AROS_LE2LONG(12 * 4);
    xsd->vcsd_MBoxMessage[1] = AROS_LE2LONG(VCTAG_REQ);
    xsd->vcsd_MBoxMessage[2] = AROS_LE2LONG(VCTAG_SETCURSORINFO);
    xsd->vcsd_MBoxMessage[3] = AROS_LE2LONG(24);
    xsd->vcsd_MBoxMessage[4] = AROS_LE2LONG(24);
    xsd->vcsd_MBoxMessage[5] = AROS_LE2LONG(width);
    xsd->vcsd_MBoxMessage[6] = AROS_LE2LONG(height);
    xsd->vcsd_MBoxMessage[7] = AROS_LE2LONG(0);
    xsd->vcsd_MBoxMessage[8] = AROS_LE2LONG(xsd->vcsd_CurBufBus);
    xsd->vcsd_MBoxMessage[9] = AROS_LE2LONG(xsd->vcsd_CurHotX);
    xsd->vcsd_MBoxMessage[10] = AROS_LE2LONG(xsd->vcsd_CurHotY);
    xsd->vcsd_MBoxMessage[11] = 0;

    if (MBoxCall((void*)VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage)
        == (volatile unsigned int *)-1)
    {
        VC4_MBOX_UNLOCK(xsd);
        return FALSE;
    }
    {
        BOOL ok = (AROS_LE2LONG(xsd->vcsd_MBoxMessage[5]) == 0);
        VC4_MBOX_UNLOCK(xsd);
        return ok;
    }
}

BOOL MNAME_GFX(SetCursorPos)(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorPos *msg)
{
    struct VideoCoreGfx_staticdata *xsd = XSD(cl);

    if (!xsd->vcsd_CurBuf)
        return FALSE;

    /* Firmware places the image top-left at (x,y) and ignores the
     * SETCURSORINFO hotspot, so apply the AROS hotspot offset here
     * (vcsd_CurHotX/Y = pointer xoffset/yoffset, typically <= 0). */
    xsd->vcsd_CurX = (LONG)msg->x + (LONG)xsd->vcsd_CurHotX;
    xsd->vcsd_CurY = (LONG)msg->y + (LONG)xsd->vcsd_CurHotY;

    if (!xsd->vcsd_CurVisible)
        return TRUE;

    VC4_MBOX_LOCK(xsd);
    xsd->vcsd_MBoxMessage[0] = AROS_LE2LONG(8 * 4);
    xsd->vcsd_MBoxMessage[1] = AROS_LE2LONG(VCTAG_REQ);
    xsd->vcsd_MBoxMessage[2] = AROS_LE2LONG(VCTAG_SETCURSORSTATE);
    xsd->vcsd_MBoxMessage[3] = AROS_LE2LONG(16);
    xsd->vcsd_MBoxMessage[4] = AROS_LE2LONG(16);
    xsd->vcsd_MBoxMessage[5] = AROS_LE2LONG(1);     /* enable */
    xsd->vcsd_MBoxMessage[6] = AROS_LE2LONG((ULONG)xsd->vcsd_CurX);
    xsd->vcsd_MBoxMessage[7] = AROS_LE2LONG((ULONG)xsd->vcsd_CurY);
    xsd->vcsd_MBoxMessage[8] = AROS_LE2LONG(1);     /* 1 = framebuffer coords (firmware scales) */
    xsd->vcsd_MBoxMessage[9] = 0;

    MBoxCall((void*)VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage);
    VC4_MBOX_UNLOCK(xsd);
    return TRUE;
}

VOID MNAME_GFX(SetCursorVisible)(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorVisible *msg)
{
    struct VideoCoreGfx_staticdata *xsd = XSD(cl);

    if (!xsd->vcsd_CurBuf)
        return;

    xsd->vcsd_CurVisible = msg->visible ? TRUE : FALSE;

    VC4_MBOX_LOCK(xsd);
    xsd->vcsd_MBoxMessage[0] = AROS_LE2LONG(8 * 4);
    xsd->vcsd_MBoxMessage[1] = AROS_LE2LONG(VCTAG_REQ);
    xsd->vcsd_MBoxMessage[2] = AROS_LE2LONG(VCTAG_SETCURSORSTATE);
    xsd->vcsd_MBoxMessage[3] = AROS_LE2LONG(16);
    xsd->vcsd_MBoxMessage[4] = AROS_LE2LONG(16);
    xsd->vcsd_MBoxMessage[5] = AROS_LE2LONG(msg->visible ? 1 : 0);
    xsd->vcsd_MBoxMessage[6] = AROS_LE2LONG((ULONG)xsd->vcsd_CurX);
    xsd->vcsd_MBoxMessage[7] = AROS_LE2LONG((ULONG)xsd->vcsd_CurY);
    xsd->vcsd_MBoxMessage[8] = AROS_LE2LONG(1);     /* 1 = framebuffer coords */
    xsd->vcsd_MBoxMessage[9] = 0;

    MBoxCall((void*)VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage);
    VC4_MBOX_UNLOCK(xsd);
}

/* Returns the DMA-addressable buffer base for our on/offscreen bitmaps
 * (uncached framebuffer / GPU mem), or 0 for foreign / RAM-backed ones.
 */
static ULONG vc4_bm_dma_base(OOP_Class *cl, OOP_Object *bm)
{
    OOP_Class *bmcl = OOP_OCLASS(bm);

    if (bmcl == XSD(cl)->vcsd_VideoCoreGfxOnBMClass ||
        bmcl == XSD(cl)->vcsd_VideoCoreGfxOffBMClass)
    {
        struct BitmapData *data = OOP_INST_DATA(bmcl, bm);
        return (ULONG)(IPTR)data->VideoData;
    }
    return 0;
}

/* NEON rect copy between two chunky 32bpp bitmaps; falls through to
 * super otherwise. Handles same-buffer overlap via row direction;
 * same-row right-shift uses a reverse copyline.
 */
VOID MNAME_GFX(CopyBox)(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CopyBox *msg)
{
    IPTR src_buf = 0, dst_buf = 0;
    IPTR src_pitch = 0, dst_pitch = 0;
    IPTR src_bpp = 0, dst_bpp = 0;
    UBYTE *srow_base, *drow_base;
    ULONG row_bytes;
    LONG y, y_start, y_end, y_step;
    BOOL same_row_overlap;

    if (GC_DRMD(msg->gc) != vHidd_GC_DrawMode_Copy)
    {
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        return;
    }

    OOP_GetAttr(msg->src,  aHidd_ChunkyBM_Buffer,    &src_buf);
    OOP_GetAttr(msg->dest, aHidd_ChunkyBM_Buffer,    &dst_buf);
    OOP_GetAttr(msg->src,  aHidd_BitMap_BytesPerRow, &src_pitch);
    OOP_GetAttr(msg->dest, aHidd_BitMap_BytesPerRow, &dst_pitch);
    {
        OOP_Object *src_pf = NULL, *dst_pf = NULL;
        OOP_GetAttr(msg->src,  aHidd_BitMap_PixFmt, (IPTR *)&src_pf);
        OOP_GetAttr(msg->dest, aHidd_BitMap_PixFmt, (IPTR *)&dst_pf);
        if (src_pf)
            OOP_GetAttr(src_pf, aHidd_PixFmt_BytesPerPixel, &src_bpp);
        if (dst_pf)
            OOP_GetAttr(dst_pf, aHidd_PixFmt_BytesPerPixel, &dst_bpp);
    }

    if (!src_buf || !dst_buf || src_bpp != 4 || dst_bpp != 4)
    {
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        return;
    }

    row_bytes = msg->width * 4;
    srow_base = (UBYTE *)src_buf + msg->srcY  * src_pitch + msg->srcX  * 4;
    drow_base = (UBYTE *)dst_buf + msg->destY * dst_pitch + msg->destX * 4;

    /* Pick row iteration direction so overlapping in-buffer copies
     * read each source row before it's overwritten by a dest row.
     */
    if (src_buf == dst_buf && msg->destY > msg->srcY)
    {
        y_start = msg->height - 1;
        y_end   = -1;
        y_step  = -1;
    }
    else
    {
        y_start = 0;
        y_end   = msg->height;
        y_step  = 1;
    }

    /* Same buffer, same row(s), dest right of source within its extent:
     * rows alias byte-wise, need a reverse copy.
     */
    same_row_overlap = (src_buf == dst_buf
                        && msg->destY == msg->srcY
                        && msg->destX > msg->srcX
                        && (ULONG)(msg->destX - msg->srcX) < msg->width);

    /* No page flip here: desktop rendering is incremental read-modify-write
     * and higher layers cache the framebuffer base, so flipping would expose
     * a stale page. Page flipping stays in the gallium full-screen path,
     * where Mesa renders a complete frame in its own BO.
     */

    /* Large copies between two uncached DMA-addressable buffers go through
     * the DMA engine (uncached CPU reads dominate the NEON cost). Same-row
     * horizontal overlap can't be done in 2D stride mode (rows always go
     * forward), so it stays on the NEON reverse copy.
     */
    if (!same_row_overlap
        && (row_bytes * msg->height) >= VC4_DMA_COPY_THRESHOLD
        && vc4_bm_dma_base(cl, msg->src) == (ULONG)src_buf
        && vc4_bm_dma_base(cl, msg->dest) == (ULONG)dst_buf
        && src_buf && dst_buf)
    {
        if (vc4_dma_copy(XSD(cl), (ULONG)(IPTR)srow_base, src_pitch,
                         (ULONG)(IPTR)drow_base, dst_pitch,
                         row_bytes, msg->height, (y_step < 0)))
            return;
    }

    for (y = y_start; y != y_end; y += y_step)
    {
        UBYTE *srow = srow_base + y * src_pitch;
        UBYTE *drow = drow_base + y * dst_pitch;
        if (same_row_overlap)
            neon_copyline_rev(drow, srow, row_bytes);
        else
            neon_copyline(drow, srow, row_bytes);
    }
}

OOP_Object *MNAME_GFX(CreateObject)(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CreateObject *msg)
{
    OOP_Object      *object = NULL;

    if (msg->cl == XSD(cl)->vcsd_basebm)
    {
        BOOL displayable;
        BOOL framebuffer;
        struct TagItem newbm_tags[2] =
        {
            {TAG_IGNORE, 0                  },
            {TAG_MORE  , (IPTR)msg->attrList}
        };
        struct pHidd_Gfx_CreateObject newbm_msg;

        displayable = (BOOL)GetTagData(aHidd_BitMap_Displayable, FALSE, msg->attrList);
        framebuffer = (BOOL)GetTagData(aHidd_BitMap_FrameBuffer, FALSE, msg->attrList);
        if (framebuffer)
        {
            newbm_tags[0].ti_Tag = aHidd_BitMap_ClassPtr;
            newbm_tags[0].ti_Data = (IPTR)XSD(cl)->vcsd_VideoCoreGfxOnBMClass;
        }
        else
        {
            /* Displayable bitmaps and friends of ours go to the offscreen
             * class: large 32bpp ones land in GPU memory, the rest in a
             * plain ChunkyBM system-RAM buffer. */
            OOP_Object *friend = (OOP_Object *)GetTagData(aHidd_BitMap_Friend, 0, msg->attrList);
            OOP_Class *frcl = friend ? OOP_OCLASS(friend) : NULL;

            if (displayable || frcl == XSD(cl)->vcsd_VideoCoreGfxOnBMClass
                            || frcl == XSD(cl)->vcsd_VideoCoreGfxOffBMClass)
            {
                newbm_tags[0].ti_Tag  = aHidd_BitMap_ClassPtr;
                newbm_tags[0].ti_Data = (IPTR)XSD(cl)->vcsd_VideoCoreGfxOffBMClass;
            }
        }

        newbm_msg.mID = msg->mID;
        newbm_msg.cl = msg->cl;
        newbm_msg.attrList = newbm_tags;

        object = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&newbm_msg);
    }
    else
    {
        /* Lazy lookup: at InitLib time the disk FS and gallium framework
         * aren't up. By the time CreatePipe calls HIDD_Gfx_CreateObject the
         * disk is mounted and CLID_Hidd_Gallium is registered. */
        if (!XSD(cl)->vcsd_basegallium)
            XSD(cl)->vcsd_basegallium = OOP_FindClass(CLID_Hidd_Gallium);

        if (XSD(cl)->vcsd_basegallium && msg->cl == XSD(cl)->vcsd_basegallium)
        {
            /* hidd.gallium.vc4 lives on the FS; load it on first request
             * so its OOP class registers before OOP_NewObject. */
            if (!XSD(cl)->vcsd_VC4GalliumLib)
                XSD(cl)->vcsd_VC4GalliumLib = OpenLibrary("vc4gallium.hidd", 0);

            if (XSD(cl)->vcsd_VC4GalliumLib)
            {
                /* Must match CLID_Hidd_Gallium_VC4 in vc4gallium_intern.h */
                object = OOP_NewObject(NULL, (STRPTR)"hidd.gallium.vc4", msg->attrList);
            }
            /* else: object stays NULL; CreatePipe will use its softpipe fallback. */
        }
        else
            object = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }

    return object;
}
