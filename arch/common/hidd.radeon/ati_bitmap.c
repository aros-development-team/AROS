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

struct pRoot_Dispose {
    OOP_MethodID mID;
};

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
        
        ULONG width, height, depth;
        UBYTE bytesPerPixel;
        ULONG fb;
        
        OOP_Object *pf;
        
        InitSemaphore(&bm->bmLock);
        
        D(bug("[ATIBitMap] Super called. o=%p\n", o));
        
        bm->onbm = (cl == sd->OnBMClass);
        
        OOP_GetAttr(o, aHidd_BitMap_Width,  &width);
        OOP_GetAttr(o, aHidd_BitMap_Height, &height);
        OOP_GetAttr(o, aHidd_BitMap_PixFmt, (APTR)&pf);
        OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);
        fb = GetTagData(aHidd_BitMap_FrameBuffer, FALSE, msg->attrList);
        
        D(bug("[ATIBitmap] width=%d height=%d depth=%d\n", width, height, depth));
        
        if (width == 0 || height == 0 || depth == 0)
        {
            bug("[ATIBitMap] size mismatch!\n");
        }
        
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
        
        if (bm->framebuffer != -1)
        {
            ULONG pitch64 = ((bm->pitch)) >> 6;
            
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
                    ULONG pixel;
                    ULONG hdisp, vdisp, hstart, hend, htotal, vstart, vend, vtotal;
                
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
                bm->framebuffer = (IPTR)AllocMem(bm->pitch * bm->height,
                            MEMF_PUBLIC | MEMF_CLEAR);
                bm->fbgfx = FALSE;
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
                                     | RADEON_ROP[GC_DRMD(vHidd_GC_DrawMode_Copy)].pattern);

        RADEONWaitForFifo(sd, 4);
    
        OUTREG(RADEON_DP_GUI_MASTER_CNTL, bm->dp_gui_master_cntl_clip);
        OUTREG(RADEON_DP_BRUSH_FRGD_CLR,  GC_BG(msg->gc));
        OUTREG(RADEON_DP_WRITE_MASK,      ~0);
        OUTREG(RADEON_DP_CNTL,            (RADEON_DST_X_LEFT_TO_RIGHT
                                          | RADEON_DST_Y_TOP_TO_BOTTOM));

        RADEONWaitForFifo(sd, 2);

        OUTREG(RADEON_DST_Y_X,          0);
        OUTREG(RADEON_DST_WIDTH_HEIGHT, (bm->width << 16) | bm->height);

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

        OUTREG(RADEON_DST_Y_X,          (msg->dr.minY << 16) | msg->dr.minX);
        OUTREG(RADEON_DST_WIDTH_HEIGHT, ((msg->dr.maxX - msg->dr.minX + 1) << 16) | (msg->dr.maxY - msg->dr.minY + 1));

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
            
            RADEONWaitForFifo(sd, 2);
            OUTREG(RADEON_SC_TOP_LEFT,        (GC_CLIPY1(gc) << 16) | GC_CLIPX1(gc));
            OUTREG(RADEON_SC_BOTTOM_RIGHT,    ((GC_CLIPY2(gc)+1) << 16) | (GC_CLIPX2(gc)+1));
        }

        RADEONWaitForFifo(sd, 3);

        OUTREG(RADEON_DP_GUI_MASTER_CNTL, bm->dp_gui_master_cntl_clip);
        OUTREG(RADEON_DP_BRUSH_FRGD_CLR,  GC_FG(gc));
        OUTREG(RADEON_DP_WRITE_MASK,      ~0);

        RADEONWaitForFifo(sd, 4);

        OUTREG(RADEON_DST_LINE_START, (msg->y1 << 16) | msg->x1);
        OUTREG(RADEON_DST_LINE_END,   (msg->y2 << 16) | msg->x2);
        OUTREG(RADEON_DST_LINE_START, (msg->y2 << 16) | msg->x2);
        OUTREG(RADEON_DST_LINE_END,   ((msg->y2+1) << 16) | (msg->x2+1));
        
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
            
            RADEONWaitForFifo(sd, 2);
            OUTREG(RADEON_SC_TOP_LEFT,        (GC_CLIPY1(gc) << 16) | GC_CLIPX1(gc));
            OUTREG(RADEON_SC_BOTTOM_RIGHT,    ((GC_CLIPY2(gc)+1) << 16) | (GC_CLIPX2(gc)+1));
        }

        RADEONWaitForFifo(sd, 3);

        OUTREG(RADEON_DP_GUI_MASTER_CNTL, bm->dp_gui_master_cntl_clip);
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
            
            RADEONWaitForFifo(sd, 2);
            OUTREG(RADEON_SC_TOP_LEFT,        (GC_CLIPY1(gc) << 16) | GC_CLIPX1(gc));
            OUTREG(RADEON_SC_BOTTOM_RIGHT,    ((GC_CLIPY2(gc)+1) << 16) | (GC_CLIPX2(gc)+1));
        }

        RADEONWaitForFifo(sd, 3);

        OUTREG(RADEON_DP_GUI_MASTER_CNTL, bm->dp_gui_master_cntl_clip);
        OUTREG(RADEON_DP_BRUSH_FRGD_CLR,  GC_FG(gc));
        OUTREG(RADEON_DP_WRITE_MASK,      ~0);

        for (i = 2; i < (2 * msg->n); i+=2)
        {
            RADEONWaitForFifo(sd, 2);
            OUTREG(RADEON_DST_LINE_START, (msg->coords[i-1] << 16) | msg->coords[i-2]);
            OUTREG(RADEON_DST_LINE_END,   (msg->coords[i+1] << 16) | msg->coords[i]);
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
    
    LOCK_BITMAP

    UBYTE *ptr = (UBYTE*)((IPTR)bm->framebuffer + bm->bpp * msg->x + bm->pitch * msg->y);

    if (bm->fbgfx)
    {
        ptr += (IPTR)sd->Card.FrameBuffer;
        if (sd->Card.Busy)
        {
            LOCK_HW
#warning TODO: NVSync(sd)
            RADEONWaitForIdleMMIO(sd);
            UNLOCK_HW
        }
    }

    switch (bm->bpp)
    {
        case 1:
            *ptr = msg->pixel;
            break;
        case 2:
            *(UWORD*)ptr = msg->pixel;
            break;
        case 4:
            *(ULONG*)ptr = msg->pixel;
            break;
    }

    UNLOCK_BITMAP
}


