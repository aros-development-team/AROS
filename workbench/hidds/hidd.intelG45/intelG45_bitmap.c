/*
 * intelG45_bitmap.c
 *
 *  Created on: May 1, 2010
 *      Author: misc
 */

#define DEBUG 1
#include <aros/debug.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <utility/tagitem.h>

#include <hidd/graphics.h>
#include <hidd/i2c.h>

#include <proto/oop.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include <stdint.h>



#include LC_LIBDEFS_FILE

#include "intelG45_intern.h"
#include "intelG45_regs.h"

#define sd ((struct g45staticdata*)SD(cl))

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

struct pRoot_Dispose {
    OOP_MethodID mID;
};

OOP_Object *METHOD(GMABM, Root, New)
{
	EnterFunc(bug("[GMABitMap] Bitmap::New()\n"));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        GMABitMap_t *bm = OOP_INST_DATA(cl, o);

        ULONG width, height, depth;
        UBYTE bytesPerPixel;
        IPTR displayable;

        OOP_Object *pf;

        InitSemaphore(&bm->bmLock);

        D(bug("[GMABitMap] Super called. o=%p\n", o));

        OOP_GetAttr(o, aHidd_BitMap_Width,  &width);
        OOP_GetAttr(o, aHidd_BitMap_Height, &height);
        OOP_GetAttr(o, aHidd_BitMap_PixFmt, (APTR)&pf);
        OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);
        OOP_GetAttr(o, aHidd_BitMap_Displayable, &displayable);

        bm->onbm = displayable;

        D(bug("[GMABitmap] width=%d height=%d depth=%d\n", width, height, depth));

        if (width == 0 || height == 0 || depth == 0)
        {
            bug("[GMABitMap] size mismatch!\n");
        }

        if (depth == 24)
        	depth = 32;

        if (depth <= 8)
            bytesPerPixel = 1;
        else if (depth <= 16)
            bytesPerPixel = 2;
        else
            bytesPerPixel = 4;

        bm->width = width;
        bm->height = height;
        bm->pitch = (width * bytesPerPixel + 63) & ~63;
        bm->depth = depth;
        bm->bpp = bytesPerPixel;
        bm->framebuffer = 0; //AllocBitmapArea(sd, bm->width, bm->height, bm->bpp, TRUE);
        bm->fbgfx = TRUE;
        bm->state = NULL;
        bm->bitmap = o;
        bm->usecount = 0;

        if (bm->framebuffer != -1)
        {
            ULONG pitch64 = ((bm->pitch)) >> 6;

//            switch(depth)
//            {
//                case 15:
//                    bm->datatype = 3;
//                    break;
//
//                case 16:
//                    bm->datatype = 4;
//                    break;
//
//                case 32:
//                    bm->datatype = 6;
//                    break;
//            }

//            bm->dp_gui_master_cntl =
//                        ((bm->datatype << RADEON_GMC_DST_DATATYPE_SHIFT)
//                        |RADEON_GMC_CLR_CMP_CNTL_DIS
//                        |RADEON_GMC_DST_PITCH_OFFSET_CNTL);
//
//            bm->pitch_offset = ((bm->framebuffer >> 10) | (bm->pitch << 16));
//
//            D(bug("[GMABitMap] PITCH_OFFSET=%08x\n", bm->pitch_offset));
        }

        if (displayable)
        {
        	bm->state = AllocVecPooled(sd->MemPool, sizeof(GMAState_t));

			if (bm->state && (bm->framebuffer != -1))
            {
                HIDDT_ModeID modeid;
                OOP_Object *sync;

                /* We should be able to get modeID from the bitmap */
                OOP_GetAttr(o, aHidd_BitMap_ModeID, &modeid);

                D(bug("[GMABitMap] BM_ModeID=%x\n", modeid));

                if (modeid != vHidd_ModeID_Invalid)
                {
                    ULONG pixel;
                    ULONG hdisp, vdisp, hstart, hend, htotal, vstart, vend, vtotal, flags;

                    /* Get Sync and PixelFormat properties */
                    struct pHidd_Gfx_GetMode __getmodemsg = {
                        modeID: modeid,
                        syncPtr:    &sync,
                        pixFmtPtr:  &pf,
                    }, *getmodemsg = &__getmodemsg;

                    getmodemsg->mID = OOP_GetMethodID((STRPTR)CLID_Hidd_Gfx, moHidd_Gfx_GetMode);
                    OOP_DoMethod(sd->GMAObject, (OOP_Msg)getmodemsg);

                    OOP_GetAttr(sync, aHidd_Sync_PixelClock,    &pixel);
                    OOP_GetAttr(sync, aHidd_Sync_HDisp,         &hdisp);
                    OOP_GetAttr(sync, aHidd_Sync_VDisp,         &vdisp);
                    OOP_GetAttr(sync, aHidd_Sync_HSyncStart,    &hstart);
                    OOP_GetAttr(sync, aHidd_Sync_VSyncStart,    &vstart);
                    OOP_GetAttr(sync, aHidd_Sync_HSyncEnd,      &hend);
                    OOP_GetAttr(sync, aHidd_Sync_VSyncEnd,      &vend);
                    OOP_GetAttr(sync, aHidd_Sync_HTotal,        &htotal);
                    OOP_GetAttr(sync, aHidd_Sync_VTotal,        &vtotal);
                    OOP_GetAttr(sync, aHidd_Sync_Flags,			&flags);

                    bm->state = (GMAState_t *)AllocPooled(sd->MemPool,
                            sizeof(GMAState_t));

                    pixel /= 1000;

                    if (bm->state)
                    {
                        LOCK_HW

                        G45_InitMode(sd, bm->state, width, height, depth, pixel, bm->framebuffer,
                                    hdisp, vdisp,
                                    hstart, hend, htotal,
                                    vstart, vend, vtotal, flags);
/*
                        LoadState(sd, bm->state);
                        DPMS(sd, sd->dpms);

                        RADEONEngineReset(sd);
                        RADEONEngineRestore(sd);
*/
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


VOID METHOD(GMABM, Root, Dispose)
{
    GMABitMap_t *bm = OOP_INST_DATA(cl, o);

    LOCK_BITMAP
    LOCK_HW

#if 0
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
#endif

    UNLOCK_HW
    UNLOCK_BITMAP

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}


VOID METHOD(GMABM, Hidd_BitMap, PutPixel)
{
}

VOID METHOD(GMABM, Hidd_BitMap, GetPixel)
{
}

VOID METHOD(GMABM, Hidd_BitMap, DrawPixel)
{
}

