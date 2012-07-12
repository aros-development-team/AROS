/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "ati.h"
#include "radeon.h"
#include "radeon_reg.h"
#include "radeon_bios.h"
#include "radeon_accel.h"
#include "radeon_macros.h"

#include <oop/oop.h>
#include <hidd/graphics.h>
#include <hidd/hidd.h>

#include <proto/oop.h>
#include <proto/utility.h>

#define DEBUG 0
#include <aros/debug.h>

#define sd ((struct ati_staticdata*)SD(cl))

#undef HiddPCIDeviceAttrBase
#undef HiddGfxAttrBase
#undef HiddPixFmtAttrBase
#undef HiddSyncAttrBase
#undef HiddBitMapAttrBase
#define HiddPCIDeviceAttrBase   (sd->pciAttrBase)
#define HiddATIBitMapAttrBase   (sd->atiBitMapAttrBase)
#define HiddBitMapAttrBase  (sd->bitMapAttrBase)
#define HiddPixFmtAttrBase  (sd->pixFmtAttrBase)
#define HiddGfxAttrBase     (sd->gfxAttrBase)
#define HiddSyncAttrBase    (sd->syncAttrBase)

#define POINT_OUTSIDE_CLIP(gc, x, y)	\
	(  (x) < GC_CLIPX1(gc)		\
	|| (x) > GC_CLIPX2(gc)		\
	|| (y) < GC_CLIPY1(gc)		\
	|| (y) > GC_CLIPY2(gc) )

OOP_Object *METHOD(ATIOffBM, Root, New)
    __attribute__((alias(METHOD_NAME_S(ATIOnBM, Root, New))));

OOP_Object *METHOD(ATIOnBM, Root, New)
{
    if (cl == sd->OnBMClass)
        EnterFunc(bug("[ATIBitMap] OnBitmap::New()\n"));
    else
        EnterFunc(bug("[ATIBitMap] OffBitmap::New()\n"));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        atiBitMap *bm = OOP_INST_DATA(cl, o);

        IPTR width, height, depth;
        UBYTE bytesPerPixel;
        ULONG fb;

        OOP_Object *pf;

        InitSemaphore(&bm->bmLock);

        D(bug("[ATIBitMap] Super called. o=%p\n", o));

        bm->onbm = (cl == sd->OnBMClass);

        OOP_GetAttr(o, aHidd_BitMap_Width,  &width);
        OOP_GetAttr(o, aHidd_BitMap_Height, &height);
        OOP_GetAttr(o, aHidd_BitMap_PixFmt, (IPTR *)&pf);
        OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);
        fb = GetTagData(aHidd_BitMap_FrameBuffer, FALSE, msg->attrList);

        D(bug("[ATIBitmap] width=%d height=%d depth=%d\n", width, height, depth));

        if (width == 0 || height == 0 || depth == 0)
        {
            bug("[ATIBitMap] size mismatch!\n");
        }

        if (depth == 24)
        	depth = 32;

        if (depth <= 8)
            bytesPerPixel = 1;
        else if (depth <= 16)
            bytesPerPixel = 2;
        else
            bytesPerPixel = 4;

        if (fb)
        {
            width = 640;
            height = 480;
            bytesPerPixel = 4;
            depth = 32;
        }

        bm->width = width;
        bm->height = height;
        bm->pitch = (width * bytesPerPixel + 63) & ~63;
        bm->depth = depth;
        bm->bpp = bytesPerPixel;
        bm->framebuffer = AllocBitmapArea(sd, bm->width, bm->height, bm->bpp, TRUE);
        bm->fbgfx = TRUE;
        bm->state = NULL;
        bm->BitMap = o;
        bm->usecount = 0;
        bm->addresses = AllocVecPooled(sd->memPool, height * sizeof(void*));

        if (bm->framebuffer != -1)
        {
        	int __tmp;
        	for (__tmp=0; __tmp < height; __tmp++)
        		bm->addresses[__tmp] = (void*)(bm->framebuffer + sd->Card.FrameBuffer + __tmp*bm->pitch);

            switch(depth)
            {
                case 15:
                    bm->datatype = 3;
                    break;

                case 16:
                    bm->datatype = 4;
                    break;

                case 32:
                    bm->datatype = 6;
                    break;
            }

            bm->dp_gui_master_cntl =
                        ((bm->datatype << RADEON_GMC_DST_DATATYPE_SHIFT)
                        |RADEON_GMC_CLR_CMP_CNTL_DIS
                        |RADEON_GMC_DST_PITCH_OFFSET_CNTL);

            bm->pitch_offset = ((bm->framebuffer >> 10) | (bm->pitch << 16));

            D(bug("[ATIBitMap] PITCH_OFFSET=%08x\n", bm->pitch_offset));
        }

        if (cl == sd->OnBMClass)
        {
            if (fb && bm->framebuffer != -1)
            {
                bm->state = (struct CardState *)AllocPooled(sd->memPool,
                            sizeof(struct CardState));

                bzero((APTR)(sd->Card.FrameBuffer + bm->framebuffer), 640*480*2);

                if (bm->state)
                {
                    LOCK_HW

                    InitMode(sd, bm->state, 640, 480, 16, 25200, bm->framebuffer,
                        640, 480,
                        656, 752, 800,
                        490, 492, 525);

                    LoadState(sd, bm->state);
                    //LoadState(sd, sd->poweron_state);
					DPMS(sd, sd->dpms);

                    RADEONEngineReset(sd);
                    RADEONEngineRestore(sd);

                    UNLOCK_HW

                    return o;
                }
            }
            else if (bm->framebuffer != -1)
            {
                HIDDT_ModeID modeid;
                OOP_Object *sync;

                /* We should be able to get modeID from the bitmap */
                OOP_GetAttr(o, aHidd_BitMap_ModeID, &modeid);

                D(bug("[ATIBitMap] BM_ModeID=%x\n", modeid));

                if (modeid != vHidd_ModeID_Invalid)
                {
                    IPTR  pixel;
                    IPTR  hdisp, vdisp, hstart, hend, htotal, vstart, vend, vtotal;

                    /* Get Sync and PixelFormat properties */
                    struct pHidd_Gfx_GetMode __getmodemsg = {
                        modeID: modeid,
                        syncPtr:    &sync,
                        pixFmtPtr:  &pf,
                    }, *getmodemsg = &__getmodemsg;

                    getmodemsg->mID = OOP_GetMethodID((STRPTR)CLID_Hidd_Gfx, moHidd_Gfx_GetMode);
                    OOP_DoMethod(sd->AtiObject, (OOP_Msg)getmodemsg);

                    OOP_GetAttr(sync, aHidd_Sync_PixelClock,    &pixel);
                    OOP_GetAttr(sync, aHidd_Sync_HDisp,         &hdisp);
                    OOP_GetAttr(sync, aHidd_Sync_VDisp,         &vdisp);
                    OOP_GetAttr(sync, aHidd_Sync_HSyncStart,    &hstart);
                    OOP_GetAttr(sync, aHidd_Sync_VSyncStart,    &vstart);
                    OOP_GetAttr(sync, aHidd_Sync_HSyncEnd,      &hend);
                    OOP_GetAttr(sync, aHidd_Sync_VSyncEnd,      &vend);
                    OOP_GetAttr(sync, aHidd_Sync_HTotal,        &htotal);
                    OOP_GetAttr(sync, aHidd_Sync_VTotal,        &vtotal);

                    bm->state = (struct CardState *)AllocPooled(sd->memPool,
                            sizeof(struct CardState));

                    pixel /= 1000;

                    if (bm->state)
                    {
                        LOCK_HW

                        InitMode(sd, bm->state, width, height, depth, pixel, bm->framebuffer,
                                    hdisp, vdisp,
                                    hstart, hend, htotal,
                                    vstart, vend, vtotal);

                        LoadState(sd, bm->state);
                        DPMS(sd, sd->dpms);

                        RADEONEngineReset(sd);
                        RADEONEngineRestore(sd);

                        UNLOCK_HW

                        return o;
                    }
                }
            }
        }
        else
        {
            if (bm->framebuffer == -1)
            {
            	int __tmp;
                bm->framebuffer = (IPTR)AllocMem(bm->pitch * bm->height,
                            MEMF_PUBLIC | MEMF_CLEAR);
                bm->fbgfx = FALSE;

                for (__tmp=0; __tmp < height; __tmp++)
                	bm->addresses[__tmp] = (void*)(bm->framebuffer + __tmp*bm->pitch);
            }
            else
                bm->fbgfx = TRUE;

            if ((bm->framebuffer != 0xffffffff) && (bm->framebuffer != 0))
            {
                return o;
            }
        }

        OOP_MethodID disp_mid = OOP_GetMethodID((STRPTR)IID_Root, moRoot_Dispose);
        OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);
    }

    return NULL;
}


VOID METHOD(ATIOffBM, Root, Dispose)
    __attribute__((alias(METHOD_NAME_S(ATIOnBM, Root, Dispose))));

VOID METHOD(ATIOnBM, Root, Dispose)
{
    atiBitMap *bm = OOP_INST_DATA(cl, o);

    LOCK_BITMAP
    LOCK_HW
//    NVDmaKickoff(&sd->Card);
    RADEONWaitForIdleMMIO(sd);

    if (bm->fbgfx)
    {
        FreeBitmapArea(sd, bm->framebuffer, bm->width, bm->height, bm->bpp);

        bm->framebuffer = -1;
        bm->fbgfx = 0;
    }
    else
        FreeMem((APTR)bm->framebuffer, bm->pitch * bm->height);

    FreeVecPooled(sd->memPool, bm->addresses);

    if (bm->state)
        FreePooled(sd->memPool, bm->state, sizeof(struct CardState));

    bm->state = NULL;

    UNLOCK_HW
    UNLOCK_BITMAP

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}


VOID METHOD(ATIOffBM, Root, Get)
    __attribute__((alias(METHOD_NAME_S(ATIOnBM, Root, Get))));