HIDDT_Pixel METHOD(ATIOffBM, Hidd_BitMap, GetPixel)
    __attribute__((alias(METHOD_NAME_S(ATIOnBM, Hidd_BitMap, GetPixel))));

HIDDT_Pixel METHOD(ATIOnBM, Hidd_BitMap, GetPixel)
{
    HIDDT_Pixel pixel=0;
    atiBitMap *bm = OOP_INST_DATA(cl, o);
    
    LOCK_BITMAP
    
    UBYTE *ptr = (UBYTE*)((IPTR)bm->framebuffer + bm->bpp * msg->x + bm->pitch * msg->y);

    if (bm->fbgfx)
    {
        ptr += (IPTR)sd->Card.FrameBuffer;
        if (sd->Card.Busy)
        {
            LOCK_HW
#warning TODO: NVSync(sd)
            RADEONWaitForIdleMMIO(sd);
            UNLOCK_HW
        }
    }

    switch (bm->bpp)
    {
        case 1:
            pixel = *ptr;
            break;
        case 2:
            pixel = *(UWORD*)ptr;
            break;
        case 4:
            pixel = *(ULONG*)ptr;
            break;
    }

    UNLOCK_BITMAP

    /* Get pen number from colortab */
    return pixel;
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
#warning TODO: NVSync(sd)
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

    IPTR VideoData = bm->framebuffer;

    if (bm->fbgfx)
    {
       VideoData += (IPTR)sd->Card.FrameBuffer;

        if (sd->Card.Busy)
        {
            LOCK_HW
#warning TODO: NVSync(sd)
            RADEONWaitForIdleMMIO(sd);
            UNLOCK_HW
        }
    }

    switch(bm->bpp)
    {
        case 2:
            {
            struct pHidd_BitMap_CopyLUTMemBox16 __m = {
                    sd->mid_CopyLUTMemBox16,
                    msg->pixels,
                    0,
                    0,
                    (APTR)VideoData,
                    msg->x,
                    msg->y,
                    msg->width,
                    msg->height,
                    msg->modulo,
                    bm->pitch,
                    msg->pixlut         
            }, *m = &__m;
        
            OOP_DoMethod(o, (OOP_Msg)m);
            }
            break;
        
        case 4: 
            {
            struct pHidd_BitMap_CopyLUTMemBox32 __m = {
                    sd->mid_CopyLUTMemBox32,
                    msg->pixels,
                    0,
                    0,
                    (APTR)VideoData,
                    msg->x,
                    msg->y,
                    msg->width,
                    msg->height,
                    msg->modulo,
                    bm->pitch,
                    msg->pixlut         
            }, *m = &__m;
        
            OOP_DoMethod(o, (OOP_Msg)m);
            }
            break;

        default:
            OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
            break;
        
    } /* switch(data->bytesperpix) */
 
    UNLOCK_BITMAP
}


VOID METHOD(ATIOffBM, Hidd_BitMap, PutImage)
    __attribute__((alias(METHOD_NAME_S(ATIOnBM, Hidd_BitMap, PutImage))));

VOID METHOD(ATIOnBM, Hidd_BitMap, PutImage)
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
#warning TODO: NVSync(sd)
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
#warning TODO: NVSync(sd)
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

    LOCK_BITMAP
    
    IPTR VideoData = bm->framebuffer;

    if (bm->fbgfx)
    {
        VideoData += (IPTR)sd->Card.FrameBuffer;
        if (sd->Card.Busy)
        {
            LOCK_HW
#warning TODO: NVSync(sd)
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

    LOCK_BITMAP
    
    IPTR VideoData = bm->framebuffer;

    if (bm->fbgfx)
    {
        VideoData += (IPTR)sd->Card.FrameBuffer;
        if (sd->Card.Busy)
        {
            LOCK_HW
#warning TODO: NVSync(sd)
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