VOID METHOD(ATIOnBM, Root, Get)
{
    atiBitMap *bm = OOP_INST_DATA(cl, o);
    ULONG idx;

    if (IS_ATIBM_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            case aoHidd_ATIBitMap_Drawable:
                if (bm->fbgfx)
                    *msg->storage = bm->framebuffer + (IPTR)sd->Card.FrameBuffer;
                else
                    *msg->storage = bm->framebuffer;
                break;

            default:
                OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        }
    }
    else
    {
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
}


VOID METHOD(ATIOffBM, Hidd_BitMap, Clear)
    __attribute__((alias(METHOD_NAME_S(ATIOnBM, Hidd_BitMap, Clear))));

VOID METHOD(ATIOnBM, Hidd_BitMap, Clear)
{
    atiBitMap *bm = OOP_INST_DATA(cl, o);

    D(bug("[ATI] Clear(%p)\n",
        bm->framebuffer));

    LOCK_BITMAP

    if (bm->fbgfx)
    {
        LOCK_HW
        sd->Card.Busy = TRUE;
        bm->usecount++;

        RADEONWaitForFifo(sd, 1);
        OUTREG(RADEON_DST_PITCH_OFFSET, bm->pitch_offset);

        bm->dp_gui_master_cntl_clip = (bm->dp_gui_master_cntl
                                     | RADEON_GMC_BRUSH_SOLID_COLOR
                                     | RADEON_GMC_SRC_DATATYPE_COLOR
                                     | RADEON_ROP[vHidd_GC_DrawMode_Copy].pattern);

        RADEONWaitForFifo(sd, 4);

        OUTREG(RADEON_DP_GUI_MASTER_CNTL, bm->dp_gui_master_cntl_clip);
        OUTREG(RADEON_DP_BRUSH_FRGD_CLR,  GC_BG(msg->gc));
        OUTREG(RADEON_DP_WRITE_MASK,      ~0);
        OUTREG(RADEON_DP_CNTL,            (RADEON_DST_X_LEFT_TO_RIGHT
                                          | RADEON_DST_Y_TOP_TO_BOTTOM));

        RADEONWaitForFifo(sd, 2);

        OUTREG(RADEON_DST_Y_X,          0);
        OUTREG(RADEON_DST_WIDTH_HEIGHT, (bm->width << 16) | (UWORD)bm->height);

        UNLOCK_HW
    }
    else
    {
        ULONG *ptr = (ULONG*)bm->framebuffer;
        ULONG val = 0;
        int i = (bm->pitch * bm->height) >> 2;

        switch (bm->bpp)
        {
            case 2:
                val = GC_BG(msg->gc) << 16 | (GC_BG(msg->gc) & 0xffff);
                break;

            default:
                val = GC_BG(msg->gc) << 16 | (GC_BG(msg->gc) & 0xffff);
                break;
        }

        do { *ptr++ = val; } while(--i);
    }

    UNLOCK_BITMAP
}

struct pHidd_BitMap_FillRect {
    struct pHidd_BitMap_DrawRect dr;
};

VOID METHOD(ATIOffBM, Hidd_BitMap, FillRect)
    __attribute__((alias(METHOD_NAME_S(ATIOnBM, Hidd_BitMap, FillRect))));

VOID METHOD(ATIOnBM, Hidd_BitMap, FillRect)
{
    OOP_Object *gc = msg->dr.gc;
    atiBitMap *bm = OOP_INST_DATA(cl, o);

    D(bug("[ATI] FillRect(%p, %d:%d - %d:%d)\n",
        bm->framebuffer, msg->dr.minX, msg->dr.minY, msg->dr.maxX, msg->dr.maxY));

    LOCK_BITMAP

    if (bm->fbgfx)
    {
        LOCK_HW
        sd->Card.Busy = TRUE;
        bm->usecount++;

        RADEONWaitForFifo(sd, 1);
        OUTREG(RADEON_DST_PITCH_OFFSET, bm->pitch_offset);

        bm->dp_gui_master_cntl_clip = (bm->dp_gui_master_cntl
                                     | RADEON_GMC_BRUSH_SOLID_COLOR
                                     | RADEON_GMC_SRC_DATATYPE_COLOR
                                     | RADEON_ROP[GC_DRMD(gc)].pattern);

        RADEONWaitForFifo(sd, 4);

        OUTREG(RADEON_DP_GUI_MASTER_CNTL, bm->dp_gui_master_cntl_clip);
        OUTREG(RADEON_DP_BRUSH_FRGD_CLR,  GC_FG(gc));
        OUTREG(RADEON_DP_WRITE_MASK,      ~0);
        OUTREG(RADEON_DP_CNTL,            (RADEON_DST_X_LEFT_TO_RIGHT
                                          | RADEON_DST_Y_TOP_TO_BOTTOM));

        RADEONWaitForFifo(sd, 2);

        OUTREG(RADEON_DST_Y_X,          (msg->dr.minY << 16) | (UWORD)msg->dr.minX);
        OUTREG(RADEON_DST_WIDTH_HEIGHT, ((msg->dr.maxX - msg->dr.minX + 1) << 16) | (UWORD)(msg->dr.maxY - msg->dr.minY + 1));

        UNLOCK_HW
    }
    else
    {
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }

    UNLOCK_BITMAP

}


VOID METHOD(ATIOffBM, Hidd_BitMap, DrawLine)
    __attribute__((alias(METHOD_NAME_S(ATIOnBM, Hidd_BitMap, DrawLine))));

VOID METHOD(ATIOnBM, Hidd_BitMap, DrawLine)
{
    OOP_Object *gc = msg->gc;
    atiBitMap *bm = OOP_INST_DATA(cl, o);

    D(bug("[ATI] DrawLine(%p, %d:%d - %d:%d) %08x\n",
        bm->framebuffer, msg->x1, msg->y1, msg->x2, msg->y2,GC_FG(gc)));

    LOCK_BITMAP

    if ((GC_LINEPAT(gc) =! (UWORD)~0) || !bm->fbgfx)
    {
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
    else
    {
        LOCK_HW
        sd->Card.Busy = TRUE;
        bm->usecount++;

        RADEONWaitForFifo(sd, 1);
        OUTREG(RADEON_DST_PITCH_OFFSET, bm->pitch_offset);

        bm->dp_gui_master_cntl_clip = (bm->dp_gui_master_cntl
                                     | RADEON_GMC_BRUSH_SOLID_COLOR
                                     | RADEON_GMC_SRC_DATATYPE_COLOR
                                     | RADEON_ROP[GC_DRMD(gc)].pattern);

        if (sd->Card.Type >= RV200) {
            RADEONWaitForFifo(sd, 1);
            OUTREG(RADEON_DST_LINE_PATCOUNT,
                      0x55 << RADEON_BRES_CNTL_SHIFT);
        }

        if (GC_DOCLIP(gc))
        {
            bm->dp_gui_master_cntl_clip |= RADEON_GMC_DST_CLIPPING;
            WORD x1,y1,x2,y2;
            x1 = GC_CLIPX1(gc);
            y1 = GC_CLIPY1(gc);
            x2 = GC_CLIPX2(gc) + 1;
            y2 = GC_CLIPY2(gc) + 1;

            if (x1 < 0)
            {
            	x1 = (-x1) & 0x3fff;
            	x1 |= RADEON_SC_SIGN_MASK_LO;
            }
            if (x2 < 0)
            {
            	x2 = (-x2) & 0x3fff;
            	x2 |= RADEON_SC_SIGN_MASK_LO;
            }
            if (y1 < 0)
            {
            	y1 = (-y1) & 0x3fff;
            	y1 |= RADEON_SC_SIGN_MASK_LO;
            }
            if (y2 < 0)
            {
            	y2 = (-y2) & 0x3fff;
            	y2 |= RADEON_SC_SIGN_MASK_LO;
            }

            RADEONWaitForFifo(sd, 5);
            OUTREG(RADEON_DP_GUI_MASTER_CNTL, bm->dp_gui_master_cntl_clip);
            OUTREG(RADEON_SC_TOP_LEFT,        (y1 << 16) | (UWORD)x1);
            OUTREG(RADEON_SC_BOTTOM_RIGHT,    (y2 << 16) | (UWORD)x2);
        }
        else
        {
            RADEONWaitForFifo(sd, 3);
            OUTREG(RADEON_DP_GUI_MASTER_CNTL, bm->dp_gui_master_cntl_clip);
        }
        OUTREG(RADEON_DP_BRUSH_FRGD_CLR,  GC_FG(gc));
        OUTREG(RADEON_DP_WRITE_MASK,      ~0);

        RADEONWaitForFifo(sd, 2);

        OUTREG(RADEON_DST_LINE_START, (msg->y1 << 16) | (UWORD)msg->x1);
        OUTREG(RADEON_DST_LINE_END,   (msg->y2 << 16) | (UWORD)msg->x2);
//        OUTREG(RADEON_DST_LINE_START, (msg->y2 << 16) | msg->x2);
//        OUTREG(RADEON_DST_LINE_END,   ((msg->y2+1) << 16) | (msg->x2+1));

        UNLOCK_HW
    }

    UNLOCK_BITMAP
}


VOID METHOD(ATIOffBM, Hidd_BitMap, DrawRect)
    __attribute__((alias(METHOD_NAME_S(ATIOnBM, Hidd_BitMap, DrawRect))));

VOID METHOD(ATIOnBM, Hidd_BitMap, DrawRect)
{
    OOP_Object *gc = msg->gc;
    atiBitMap *bm = OOP_INST_DATA(cl, o);
    UWORD addX, addY;

    D(bug("[ATI] DrawRect(%p, %d:%d - %d:%d)\n",
        bm->framebuffer, msg->minX, msg->minY, msg->maxX, msg->maxY));

    if (msg->minX == msg->maxX) addX = 1; else addX = 0;
    if (msg->minY == msg->maxY) addY = 1; else addY = 0;

    LOCK_BITMAP

    if ((GC_LINEPAT(gc) =! (UWORD)~0) || !bm->fbgfx)
    {
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
    else
    {
        LOCK_HW
        sd->Card.Busy = TRUE;
        bm->usecount++;

        RADEONWaitForFifo(sd, 1);
        OUTREG(RADEON_DST_PITCH_OFFSET, bm->pitch_offset);

        bm->dp_gui_master_cntl_clip = (bm->dp_gui_master_cntl
                                     | RADEON_GMC_BRUSH_SOLID_COLOR
                                     | RADEON_GMC_SRC_DATATYPE_COLOR
                                     | RADEON_ROP[GC_DRMD(gc)].pattern);

        if (sd->Card.Type >= RV200) {
            RADEONWaitForFifo(sd, 1);
            OUTREG(RADEON_DST_LINE_PATCOUNT,
                      0x55 << RADEON_BRES_CNTL_SHIFT);
        }

        if (GC_DOCLIP(gc))
        {
            bm->dp_gui_master_cntl_clip |= RADEON_GMC_DST_CLIPPING;
            UWORD x1,y1,x2,y2;
            x1 = GC_CLIPX1(gc);
            y1 = GC_CLIPY1(gc);
            x2 = GC_CLIPX2(gc) + 1;
            y2 = GC_CLIPY2(gc) + 1;

            if (x1 < 0)
            {
            	x1 = (-x1) & 0x3fff;
            	x1 |= RADEON_SC_SIGN_MASK_LO;
            }
            if (x2 < 0)
            {
            	x2 = (-x2) & 0x3fff;
            	x2 |= RADEON_SC_SIGN_MASK_LO;
            }
            if (y1 < 0)
            {
            	y1 = (-y1) & 0x3fff;
            	y1 |= RADEON_SC_SIGN_MASK_LO;
            }
            if (y2 < 0)
            {
            	y2 = (-y2) & 0x3fff;
            	y2 |= RADEON_SC_SIGN_MASK_LO;
            }

            RADEONWaitForFifo(sd, 5);
            OUTREG(RADEON_DP_GUI_MASTER_CNTL, bm->dp_gui_master_cntl_clip);
            OUTREG(RADEON_SC_TOP_LEFT,        (y1 << 16) | (UWORD)x1);
            OUTREG(RADEON_SC_BOTTOM_RIGHT,    (y2 << 16) | (UWORD)x2);
        }
        else
        {
        	RADEONWaitForFifo(sd, 3);
        	OUTREG(RADEON_DP_GUI_MASTER_CNTL, bm->dp_gui_master_cntl_clip);
        }
        OUTREG(RADEON_DP_BRUSH_FRGD_CLR,  GC_FG(gc));
        OUTREG(RADEON_DP_WRITE_MASK,      ~0);

        RADEONWaitForFifo(sd, 8);

        OUTREG(RADEON_DST_LINE_START, (msg->minY << 16) | (msg->minX & 0xffff));
        OUTREG(RADEON_DST_LINE_END,   (msg->minY << 16) | (msg->maxX & 0xffff));

        OUTREG(RADEON_DST_LINE_START, ((msg->minY + addY) << 16) | (msg->maxX & 0xffff));
        OUTREG(RADEON_DST_LINE_END,   ((msg->maxY << 16)) | (msg->maxX & 0xffff));

        OUTREG(RADEON_DST_LINE_START, ((msg->maxY << 16)) | ((msg->maxX - addX) & 0xffff));
        OUTREG(RADEON_DST_LINE_END,   ((msg->maxY << 16)) | ((msg->minX) & 0xffff));

        OUTREG(RADEON_DST_LINE_START, ((msg->maxY - addY) << 16) | (msg->minX & 0xffff));
        OUTREG(RADEON_DST_LINE_END,   ((msg->minY + addY) << 16) | (msg->minX & 0xffff));

        UNLOCK_HW
    }

    UNLOCK_BITMAP
}


VOID METHOD(ATIOffBM, Hidd_BitMap, DrawPolygon)
    __attribute__((alias(METHOD_NAME_S(ATIOnBM, Hidd_BitMap, DrawPolygon))));

VOID METHOD(ATIOnBM, Hidd_BitMap, DrawPolygon)
{
    OOP_Object *gc = msg->gc;
    atiBitMap *bm = OOP_INST_DATA(cl, o);
    ULONG i;

    D(bug("[ATI] DrawPolygon(%p)\n",
        bm->framebuffer));

    LOCK_BITMAP

    if ((GC_LINEPAT(gc) =! (UWORD)~0) || !bm->fbgfx)
    {
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
    else
    {
        LOCK_HW
        sd->Card.Busy = TRUE;
        bm->usecount++;

        RADEONWaitForFifo(sd, 1);
        OUTREG(RADEON_DST_PITCH_OFFSET, bm->pitch_offset);

        bm->dp_gui_master_cntl_clip = (bm->dp_gui_master_cntl
                                     | RADEON_GMC_BRUSH_SOLID_COLOR
                                     | RADEON_GMC_SRC_DATATYPE_COLOR
                                     | RADEON_ROP[GC_DRMD(gc)].pattern);

        if (sd->Card.Type >= RV200) {
            RADEONWaitForFifo(sd, 1);
            OUTREG(RADEON_DST_LINE_PATCOUNT,
                      0x55 << RADEON_BRES_CNTL_SHIFT);
        }

        if (GC_DOCLIP(gc))
        {
            bm->dp_gui_master_cntl_clip |= RADEON_GMC_DST_CLIPPING;
            UWORD x1,y1,x2,y2;
            x1 = GC_CLIPX1(gc);
            y1 = GC_CLIPY1(gc);
            x2 = GC_CLIPX2(gc) + 1;
            y2 = GC_CLIPY2(gc) + 1;

            if (x1 < 0)
            {
            	x1 = (-x1) & 0x3fff;
            	x1 |= RADEON_SC_SIGN_MASK_LO;
            }
            if (x2 < 0)
            {
            	x2 = (-x2) & 0x3fff;
            	x2 |= RADEON_SC_SIGN_MASK_LO;
            }
            if (y1 < 0)
            {
            	y1 = (-y1) & 0x3fff;
            	y1 |= RADEON_SC_SIGN_MASK_LO;
            }
            if (y2 < 0)
            {
            	y2 = (-y2) & 0x3fff;
            	y2 |= RADEON_SC_SIGN_MASK_LO;
            }

            RADEONWaitForFifo(sd, 5);
            OUTREG(RADEON_DP_GUI_MASTER_CNTL, bm->dp_gui_master_cntl_clip);
            OUTREG(RADEON_SC_TOP_LEFT,        (y1 << 16) | (UWORD)x1);
            OUTREG(RADEON_SC_BOTTOM_RIGHT,    (y2 << 16) | (UWORD)x2);
        }
        else
        {
            RADEONWaitForFifo(sd, 3);
            OUTREG(RADEON_DP_GUI_MASTER_CNTL, bm->dp_gui_master_cntl_clip);
        }
        OUTREG(RADEON_DP_BRUSH_FRGD_CLR,  GC_FG(gc));
        OUTREG(RADEON_DP_WRITE_MASK,      ~0);

        for (i = 2; i < (2 * msg->n); i+=2)
        {
            RADEONWaitForFifo(sd, 2);
            OUTREG(RADEON_DST_LINE_START, (msg->coords[i-1] << 16) | (UWORD)msg->coords[i-2]);
            OUTREG(RADEON_DST_LINE_END,   (msg->coords[i+1] << 16) | (UWORD)msg->coords[i]);
        }

        UNLOCK_HW
    }

    UNLOCK_BITMAP
}

VOID METHOD(ATIOffBM, Hidd_BitMap, PutPixel)
    __attribute__((alias(METHOD_NAME_S(ATIOnBM, Hidd_BitMap, PutPixel))));

VOID METHOD(ATIOnBM, Hidd_BitMap, PutPixel)
{
    atiBitMap *bm = OOP_INST_DATA(cl, o);
    void *ptr = bm->addresses[msg->y];

    LOCK_BITMAP

    if (bm->fbgfx)
    {
        if (sd->Card.Busy)
        {
            LOCK_HW
/* TODO: NVSync(sd) */
            RADEONWaitForIdleMMIO(sd);
            UNLOCK_HW
        }
    }


    switch (bm->bpp)
    {
        case 1:
            ((UBYTE *)ptr)[msg->x] = msg->pixel;
            break;
        case 2:
            ((UWORD *)ptr)[msg->x] = msg->pixel;
            break;
        case 4:
            ((ULONG *)ptr)[msg->x] = msg->pixel;
            break;
    }

    UNLOCK_BITMAP
}

VOID METHOD(ATIOffBM, Hidd_BitMap, DrawPixel)
    __attribute__((alias(METHOD_NAME_S(ATIOnBM, Hidd_BitMap, DrawPixel))));

VOID METHOD(ATIOnBM, Hidd_BitMap, DrawPixel)
{
    atiBitMap *bm = OOP_INST_DATA(cl, o);
    void *ptr = bm->addresses[msg->y];
	OOP_Object *gc = msg->gc;

    HIDDT_Pixel     	    	    src, dest = 0, val;
    HIDDT_DrawMode  	    	    mode;
    HIDDT_Pixel     	    	    writeMask;

    src       = GC_FG(gc);
    mode      = GC_DRMD(gc);

    LOCK_BITMAP

    if (bm->fbgfx)
    {
        if (sd->Card.Busy)
        {
            LOCK_HW
/* TODO: NVSync(sd) */
            RADEONWaitForIdleMMIO(sd);
            UNLOCK_HW
        }
    }

    if (vHidd_GC_DrawMode_Copy == mode && GC_COLMASK(gc) == ~0)
	{
		val = src;
	}
	else
	{
		switch (bm->bpp)
	    {
	        case 1:
	            dest = ((UBYTE *)ptr)[msg->x];
	            break;
	        case 2:
	            dest = ((UWORD *)ptr)[msg->x];
	            break;
	        case 4:
	            dest = ((ULONG *)ptr)[msg->x];
	            break;
	    }

		writeMask = ~GC_COLMASK(gc) & dest;

		val = 0;

		if(mode & 1) val = ( src &  dest);
		if(mode & 2) val = ( src & ~dest) | val;
		if(mode & 4) val = (~src &  dest) | val;
		if(mode & 8) val = (~src & ~dest) | val;

		val = (val & (writeMask | GC_COLMASK(gc) )) | writeMask;
	}

	switch (bm->bpp)
    {
        case 1:
            ((UBYTE *)ptr)[msg->x] = val;
            break;
        case 2:
            ((UWORD *)ptr)[msg->x] = val;
            break;
        case 4:
            ((ULONG *)ptr)[msg->x] = val;
            break;
    }

    UNLOCK_BITMAP
}

VOID METHOD(ATIOffBM, Hidd_BitMap, DrawEllipse)
    __attribute__((alias(METHOD_NAME_S(ATIOnBM, Hidd_BitMap, DrawEllipse))));

VOID METHOD(ATIOnBM, Hidd_BitMap, DrawEllipse)
{
	atiBitMap *bm = OOP_INST_DATA(cl, o);
	OOP_Object *gc = msg->gc;
	WORD    	x = msg->rx, y = 0;     /* ellipse points */
	HIDDT_Pixel     	    	    src;
	HIDDT_DrawMode  	    	    mode;
	HIDDT_Pixel     	    	    writeMask;

	/* intermediate terms to speed up loop */
	LONG    	t1 = msg->rx * msg->rx, t2 = t1 << 1, t3 = t2 << 1;
	LONG    	t4 = msg->ry * msg->ry, t5 = t4 << 1, t6 = t5 << 1;
	LONG    	t7 = msg->rx * t5, t8 = t7 << 1, t9 = 0L;
	LONG    	d1 = t2 - t7 + (t4 >> 1);    /* error terms */
	LONG    	d2 = (t1 >> 1) - t8 + t5;

	APTR    	doclip = GC_DOCLIP(gc);

    src       = GC_FG(gc);
    mode      = GC_DRMD(gc);

	void _drawpixel(int x, int y)
	{
		void *ptr = bm->addresses[y];
		HIDDT_Pixel val, dest = 0;

		if (vHidd_GC_DrawMode_Copy == mode && GC_COLMASK(gc) == ~0)
		{
			val = src;
		}
		else
		{
			switch (bm->bpp)
		    {
		        case 1:
		            dest = ((UBYTE *)ptr)[x];
		            break;
		        case 2:
		            dest = ((UWORD *)ptr)[x];
		            break;
		        case 4:
		            dest = ((ULONG *)ptr)[x];
		            break;
		    }

			writeMask = ~GC_COLMASK(gc) & dest;

			val = 0;

			if(mode & 1) val = ( src &  dest);
			if(mode & 2) val = ( src & ~dest) | val;
			if(mode & 4) val = (~src &  dest) | val;
			if(mode & 8) val = (~src & ~dest) | val;

			val = (val & (writeMask | GC_COLMASK(gc) )) | writeMask;
		}

		switch (bm->bpp)
	    {
	        case 1:
	            ((UBYTE *)ptr)[x] = val;
	            break;
	        case 2:
	            ((UWORD *)ptr)[x] = val;
	            break;
	        case 4:
	            ((ULONG *)ptr)[x] = val;
	            break;
	    }
	}

	LOCK_BITMAP

	UBYTE *ptr = (UBYTE*)((IPTR)bm->framebuffer);
    if (bm->fbgfx)
    {
        ptr += (IPTR)sd->Card.FrameBuffer;
        if (sd->Card.Busy)
        {
            LOCK_HW
/* TODO: NVSync(sd) */
            RADEONWaitForIdleMMIO(sd);
            UNLOCK_HW
        }
    }

    while (d2 < 0)                  /* til slope = -1 */
    {
    	/* draw 4 points using symmetry */

    	if  (doclip)
    	{
    		if (!POINT_OUTSIDE_CLIP(gc, msg->x + x, msg->y + y))
    			_drawpixel(msg->x + x, msg->y + y);

    		if (!POINT_OUTSIDE_CLIP(gc, msg->x + x, msg->y - y))
    			_drawpixel(msg->x + x, msg->y - y);

    		if (!POINT_OUTSIDE_CLIP(gc, msg->x - x, msg->y + y))
    			_drawpixel(msg->x - x, msg->y + y);

    		if (!POINT_OUTSIDE_CLIP(gc, msg->x - x, msg->y - y))
    			_drawpixel(msg->x - x, msg->y - y);
    	}
    	else
    	{
    		_drawpixel(msg->x + x, msg->y + y);
    		_drawpixel(msg->x + x, msg->y - y);
    		_drawpixel(msg->x - x, msg->y + y);
    		_drawpixel(msg->x - x, msg->y - y);
    	}

    	y++;            /* always move up here */
    	t9 = t9 + t3;
    	if (d1 < 0)     /* move straight up */
    	{
    		d1 = d1 + t9 + t2;
    		d2 = d2 + t9;
    	}
    	else            /* move up and left */
    	{
    		x--;
    		t8 = t8 - t6;
    		d1 = d1 + t9 + t2 - t8;
    		d2 = d2 + t9 + t5 - t8;
    	}
    }

    do                              /* rest of top right quadrant */
    {
    	/* draw 4 points using symmetry */
    	if  (doclip)
    	{
    		if (!POINT_OUTSIDE_CLIP(gc, msg->x + x, msg->y + y))
    			_drawpixel(msg->x + x, msg->y + y);

    		if (!POINT_OUTSIDE_CLIP(gc, msg->x + x, msg->y - y))
    			_drawpixel(msg->x + x, msg->y - y);

    		if (!POINT_OUTSIDE_CLIP(gc, msg->x - x, msg->y + y))
    			_drawpixel(msg->x - x, msg->y + y);

    		if (!POINT_OUTSIDE_CLIP(gc, msg->x - x, msg->y - y))
    			_drawpixel(msg->x - x, msg->y - y);
    	}
    	else
    	{
    		_drawpixel(msg->x + x, msg->y + y);
    		_drawpixel(msg->x + x, msg->y - y);
    		_drawpixel(msg->x - x, msg->y + y);
    		_drawpixel(msg->x - x, msg->y - y);
    	}

    	x--;            /* always move left here */
    	t8 = t8 - t6;
    	if (d2 < 0)     /* move up and left */
    	{
    		y++;
    		t9 = t9 + t3;
    		d2 = d2 + t9 + t5 - t8;
    	}
    	else            /* move straight left */
    	{
    		d2 = d2 + t5 - t8;
    	}

    } while (x >= 0);


    UNLOCK_BITMAP
}

HIDDT_Pixel METHOD(ATIOffBM, Hidd_BitMap, GetPixel)
    __attribute__((alias(METHOD_NAME_S(ATIOnBM, Hidd_BitMap, GetPixel))));

HIDDT_Pixel METHOD(ATIOnBM, Hidd_BitMap, GetPixel)
{
    HIDDT_Pixel pixel=0;
    atiBitMap *bm = OOP_INST_DATA(cl, o);

    LOCK_BITMAP

    void *ptr = bm->addresses[msg->y];

    if (bm->fbgfx)
    {
        if (sd->Card.Busy)
        {
            LOCK_HW
/* TODO: NVSync(sd) */
            RADEONWaitForIdleMMIO(sd);
            UNLOCK_HW
        }
    }

    switch (bm->bpp)
    {
        case 1:
            pixel = ((UBYTE *)ptr)[msg->x];
            break;
        case 2:
            pixel = ((UWORD *)ptr)[msg->x];
            break;
        case 4:
            pixel = ((ULONG *)ptr)[msg->x];
            break;
    }

    UNLOCK_BITMAP

    /* Get pen number from colortab */
    return pixel;
}


void METHOD(ATIOffBM, Hidd_BitMap, BlitColorExpansion)
    __attribute__((alias(METHOD_NAME_S(ATIOnBM, Hidd_BitMap, BlitColorExpansion))));

void METHOD(ATIOnBM, Hidd_BitMap, BlitColorExpansion)
{
    atiBitMap *bm = OOP_INST_DATA(cl, o);

    LOCK_BITMAP

    if ((OOP_OCLASS(msg->srcBitMap) == sd->PlanarBMClass) && bm->fbgfx)
    {
    	struct planarbm_data    *planar = OOP_INST_DATA(OOP_OCLASS(msg->srcBitMap), msg->srcBitMap);
    	HIDDT_Pixel             bg, fg;
    	ULONG                   cemd;
    	ULONG                   skipleft = msg->srcX - (msg->srcX & ~31);
    	ULONG                   mask = ~0 << bm->depth;

	if (bm->depth == 32)
	    mask = 0xff000000;

    	cemd = GC_COLEXP(msg->gc);
    	bg   = GC_BG(msg->gc) | mask;
    	fg   = GC_FG(msg->gc) | mask;

    	ULONG bw = (msg->width + 31 + skipleft) & ~31;
    	LONG x = msg->destX, y = msg->destY, w = msg->width, h = msg->height;

    	LOCK_HW

        bm->usecount++;
    	sd->Card.Busy = TRUE;

    	RADEONWaitForFifo(sd, 1);
    	OUTREG(RADEON_DST_PITCH_OFFSET, bm->pitch_offset);

        bm->dp_gui_master_cntl_clip = (bm->dp_gui_master_cntl
									 | RADEON_GMC_WR_MSK_DIS
                                     | RADEON_GMC_BRUSH_NONE
                                     | RADEON_DP_SRC_SOURCE_HOST_DATA
                                     | RADEON_GMC_DST_CLIPPING
                                     | RADEON_GMC_BYTE_MSB_TO_LSB
                                     | RADEON_ROP[GC_DRMD(msg->gc)].rop);

        if (cemd & vHidd_GC_ColExp_Transparent)
        {
        	bm->dp_gui_master_cntl_clip |= RADEON_GMC_SRC_DATATYPE_MONO_FG_LA;

            RADEONWaitForFifo(sd, 6);
            OUTREG(RADEON_DP_GUI_MASTER_CNTL, bm->dp_gui_master_cntl_clip);
            OUTREG(RADEON_DP_SRC_FRGD_CLR,  fg);
        }
        else
        {
        	bm->dp_gui_master_cntl_clip |= RADEON_GMC_SRC_DATATYPE_MONO_FG_BG;

        	RADEONWaitForFifo(sd, 7);
            OUTREG(RADEON_DP_GUI_MASTER_CNTL, bm->dp_gui_master_cntl_clip);
            OUTREG(RADEON_DP_SRC_FRGD_CLR,  fg);
            OUTREG(RADEON_DP_SRC_BKGD_CLR,  bg);
        }

        OUTREG(RADEON_SC_TOP_LEFT,        (y << 16) | (UWORD)x);
		OUTREG(RADEON_SC_BOTTOM_RIGHT,    ((y+h) << 16) | (UWORD)(x+w));

        OUTREG(RADEON_DST_X_Y,          ((x - skipleft) << 16) | (UWORD)y);
        OUTREG(RADEON_DST_WIDTH_HEIGHT, (bw << 16) | (UWORD)h);

    	ULONG *ptr = (ULONG*)planar->planes[0];
    	ptr += ((msg->srcY * planar->bytesperrow) >> 2) + (msg->srcX >> 5);

#if AROS_BIG_ENDIAN
        RADEONWaitForFifo(sd, 1);
        OUTREG(RADEON_RBBM_GUICNTL, RADEON_HOST_DATA_SWAP_32BIT);
#endif

        while(h--)
        {
        	int i;

        	for (i=0; i < bw >> 5; i++)
        	{
                RADEONWaitForFifo(sd, 1);
                OUTREG(RADEON_HOST_DATA0, ptr[i]);
        	}

        	ptr += planar->bytesperrow >> 2;
        }

#if AROS_BIG_ENDIAN
        RADEONWaitForFifo(sd, 1);
        OUTREG(RADEON_RBBM_GUICNTL, RADEON_HOST_DATA_SWAP_NONE);
#endif

    	UNLOCK_HW

    }
    else
    	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    UNLOCK_BITMAP

}

ULONG METHOD(ATIOffBM, Hidd_BitMap, BytesPerLine)
    __attribute__((alias(METHOD_NAME_S(ATIOnBM, Hidd_BitMap, BytesPerLine))));

ULONG METHOD(ATIOnBM, Hidd_BitMap, BytesPerLine)
{
    atiBitMap *bm = OOP_INST_DATA(cl, o);

    return (bm->bpp * msg->width + 255) & ~255;
}


BOOL METHOD(ATIOffBM, Hidd_BitMap, ObtainDirectAccess)
    __attribute__((alias(METHOD_NAME_S(ATIOnBM, Hidd_BitMap, ObtainDirectAccess))));

BOOL METHOD(ATIOnBM, Hidd_BitMap, ObtainDirectAccess)
{
    atiBitMap *bm = OOP_INST_DATA(cl, o);
    LOCK_BITMAP

    IPTR VideoData = bm->framebuffer;

    if (bm->fbgfx)
    {
        VideoData += (IPTR)sd->Card.FrameBuffer;
        if (sd->Card.Busy)
        {
            LOCK_HW
/* TODO: NVSync(sd) */
            RADEONWaitForIdleMMIO(sd);
            UNLOCK_HW
        }
    }

    *msg->addressReturn = (UBYTE*)VideoData;
    *msg->widthReturn = bm->pitch / bm->bpp;
    *msg->heightReturn = bm->height;
    *msg->bankSizeReturn = *msg->memSizeReturn = bm->pitch * bm->height;

    return TRUE;
}

VOID METHOD(ATIOffBM, Hidd_BitMap, ReleaseDirectAccess)
    __attribute__((alias(METHOD_NAME_S(ATIOnBM, Hidd_BitMap, ReleaseDirectAccess))));

VOID METHOD(ATIOnBM, Hidd_BitMap, ReleaseDirectAccess)
{
    atiBitMap *bm = OOP_INST_DATA(cl, o);

    UNLOCK_BITMAP
}


/*
 * Unaccelerated methods
 */

VOID METHOD(ATIOffBM, Hidd_BitMap, PutImageLUT)
    __attribute__((alias(METHOD_NAME_S(ATIOnBM, Hidd_BitMap, PutImageLUT))));

VOID METHOD(ATIOnBM, Hidd_BitMap, PutImageLUT)
{
    atiBitMap *bm = OOP_INST_DATA(cl, o);

    LOCK_BITMAP

    if (bm->fbgfx)
    {

    	UBYTE *src = msg->pixels;
    	ULONG x_add = msg->modulo;
    	UWORD height = msg->height;
    	UWORD bw = msg->width;
		HIDDT_Pixel *colmap = msg->pixlut->pixels;

    	if (bm->bpp == 2)
    		bw = (bw + 1) & ~1;

		LOCK_HW

        bm->usecount++;
    	sd->Card.Busy = TRUE;

		RADEONWaitForFifo(sd, 1);
		OUTREG(RADEON_DST_PITCH_OFFSET, bm->pitch_offset);

		bm->dp_gui_master_cntl_clip = (bm->dp_gui_master_cntl
									 | RADEON_GMC_WR_MSK_DIS
									 | RADEON_GMC_BRUSH_NONE
									 | RADEON_DP_SRC_SOURCE_HOST_DATA
									 | RADEON_GMC_DST_CLIPPING
									 | RADEON_GMC_SRC_DATATYPE_COLOR
									 | RADEON_ROP[vHidd_GC_DrawMode_Copy].rop);

		RADEONWaitForFifo(sd, 5);
		OUTREG(RADEON_DP_GUI_MASTER_CNTL, bm->dp_gui_master_cntl_clip);

		OUTREG(RADEON_SC_TOP_LEFT,        (msg->y << 16) | (UWORD)msg->x);
		OUTREG(RADEON_SC_BOTTOM_RIGHT,    ((msg->y+msg->height) << 16) | (UWORD)(msg->x+msg->width));

		OUTREG(RADEON_DST_X_Y,          ((msg->x) << 16) | (UWORD)msg->y);
		OUTREG(RADEON_DST_WIDTH_HEIGHT, (bw << 16) | (UWORD)msg->height);

		if (bm->bpp == 4)
		{
#if AROS_BIG_ENDIAN
        	RADEONWaitForFifo(sd, 1);
        	OUTREG(RADEON_RBBM_GUICNTL, RADEON_HOST_DATA_SWAP_32BIT);
#endif
			while (height--)
			{
				UBYTE *line = (UBYTE*)src;
				ULONG width = msg->width;

				while(width)
				{
					if (width <= 8)
					{
						RADEONWaitForFifo(sd, width);
						switch (width)
						{
							case 8: OUTREGN(RADEON_HOST_DATA0, colmap[*line++]);
							case 7: OUTREGN(RADEON_HOST_DATA1, colmap[*line++]);
							case 6: OUTREGN(RADEON_HOST_DATA2, colmap[*line++]);
							case 5: OUTREGN(RADEON_HOST_DATA3, colmap[*line++]);
							case 4: OUTREGN(RADEON_HOST_DATA4, colmap[*line++]);
							case 3: OUTREGN(RADEON_HOST_DATA5, colmap[*line++]);
							case 2: OUTREGN(RADEON_HOST_DATA6, colmap[*line++]);
							case 1: OUTREGN(RADEON_HOST_DATA7, colmap[*line++]);
						}
						width = 0;
					}
					else
					{
						RADEONWaitForFifo(sd, 8);

						OUTREGN(RADEON_HOST_DATA0, colmap[*line++]);
						OUTREGN(RADEON_HOST_DATA1, colmap[*line++]);
						OUTREGN(RADEON_HOST_DATA2, colmap[*line++]);
						OUTREGN(RADEON_HOST_DATA3, colmap[*line++]);
						OUTREGN(RADEON_HOST_DATA4, colmap[*line++]);
						OUTREGN(RADEON_HOST_DATA5, colmap[*line++]);
						OUTREGN(RADEON_HOST_DATA6, colmap[*line++]);
						OUTREGN(RADEON_HOST_DATA7, colmap[*line++]);

						width -= 8;
					}
				}

				src += x_add;
			}
		}
		else if (bm->bpp == 2)
		{
#if AROS_BIG_ENDIAN
			RADEONWaitForFifo(sd, 1);
			OUTREG(RADEON_RBBM_GUICNTL, RADEON_HOST_DATA_SWAP_HDW);
#endif
			while (height--)
			{
				UBYTE *line = (UBYTE*)src;
				ULONG width = bw >> 1;

				while(width--)
				{
					ULONG tmp = (colmap[line[0]] << 16) | (colmap[line[1]] & 0x0000ffff);
					RADEONWaitForFifo(sd, 1);
					OUTREG(RADEON_HOST_DATA0, tmp);
					line+=2;
				}

				src += x_add;
			}

		}

#if AROS_BIG_ENDIAN
		RADEONWaitForFifo(sd, 1);
		OUTREG(RADEON_RBBM_GUICNTL, RADEON_HOST_DATA_SWAP_NONE);
#endif

		UNLOCK_HW
    }
    else
    	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    UNLOCK_BITMAP
}

static inline int do_alpha(int a, int v)
{
	int tmp = a*v;
	return ((tmp << 8) + tmp + 32768) >> 16;
}

VOID METHOD(ATIOffBM, Hidd_BitMap, PutAlphaImage)
    __attribute__((alias(METHOD_NAME_S(ATIOnBM, Hidd_BitMap, PutAlphaImage))));

VOID METHOD(ATIOnBM, Hidd_BitMap, PutAlphaImage)
{
    atiBitMap *bm = OOP_INST_DATA(cl, o);

    LOCK_BITMAP

    /* Try to PutAlphaImage with 2D engine first */
    if (bm->fbgfx)
    {
    	ULONG x_add = (msg->modulo - msg->width * 4) >> 2;
    	UWORD height = msg->height;
    	UWORD bw = msg->width;
        ULONG *pixarray = (ULONG *)msg->pixels;
        ULONG y = msg->y;
        ULONG x;

D(bug("ATI: PutAlphaImage(%d, %d, %d:%d)\n", msg->x, msg->y, msg->width, msg->height));

		/* We're not going to use the 2D engine now. Therefore, flush the chip */
        if (sd->Card.Busy)
        {
            LOCK_HW
            RADEONWaitForIdleMMIO(sd);
            UNLOCK_HW
        }

        /*
         * Treat each depth case separately
         */
        if (bm->bpp == 4)
        {
        	while(height--)
        	{
        		ULONG *xbuf = bm->addresses[y];
        		xbuf += msg->x;

        		for (x=0; x < bw; x++)
        		{
        			ULONG       destpix;
        			ULONG       srcpix;
        			LONG        src_red, src_green, src_blue, src_alpha;
        			LONG        dst_red, dst_green, dst_blue;

					/* Read RGBA pixel from input array */
        			srcpix = *pixarray++;
#if AROS_BIG_ENDIAN
        			src_red   = (srcpix & 0x00FF0000) >> 16;
        			src_green = (srcpix & 0x0000FF00) >> 8;
        			src_blue  = (srcpix & 0x000000FF);
        			src_alpha = (srcpix & 0xFF000000) >> 24;
#else
        			src_red   = (srcpix & 0x0000FF00) >> 8;
        			src_green = (srcpix & 0x00FF0000) >> 16;
        			src_blue  = (srcpix & 0xFF000000) >> 24;
        			src_alpha = (srcpix & 0x000000FF);
#endif

        			/*
        			 * If alpha=0, do not change the destination pixel at all.
        			 * This saves us unnecessary reads and writes to VRAM.
        			 */
					if (src_alpha != 0)
					{
						/*
						 * Full opacity. Do not read the destination pixel, as
						 * it's value does not matter anyway.
						 */
						if (src_alpha == 0xff)
						{
							dst_red = src_red;
							dst_green = src_green;
							dst_blue = src_blue;
						}
						else
						{
							/*
							 * Alpha blending with source and destination pixels.
							 * Get destination.
							 */
							destpix = xbuf[x];

//					#if AROS_BIG_ENDIAN
//							dst_red   = (destpix & 0x0000FF00) >> 8;
//							dst_green = (destpix & 0x00FF0000) >> 16;
//							dst_blue  = (destpix & 0xFF000000) >> 24;
//					#else
							dst_red   = (destpix & 0x00FF0000) >> 16;
							dst_green = (destpix & 0x0000FF00) >> 8;
							dst_blue  = (destpix & 0x000000FF);
//					#endif

							dst_red   += do_alpha(src_alpha, src_red - dst_red);
							dst_green += do_alpha(src_alpha, src_green - dst_green);
							dst_blue  += do_alpha(src_alpha, src_blue - dst_blue);

						}

//					#if AROS_BIG_ENDIAN
//                    destpix = (dst_blue << 24) + (dst_green << 16) + (dst_red << 8);
//                #else
						destpix = (dst_red << 16) + (dst_green << 8) + (dst_blue);
//                #endif

						/* Store the new pixel */
						xbuf[x] = destpix;
					}
        		}

        		y++;
        		pixarray += x_add;
        	}
        }
        /* 2bpp cases... */
        else if (bm->bpp == 2)
        {
        	if (bm->depth == 16)
        	{
				while(height--)
				{
					UWORD *xbuf = bm->addresses[y];
					xbuf += msg->x;

					for (x=0; x < bw; x++)
					{
						UWORD       destpix;
						ULONG       srcpix;
						LONG        src_red, src_green, src_blue, src_alpha;
						LONG        dst_red, dst_green, dst_blue;

						srcpix = *pixarray++;
#if AROS_BIG_ENDIAN
						src_red   = (srcpix & 0x00FF0000) >> 16;
						src_green = (srcpix & 0x0000FF00) >> 8;
						src_blue  = (srcpix & 0x000000FF);
						src_alpha = (srcpix & 0xFF000000) >> 24;
#else
						src_red   = (srcpix & 0x0000FF00) >> 8;
						src_green = (srcpix & 0x00FF0000) >> 16;
						src_blue  = (srcpix & 0xFF000000) >> 24;
						src_alpha = (srcpix & 0x000000FF);
#endif

						/*
						 * If alpha=0, do not change the destination pixel at all.
						 * This saves us unnecessary reads and writes to VRAM.
						 */
						if (src_alpha != 0)
						{
							/*
							 * Full opacity. Do not read the destination pixel, as
							 * it's value does not matter anyway.
							 */
							if (src_alpha == 0xff)
							{
								dst_red = src_red;
								dst_green = src_green;
								dst_blue = src_blue;
							}
							else
							{
								/*
								 * Alpha blending with source and destination pixels.
								 * Get destination.
								 */

								destpix = xbuf[x];

								dst_red   = (destpix & 0x0000F800) >> 8;
								dst_green = (destpix & 0x000007e0) >> 3;
								dst_blue  = (destpix & 0x0000001f) << 3;

								dst_red   += do_alpha(src_alpha, src_red - dst_red);
								dst_green += do_alpha(src_alpha, src_green - dst_green);
								dst_blue  += do_alpha(src_alpha, src_blue - dst_blue);
							}

							destpix = (((dst_red << 8) & 0xf800) | ((dst_green << 3) & 0x07e0) | ((dst_blue >> 3) & 0x001f));

							xbuf[x] = destpix;
						}
					}

					y++;
					pixarray += x_add;
				}
        	}
        	else if (bm->depth == 15)
        	{
        		while(height--)
        		{
					UWORD *xbuf = bm->addresses[y];
					xbuf += msg->x;

					for (x=0; x < bw; x++)
					{
						UWORD       destpix = 0;
						ULONG       srcpix;
						LONG        src_red, src_green, src_blue, src_alpha;
						LONG        dst_red, dst_green, dst_blue;

						srcpix = *pixarray++;
#if AROS_BIG_ENDIAN
						src_red   = (srcpix & 0x00FF0000) >> 16;
						src_green = (srcpix & 0x0000FF00) >> 8;
						src_blue  = (srcpix & 0x000000FF);
						src_alpha = (srcpix & 0xFF000000) >> 24;
#else
						src_red   = (srcpix & 0x0000FF00) >> 8;
						src_green = (srcpix & 0x00FF0000) >> 16;
						src_blue  = (srcpix & 0xFF000000) >> 24;
						src_alpha = (srcpix & 0x000000FF);
#endif
						/*
						 * If alpha=0, do not change the destination pixel at all.
						 * This saves us unnecessary reads and writes to VRAM.
						 */
						if (src_alpha != 0)
						{
							/*
							 * Full opacity. Do not read the destination pixel, as
							 * it's value does not matter anyway.
							 */
							if (src_alpha == 0xff)
							{
								dst_red = src_red;
								dst_green = src_green;
								dst_blue = src_blue;
							}
							else
							{
								/*
								 * Alpha blending with source and destination pixels.
								 * Get destination.
								 */

								destpix = xbuf[x];

								dst_red   = (destpix & 0x00007c00) >> 7;
								dst_green = (destpix & 0x000003e0) >> 2;
								dst_blue  = (destpix & 0x0000001f) << 3;

								dst_red   += do_alpha(src_alpha, src_red - dst_red);
								dst_green += do_alpha(src_alpha, src_green - dst_green);
								dst_blue  += do_alpha(src_alpha, src_blue - dst_blue);

								destpix = (ULONG)(((dst_red << 7) & 0x7c00) | ((dst_green << 2) & 0x03e0) | ((dst_blue >> 3) & 0x001f));
							}

							xbuf[x] = destpix;
						}
					}

					y++;
					pixarray += x_add;
        		}
        	}
        	else
            	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

        }
        else
        	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
    else
    	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    UNLOCK_BITMAP
}

VOID METHOD(ATIOffBM, Hidd_BitMap, PutImage)
    __attribute__((alias(METHOD_NAME_S(ATIOnBM, Hidd_BitMap, PutImage))));

VOID METHOD(ATIOnBM, Hidd_BitMap, PutImage)
{
    atiBitMap *bm = OOP_INST_DATA(cl, o);
    BOOL done = FALSE;

    LOCK_BITMAP

    IPTR VideoData = bm->framebuffer;

    /* Try to PutImage with 2D engine first */
    if (bm->fbgfx)
    {
    	UBYTE *src = msg->pixels;
    	ULONG x_add = msg->modulo;
    	UWORD height = msg->height;
    	UWORD bw = msg->width;

    	if (bm->bpp == 2)
    		bw = (bw + 1) & ~1;

    	done = TRUE;

    	if (done)
    	{
			LOCK_HW

	        bm->usecount++;
	    	sd->Card.Busy = TRUE;

			RADEONWaitForFifo(sd, 1);
			OUTREG(RADEON_DST_PITCH_OFFSET, bm->pitch_offset);

			bm->dp_gui_master_cntl_clip = (bm->dp_gui_master_cntl
										 | RADEON_GMC_WR_MSK_DIS
										 | RADEON_GMC_BRUSH_NONE
										 | RADEON_DP_SRC_SOURCE_HOST_DATA
										 | RADEON_GMC_DST_CLIPPING
										 | RADEON_GMC_SRC_DATATYPE_COLOR
										 | RADEON_ROP[vHidd_GC_DrawMode_Copy].rop);

			RADEONWaitForFifo(sd, 5);
			OUTREG(RADEON_DP_GUI_MASTER_CNTL, bm->dp_gui_master_cntl_clip);

			OUTREG(RADEON_SC_TOP_LEFT,        (msg->y << 16) | (UWORD)msg->x);
			OUTREG(RADEON_SC_BOTTOM_RIGHT,    ((msg->y+msg->height) << 16) | (UWORD)(msg->x+msg->width));

			OUTREG(RADEON_DST_X_Y,          ((msg->x) << 16) | (UWORD)msg->y);
			OUTREG(RADEON_DST_WIDTH_HEIGHT, (bw << 16) | (UWORD)msg->height);

			switch (msg->pixFmt)
			{
				case vHidd_StdPixFmt_Native32:
				case vHidd_StdPixFmt_Native:
					if (bm->bpp == 4)
					{
#if AROS_BIG_ENDIAN
			        	RADEONWaitForFifo(sd, 1);
			        	OUTREG(RADEON_RBBM_GUICNTL, RADEON_HOST_DATA_SWAP_32BIT);
#endif
						while (height--)
						{
							ULONG *line = (ULONG*)src;
							ULONG width = msg->width;

							while(width)
							{
								if (width <= 8)
								{
									RADEONWaitForFifo(sd, width);
									switch (width)
									{
										case 8: OUTREGN(RADEON_HOST_DATA0, *line++);
										case 7: OUTREGN(RADEON_HOST_DATA1, *line++);
										case 6: OUTREGN(RADEON_HOST_DATA2, *line++);
										case 5: OUTREGN(RADEON_HOST_DATA3, *line++);
										case 4: OUTREGN(RADEON_HOST_DATA4, *line++);
										case 3: OUTREGN(RADEON_HOST_DATA5, *line++);
										case 2: OUTREGN(RADEON_HOST_DATA6, *line++);
										case 1: OUTREGN(RADEON_HOST_DATA7, *line++);
									}
									width = 0;
								}
								else
								{
									RADEONWaitForFifo(sd, 8);

									OUTREGN(RADEON_HOST_DATA0, *line++);
									OUTREGN(RADEON_HOST_DATA1, *line++);
									OUTREGN(RADEON_HOST_DATA2, *line++);
									OUTREGN(RADEON_HOST_DATA3, *line++);
									OUTREGN(RADEON_HOST_DATA4, *line++);
									OUTREGN(RADEON_HOST_DATA5, *line++);
									OUTREGN(RADEON_HOST_DATA6, *line++);
									OUTREGN(RADEON_HOST_DATA7, *line++);

									width -= 8;
								}
							}

							src += x_add;
						}
#if AROS_BIG_ENDIAN
					RADEONWaitForFifo(sd, 1);
					OUTREG(RADEON_RBBM_GUICNTL, RADEON_HOST_DATA_SWAP_NONE);
#endif

					}
					else if (bm->bpp == 2)
					{
#if AROS_BIG_ENDIAN
						RADEONWaitForFifo(sd, 1);
						OUTREG(RADEON_RBBM_GUICNTL, RADEON_HOST_DATA_SWAP_HDW);
#endif
						if (msg->pixFmt == vHidd_StdPixFmt_Native)
						{
							while (height--)
							{
								ULONG *line = (ULONG*)src;
								ULONG width = bw >> 1;

								while(width--)
								{
									RADEONWaitForFifo(sd, 1);
									OUTREG(RADEON_HOST_DATA0, *line++);
								}

								src += x_add;
							}
						}
						else
						{
							while (height--)
							{
								ULONG *line = (ULONG*)src;
								ULONG width = bw >> 1;

								while(width--)
								{
									ULONG tmp = (line[0] << 16) | (line[1] & 0x0000ffff);
									RADEONWaitForFifo(sd, 1);
									OUTREG(RADEON_HOST_DATA0, tmp);
									line+=2;
								}

								src += x_add;
							}

						}
					}
#if AROS_BIG_ENDIAN
					RADEONWaitForFifo(sd, 1);
					OUTREG(RADEON_RBBM_GUICNTL, RADEON_HOST_DATA_SWAP_NONE);
#endif

					break;

				default:
				{
					HIDDT_PixelFormat *dstpf, *srcpf;

					srcpf = (HIDDT_PixelFormat *)HIDD_Gfx_GetPixFmt(sd->AtiObject, msg->pixFmt);
			        OOP_GetAttr(o, aHidd_BitMap_PixFmt, (APTR)&dstpf);

			        if (bm->bpp == 4)
			        {
#if AROS_BIG_ENDIAN
			        	RADEONWaitForFifo(sd, 1);
			        	OUTREG(RADEON_RBBM_GUICNTL, RADEON_HOST_DATA_SWAP_32BIT);
#endif
			        	while(height--)
			        	{
			        		ULONG *line = (ULONG*)sd->cpuscratch;
			        		ULONG width = bw;
			        		APTR _src = src;

			        		HIDD_BM_ConvertPixels(o, &_src, srcpf, msg->modulo, (void **)&line, dstpf, msg->modulo, msg->width, 1, NULL);

			        		line = (ULONG*)sd->cpuscratch;

			        		while(width)
			        		{
			        			if (width <= 8)
			        			{
			        				RADEONWaitForFifo(sd, width);
			        				switch (width)
			        				{
			        				case 8: OUTREGN(RADEON_HOST_DATA0, *line++);
			        				case 7: OUTREGN(RADEON_HOST_DATA1, *line++);
			        				case 6: OUTREGN(RADEON_HOST_DATA2, *line++);
			        				case 5: OUTREGN(RADEON_HOST_DATA3, *line++);
			        				case 4: OUTREGN(RADEON_HOST_DATA4, *line++);
			        				case 3: OUTREGN(RADEON_HOST_DATA5, *line++);
			        				case 2: OUTREGN(RADEON_HOST_DATA6, *line++);
			        				case 1: OUTREGN(RADEON_HOST_DATA7, *line++);
			        				}
			        				width = 0;
			        			}
			        			else
			        			{
			        				RADEONWaitForFifo(sd, 8);

			        				OUTREGN(RADEON_HOST_DATA0, *line++);
			        				OUTREGN(RADEON_HOST_DATA1, *line++);
			        				OUTREGN(RADEON_HOST_DATA2, *line++);
			        				OUTREGN(RADEON_HOST_DATA3, *line++);
			        				OUTREGN(RADEON_HOST_DATA4, *line++);
			        				OUTREGN(RADEON_HOST_DATA5, *line++);
			        				OUTREGN(RADEON_HOST_DATA6, *line++);
			        				OUTREGN(RADEON_HOST_DATA7, *line++);

			        				width -= 8;
			        			}
			        		}

			        		src += x_add;
			        	}
#if AROS_BIG_ENDIAN
					RADEONWaitForFifo(sd, 1);
					OUTREG(RADEON_RBBM_GUICNTL, RADEON_HOST_DATA_SWAP_NONE);
#endif

			        }
			        else if (bm->bpp == 2)
			        {
#if AROS_BIG_ENDIAN
			        	RADEONWaitForFifo(sd, 1);
			        	OUTREG(RADEON_RBBM_GUICNTL, RADEON_HOST_DATA_SWAP_HDW);
#endif

			        	while(height--)
			        	{
			        		ULONG *line = (ULONG*)sd->cpuscratch;
			        		ULONG width = bw;
			        		APTR _src = src;

			        		HIDD_BM_ConvertPixels(o, &_src, srcpf, msg->modulo, (void **)&line, dstpf, msg->modulo, msg->width, 1, NULL);

			        		line = (ULONG*)sd->cpuscratch;

			        		while(width)
			        		{
			        			if (width <= 16)
			        			{
			        				RADEONWaitForFifo(sd, width >> 1);
			        				switch (width)
			        				{
			        				case 16: OUTREG(RADEON_HOST_DATA0, *line++);
			        				case 14: OUTREG(RADEON_HOST_DATA1, *line++);
			        				case 12: OUTREG(RADEON_HOST_DATA2, *line++);
			        				case 10: OUTREG(RADEON_HOST_DATA3, *line++);
			        				case 8: OUTREG(RADEON_HOST_DATA4, *line++);
			        				case 6: OUTREG(RADEON_HOST_DATA5, *line++);
			        				case 4: OUTREG(RADEON_HOST_DATA6, *line++);
			        				case 2: OUTREG(RADEON_HOST_DATA7, *line++);
			        				}
			        				width = 0;
			        			}
			        			else
			        			{
			        				RADEONWaitForFifo(sd, 8);

			        				OUTREG(RADEON_HOST_DATA0, *line++);
			        				OUTREG(RADEON_HOST_DATA1, *line++);
			        				OUTREG(RADEON_HOST_DATA2, *line++);
			        				OUTREG(RADEON_HOST_DATA3, *line++);
			        				OUTREG(RADEON_HOST_DATA4, *line++);
			        				OUTREG(RADEON_HOST_DATA5, *line++);
			        				OUTREG(RADEON_HOST_DATA6, *line++);
			        				OUTREG(RADEON_HOST_DATA7, *line++);

			        				width -= 16;
			        			}
			        		}

			        		src += x_add;
			        	}
			        }
#if AROS_BIG_ENDIAN
			        RADEONWaitForFifo(sd, 1);
			        OUTREG(RADEON_RBBM_GUICNTL, RADEON_HOST_DATA_SWAP_NONE);
#endif

				}
			}

			UNLOCK_HW
    	}
    }

    if (!done)
    {
    	if (bm->fbgfx)
        {
			VideoData += (IPTR)sd->Card.FrameBuffer;

			if (sd->Card.Busy)
			{
				LOCK_HW
/* TODO: NVSync(sd) */
				RADEONWaitForIdleMMIO(sd);
				UNLOCK_HW
			}
        }

    	switch(msg->pixFmt)
		{
			case vHidd_StdPixFmt_Native:
				switch(bm->bpp)
				{
					case 1:
					{
						struct pHidd_BitMap_CopyMemBox8 __m = {
								sd->mid_CopyMemBox8,
								msg->pixels,
								0,
								0,
								(APTR)VideoData,
								msg->x,
								msg->y,
								msg->width,
								msg->height,
								msg->modulo,
								bm->pitch
						}, *m = &__m;

						OOP_DoMethod(o, (OOP_Msg)m);
					}
					break;

				case 2:
					{
						struct pHidd_BitMap_CopyMemBox16 __m = {
								sd->mid_CopyMemBox16,
								msg->pixels,
								0,
								0,
								(APTR)VideoData,
								msg->x,
								msg->y,
								msg->width,
								msg->height,
								msg->modulo,
								bm->pitch
						}, *m = &__m;

						OOP_DoMethod(o, (OOP_Msg)m);
					}
					break;

				case 4:
					{
						struct pHidd_BitMap_CopyMemBox32 __m = {
								sd->mid_CopyMemBox32,
								msg->pixels,
								0,
								0,
								(APTR)VideoData,
								msg->x,
								msg->y,
								msg->width,
								msg->height,
								msg->modulo,
								bm->pitch
						}, *m = &__m;

						OOP_DoMethod(o, (OOP_Msg)m);
					}
					break;

					} /* switch(data->bytesperpix) */
				break;

			case vHidd_StdPixFmt_Native32:
				switch(bm->bpp)
				{
					case 1:
					{
					struct pHidd_BitMap_PutMem32Image8 __m = {
								sd->mid_PutMem32Image8,
								msg->pixels,
								(APTR)VideoData,
								msg->x,
								msg->y,
								msg->width,
								msg->height,
								msg->modulo,
								bm->pitch
						}, *m = &__m;
					OOP_DoMethod(o, (OOP_Msg)m);
					}
					break;

				case 2:
					{
					struct pHidd_BitMap_PutMem32Image16 __m = {
								sd->mid_PutMem32Image16,
								msg->pixels,
								(APTR)VideoData,
								msg->x,
								msg->y,
								msg->width,
								msg->height,
								msg->modulo,
								bm->pitch
						}, *m = &__m;
					OOP_DoMethod(o, (OOP_Msg)m);
					}
					break;

				case 4:
					{
					struct pHidd_BitMap_CopyMemBox32 __m = {
							sd->mid_CopyMemBox32,
							msg->pixels,
							0,
							0,
							(APTR)VideoData,
							msg->x,
							msg->y,
							msg->width,
							msg->height,
							msg->modulo,
							bm->pitch
					}, *m = &__m;

					OOP_DoMethod(o, (OOP_Msg)m);
					}
					break;

				} /* switch(data->bytesperpix) */
				break;

			default:
				OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
				break;
		} /* switch(msg->pixFmt) */
    }

    UNLOCK_BITMAP
}

VOID METHOD(ATIOffBM, Hidd_BitMap, GetImage)
    __attribute__((alias(METHOD_NAME_S(ATIOnBM, Hidd_BitMap, GetImage))));

VOID METHOD(ATIOnBM, Hidd_BitMap, GetImage)
{
    atiBitMap *bm = OOP_INST_DATA(cl, o);

    LOCK_BITMAP

    IPTR VideoData = bm->framebuffer;

    if (bm->fbgfx)
    {
        VideoData += (IPTR)sd->Card.FrameBuffer;
        if (sd->Card.Busy)
        {
            LOCK_HW
/* TODO: NVSync(sd) */
            RADEONWaitForIdleMMIO(sd);
            UNLOCK_HW
        }
    }

    switch(msg->pixFmt)
    {
        case vHidd_StdPixFmt_Native:
            switch(bm->bpp)
            {
                case 1:
                {
                struct pHidd_BitMap_CopyMemBox8 __m = {
                            sd->mid_CopyMemBox8,
                        (APTR)VideoData,
                        msg->x,
                        msg->y,
                        msg->pixels,
                        0,
                        0,
                        msg->width,
                        msg->height,
                        bm->pitch,
                        msg->modulo
                }, *m = &__m;

                OOP_DoMethod(o, (OOP_Msg)m);
                }
                break;

            case 2:
                {
                struct pHidd_BitMap_CopyMemBox16 __m = {
                            sd->mid_CopyMemBox16,
                        (APTR)VideoData,
                        msg->x,
                        msg->y,
                        msg->pixels,
                        0,
                        0,
                        msg->width,
                        msg->height,
                        bm->pitch,
                        msg->modulo
                }, *m = &__m;

                OOP_DoMethod(o, (OOP_Msg)m);
                }
                break;

            case 4:
                {
                struct pHidd_BitMap_CopyMemBox32 __m = {
                            sd->mid_CopyMemBox32,
                        (APTR)VideoData,
                        msg->x,
                        msg->y,
                        msg->pixels,
                        0,
                        0,
                        msg->width,
                        msg->height,
                        bm->pitch,
                        msg->modulo
                }, *m = &__m;

                OOP_DoMethod(o, (OOP_Msg)m);
                }
                break;

                } /* switch(data->bytesperpix) */
            break;

        case vHidd_StdPixFmt_Native32:
            switch(bm->bpp)
            {
                case 1:
                {
                struct pHidd_BitMap_GetMem32Image8 __m = {
                    sd->mid_GetMem32Image8,
                    (APTR)VideoData,
                    msg->x,
                    msg->y,
                    msg->pixels,
                    msg->width,
                    msg->height,
                    bm->pitch,
                    msg->modulo
                }, *m = &__m;

                OOP_DoMethod(o, (OOP_Msg)m);
                }
                break;

            case 2:
                {
                struct pHidd_BitMap_GetMem32Image16 __m = {
                    sd->mid_GetMem32Image16,
                    (APTR)VideoData,
                    msg->x,
                    msg->y,
                    msg->pixels,
                    msg->width,
                    msg->height,
                    bm->pitch,
                    msg->modulo
                }, *m = &__m;

                OOP_DoMethod(o, (OOP_Msg)m);
                }
                break;

            case 4:
                {
                struct pHidd_BitMap_CopyMemBox32 __m = {
                            sd->mid_CopyMemBox32,
                        (APTR)VideoData,
                        msg->x,
                        msg->y,
                        msg->pixels,
                        0,
                        0,
                        msg->width,
                        msg->height,
                        bm->pitch,
                        msg->modulo
                }, *m = &__m;

                OOP_DoMethod(o, (OOP_Msg)m);
                }
                break;

            } /* switch(data->bytesperpix) */
            break;

        default:
            OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
            break;

    } /* switch(msg->pixFmt) */

    UNLOCK_BITMAP

}

VOID METHOD(ATIOffBM, Hidd_BitMap, PutTemplate)
    __attribute__((alias(METHOD_NAME_S(ATIOnBM, Hidd_BitMap, PutTemplate))));

VOID METHOD(ATIOnBM, Hidd_BitMap, PutTemplate)
{
    atiBitMap *bm = OOP_INST_DATA(cl, o);

    D(bug("[ATI] NO-ACCEL: BitMap::PutTemplate\n"));

    LOCK_BITMAP

    IPTR VideoData = bm->framebuffer;

    if (bm->fbgfx)
    {
        VideoData += (IPTR)sd->Card.FrameBuffer;
        if (sd->Card.Busy)
        {
            LOCK_HW
/* TODO: NVSync(sd) */
            RADEONWaitForIdleMMIO(sd);
            UNLOCK_HW
        }
    }


    switch(bm->bpp)
    {
        case 1:
            {
            struct pHidd_BitMap_PutMemTemplate8 __m = {
                    sd->mid_PutMemTemplate8,
                    msg->gc,
                    msg->template,
                    msg->modulo,
                    msg->srcx,
                    (APTR)VideoData,
                    bm->pitch,
                    msg->x,
                    msg->y,
                    msg->width,
                    msg->height,
                    msg->inverttemplate
            }, *m = &__m;

            OOP_DoMethod(o, (OOP_Msg)m);
            }
            break;

        case 2:
            {
            struct pHidd_BitMap_PutMemTemplate16 __m = {
                    sd->mid_PutMemTemplate16,
                    msg->gc,
                    msg->template,
                    msg->modulo,
                    msg->srcx,
                    (APTR)VideoData,
                    bm->pitch,
                    msg->x,
                    msg->y,
                    msg->width,
                    msg->height,
                    msg->inverttemplate
            }, *m = &__m;

            OOP_DoMethod(o, (OOP_Msg)m);
            }
            break;

        case 4:
            {
            struct pHidd_BitMap_PutMemTemplate32 __m = {
                    sd->mid_PutMemTemplate32,
                    msg->gc,
                    msg->template,
                    msg->modulo,
                    msg->srcx,
                    (APTR)VideoData,
                    bm->pitch,
                    msg->x,
                    msg->y,
                    msg->width,
                    msg->height,
                    msg->inverttemplate
            }, *m = &__m;

            OOP_DoMethod(o, (OOP_Msg)m);
            }
            break;
    } /* switch(bm->bpp) */

    UNLOCK_BITMAP
}

VOID METHOD(ATIOffBM, Hidd_BitMap, PutPattern)
    __attribute__((alias(METHOD_NAME_S(ATIOnBM, Hidd_BitMap, PutPattern))));

VOID METHOD(ATIOnBM, Hidd_BitMap, PutPattern)
{
    atiBitMap *bm = OOP_INST_DATA(cl, o);

    D(bug("[ATI] NO-ACCEL: BitMap::PutPattern\n"));

    LOCK_BITMAP

    IPTR VideoData = bm->framebuffer;

    if (bm->fbgfx)
    {
        VideoData += (IPTR)sd->Card.FrameBuffer;
        if (sd->Card.Busy)
        {
            LOCK_HW
/* TODO: NVSync(sd) */
            RADEONWaitForIdleMMIO(sd);
            UNLOCK_HW
        }
    }


    switch(bm->bpp)
    {
        case 1:
            {
            struct pHidd_BitMap_PutMemPattern8 __m = {
                    sd->mid_PutMemPattern8,
                    msg->gc,
                    msg->pattern,
                    msg->patternsrcx,
                    msg->patternsrcy,
                    msg->patternheight,
                    msg->patterndepth,
                    msg->patternlut,
                    msg->invertpattern,
                    msg->mask,
                    msg->maskmodulo,
                    msg->masksrcx,
                    (APTR)VideoData,
                    bm->pitch,
                    msg->x,
                    msg->y,
                    msg->width,
                    msg->height
            }, *m = &__m;

            OOP_DoMethod(o, (OOP_Msg)m);
            }
            break;

        case 2:
            {
            struct pHidd_BitMap_PutMemPattern16 __m = {
                    sd->mid_PutMemPattern16,
                    msg->gc,
                    msg->pattern,
                    msg->patternsrcx,
                    msg->patternsrcy,
                    msg->patternheight,
                    msg->patterndepth,
                    msg->patternlut,
                    msg->invertpattern,
                    msg->mask,
                    msg->maskmodulo,
                    msg->masksrcx,
                    (APTR)VideoData,
                    bm->pitch,
                    msg->x,
                    msg->y,
                    msg->width,
                    msg->height
            }, *m = &__m;

            OOP_DoMethod(o, (OOP_Msg)m);
            }
            break;

        case 4:
            {
            struct pHidd_BitMap_PutMemPattern32 __m = {
                    sd->mid_PutMemPattern32,
                    msg->gc,
                    msg->pattern,
                    msg->patternsrcx,
                    msg->patternsrcy,
                    msg->patternheight,
                    msg->patterndepth,
                    msg->patternlut,
                    msg->invertpattern,
                    msg->mask,
                    msg->maskmodulo,
                    msg->masksrcx,
                    (APTR)VideoData,
                    bm->pitch,
                    msg->x,
                    msg->y,
                    msg->width,
                    msg->height
            }, *m = &__m;

            OOP_DoMethod(o, (OOP_Msg)m);
            }
            break;
    } /* switch(bm->bpp) */

    UNLOCK_BITMAP
}
