/*
    Copyright © 2004-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: NVidia bitmap class
    Lang: English
*/

#include <exec/types.h>
#include <exec/memory.h>

#include <hidd/hidd.h>
#include <hidd/graphics.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include <aros/symbolsets.h>

#include "nv.h"
#include "nv_dma.h"

#define DEBUG 0
#include <aros/debug.h>

#define _sd (&((LIBBASETYPEPTR)cl->UserData)->sd)

#undef HiddPCIDeviceAttrBase
#undef HiddGfxAttrBase
#undef HiddPixFmtAttrBase
#undef HiddSyncAttrBase
#undef HiddBitMapAttrBase
#define HiddPCIDeviceAttrBase	(_sd->pciAttrBase)
#define HiddNVidiaBitMapAttrBase (_sd->nvBitMapAttrBase)
#define HiddBitMapAttrBase	(_sd->bitMapAttrBase)
#define HiddPixFmtAttrBase	(_sd->pixFmtAttrBase)
#define HiddGfxAttrBase		(_sd->gfxAttrBase)
#define HiddSyncAttrBase	(_sd->syncAttrBase)

/* Class methods */

OOP_Object *NVOnBM__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    EnterFunc(bug("[NVBitMap] OnBitmap::New()\n"));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	D(bug("o=%p, cl=%p\n", o, cl));

    if (o)
    {
	nvBitMap *bm = OOP_INST_DATA(cl, o);

	ULONG width, height, depth;
	UBYTE bytesPerPixel;
	ULONG fb;

	OOP_Object *pf;

	InitSemaphore(&bm->bmLock);

	D(bug("[NVBitMap] Super called. o=%p\n", o));

	bm->onbm = TRUE;

	OOP_GetAttr(o, aHidd_BitMap_Width,  &width);
	OOP_GetAttr(o, aHidd_BitMap_Height, &height);
	OOP_GetAttr(o, aHidd_BitMap_PixFmt, (APTR)&pf);
	OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);

	fb = GetTagData(aHidd_BitMap_FrameBuffer, FALSE, msg->attrList);

	D(bug("[NVBitmap] width=%d height=%d depth=%d\n", width, height, depth));

	if (width == 0 || height == 0 || depth == 0)
	{
	    bug("[NVBitMap] size mismatch!\n");
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
	    bytesPerPixel = 2;
	    depth = 16;
	}

	bm->width = width;
	bm->height = height;
	bm->pitch = (width * bytesPerPixel + 63) & ~63;
	bm->depth = depth;
	bm->bpp = bytesPerPixel;

	bm->framebuffer = AllocBitmapArea(_sd, bm->width, bm->height,
				bm->bpp, TRUE);

	bm->fbgfx = TRUE;
	bm->BitMap = o;
	bm->usecount = 0;

	switch(bm->depth)
	{
	    case 32:
	    case 24:
		bm->surface_format = SURFACE_FORMAT_DEPTH24;
		bm->pattern_format = PATTERN_FORMAT_DEPTH24;
		bm->rect_format    = RECT_FORMAT_DEPTH24;
		bm->line_format    = LINE_FORMAT_DEPTH24;
		break;
	    case 16:
	    case 15:
		bm->surface_format = SURFACE_FORMAT_DEPTH16;
		bm->pattern_format = PATTERN_FORMAT_DEPTH16;
		bm->rect_format    = RECT_FORMAT_DEPTH16;
		bm->line_format    = LINE_FORMAT_DEPTH16;
		break;
	    default:
		bm->surface_format = SURFACE_FORMAT_DEPTH8;
		bm->pattern_format = PATTERN_FORMAT_DEPTH8;
		bm->rect_format    = RECT_FORMAT_DEPTH8;
		bm->line_format    = LINE_FORMAT_DEPTH8;
		break;
	}

	if (fb && bm->framebuffer != 0xffffffff)
	{
	    bm->state = (struct CardState *)AllocPooled(_sd->memPool, 
					sizeof(struct CardState));
	    
	    bzero(_sd->Card.FrameBuffer + bm->framebuffer, 640*480*2);
	    
	    if (bm->state)
	    {
		LOCK_HW

		InitMode(_sd, bm->state, 640, 480, 16, 25200, bm->framebuffer, 
		    640, 480,
		    656, 752, 800,
		    490, 492, 525);

		LoadState(_sd, bm->state);
		DPMS(_sd, _sd->dpms);

		UNLOCK_HW

		return o;
	    }
	}
	else if (bm->framebuffer != 0xffffffff)
	{
	    HIDDT_ModeID modeid;
	    OOP_Object *sync;
				
	    /* We should be able to get modeID from the bitmap */
	    OOP_GetAttr(o, aHidd_BitMap_ModeID, &modeid);

	    D(bug("[NVBitMap] BM_ModeID=%x\n", modeid));
				
	    if (modeid != vHidd_ModeID_Invalid)
	    {
		ULONG pixel;
		ULONG hdisp, vdisp, hstart, hend, htotal, vstart, vend, vtotal;
	
		/* Get Sync and PixelFormat properties */
		struct pHidd_Gfx_GetMode __getmodemsg = {
		    modeID:	modeid,
		    syncPtr:	&sync,
		    pixFmtPtr:	&pf,
		}, *getmodemsg = &__getmodemsg;

		getmodemsg->mID = OOP_GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_GetMode);
		OOP_DoMethod(_sd->nvobject, (OOP_Msg)getmodemsg);

		OOP_GetAttr(sync, aHidd_Sync_PixelClock, 	&pixel);
		OOP_GetAttr(sync, aHidd_Sync_HDisp, 		&hdisp);
		OOP_GetAttr(sync, aHidd_Sync_VDisp, 		&vdisp);
		OOP_GetAttr(sync, aHidd_Sync_HSyncStart, 	&hstart);
		OOP_GetAttr(sync, aHidd_Sync_VSyncStart, 	&vstart);
		OOP_GetAttr(sync, aHidd_Sync_HSyncEnd,		&hend);
		OOP_GetAttr(sync, aHidd_Sync_VSyncEnd,		&vend);
		OOP_GetAttr(sync, aHidd_Sync_HTotal,		&htotal);
		OOP_GetAttr(sync, aHidd_Sync_VTotal,		&vtotal);

		bm->state = (struct CardState *)AllocPooled(_sd->memPool, 
					sizeof(struct CardState));

		pixel /= 1000;

		if (bm->state)
		{
		    LOCK_HW
		    
		    InitMode(_sd, bm->state, width, height, depth, pixel, bm->framebuffer,
			hdisp, vdisp,
			hstart, hend, htotal,
			vstart, vend, vtotal);

		    LoadState(_sd, bm->state);
		    DPMS(_sd, _sd->dpms);

		    UNLOCK_HW

		    return o;
		}
	    }
	}
	OOP_MethodID disp_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
	OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);
    }
   
    return NULL;
}

OOP_Object *NVOffBM__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    EnterFunc(bug("[NVBitMap] OffBitmap::New()\n"));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    if (o)
    {
	nvBitMap *bm = OOP_INST_DATA(cl, o);

	ULONG width, height, depth;
	UBYTE bytesPerPixel;

	OOP_Object *pf;

	InitSemaphore(&bm->bmLock);

	bm->onbm = FALSE;

	OOP_GetAttr(o, aHidd_BitMap_Width,  &width);
	OOP_GetAttr(o, aHidd_BitMap_Height, &height);
	OOP_GetAttr(o, aHidd_BitMap_PixFmt, (APTR)&pf);
	OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);
	D(bug("[NVBitmap] width=%d height=%d depth=%d\n", width, height, depth));

	if (width == 0 || height == 0 || depth == 0)
	{
	    bug("[NVBitMap] size mismatch!\n");
	}

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
	bm->state = NULL;
	bm->bpp = bytesPerPixel;

        bm->framebuffer = AllocBitmapArea(_sd, bm->width, bm->height,
			    bm->bpp, TRUE);

        if (bm->framebuffer == -1)
        {
	    bm->framebuffer = (IPTR)AllocMem(bm->pitch * bm->height,
					MEMF_PUBLIC | MEMF_CLEAR);
	    bm->fbgfx = FALSE;
	}
	else
	    bm->fbgfx = TRUE;
	
	bm->BitMap = o;
	bm->usecount = 0;

	switch(bm->depth)
	{
	    case 32:
	    case 24:
		bm->surface_format = SURFACE_FORMAT_DEPTH24;
		bm->pattern_format = PATTERN_FORMAT_DEPTH24;
		bm->rect_format    = RECT_FORMAT_DEPTH24;
		bm->line_format    = LINE_FORMAT_DEPTH24;
		break;
	    case 16:
	    case 15:
		bm->surface_format = SURFACE_FORMAT_DEPTH16;
		bm->pattern_format = PATTERN_FORMAT_DEPTH16;
		bm->rect_format    = RECT_FORMAT_DEPTH16;
		bm->line_format    = LINE_FORMAT_DEPTH16;
		break;
	    default:
		bm->surface_format = SURFACE_FORMAT_DEPTH8;
		bm->pattern_format = PATTERN_FORMAT_DEPTH8;
		bm->rect_format    = RECT_FORMAT_DEPTH8;
		bm->line_format    = LINE_FORMAT_DEPTH8;
		break;
	}

	if ((bm->framebuffer != 0xffffffff) && (bm->framebuffer != 0))
	{
	    return o;
	}
    }

    OOP_MethodID disp_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
    OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);
    
    return NULL;
}


VOID NVBM__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    nvBitMap *bm = OOP_INST_DATA(cl, o);

    LOCK_BITMAP
    LOCK_HW
    NVDmaKickoff(&_sd->Card);
    NVSync(_sd);
    
    if (bm->fbgfx)
    {
	FreeBitmapArea(_sd, bm->framebuffer, bm->width, bm->height, bm->bpp);

	bm->framebuffer = -1;
	bm->fbgfx = 0;
    }
    else
	FreeMem((APTR)bm->framebuffer, bm->pitch * bm->height);

    if (bm->state)
	FreePooled(_sd->memPool, bm->state, sizeof(struct CardState));

    bm->state = NULL;

    UNLOCK_HW
    UNLOCK_BITMAP

    OOP_DoSuperMethod(cl, o, msg);
}

void NVBM__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    nvBitMap *bm = OOP_INST_DATA(cl, o);
    ULONG idx;

    if (IS_NVBM_ATTR(msg->attrID, idx))
    {
	switch (idx)
	{
	    case aoHidd_nvBitMap_Drawable:
		if (bm->fbgfx)
		    *msg->storage = bm->framebuffer + (IPTR)_sd->Card.FrameBuffer;
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

VOID NVBM__Hidd_BitMap__Clear(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_Clear *msg)
{
    nvBitMap *bm = OOP_INST_DATA(cl, o);
    
    LOCK_BITMAP

    D(bug("[NVBitMap] Clear()\n"));

    if (bm->fbgfx)
    {
	LOCK_HW

	bm->usecount++;

	_sd->Card.DMAKickoffCallback = NVDMAKickoffCallback;

	NVDmaStart(&_sd->Card, SURFACE_FORMAT, 4);
	NVDmaNext(&_sd->Card, bm->surface_format);
	NVDmaNext(&_sd->Card, (bm->pitch << 16) | _sd->src_pitch);
	NVDmaNext(&_sd->Card, _sd->src_offset);
	NVDmaNext(&_sd->Card, bm->framebuffer);
	
	_sd->surface_format = bm->surface_format;
	_sd->dst_pitch 	= bm->pitch;
	_sd->dst_offset 	= bm->framebuffer;

	NVDmaStart(&_sd->Card, RECT_FORMAT, 1);
	NVDmaNext(&_sd->Card, bm->rect_format);
	_sd->rect_format = bm->rect_format;
			
	NVSetRopSolid(_sd, vHidd_GC_DrawMode_Copy, ~0 << bm->depth);
        NVDmaStart(&_sd->Card, RECT_SOLID_COLOR, 1);
        NVDmaNext(&_sd->Card, GC_BG(msg->gc));

        NVDmaStart(&_sd->Card, RECT_SOLID_RECTS(0), 2);
        NVDmaNext(&_sd->Card, 0);
        NVDmaNext(&_sd->Card, (bm->width << 16) | (bm->height));

        NVDmaKickoff(&_sd->Card);
	//NVSync(_sd);

	UNLOCK_HW   
    }
    else
    {
	ULONG *ptr = (ULONG*)bm->framebuffer;
	ULONG val=0;
	int i = bm->width * bm->height * bm->bpp / 4;

	if (_sd->gpu_busy)
	{
	    LOCK_HW
	    NVSync(_sd);
	    UNLOCK_HW
	}

	switch (bm->bpp)
	{
	    case 1:
		val = GC_BG(msg->gc) & 0xff;
		val |= val << 8;
		val |= val << 16;
		break;
	    case 2:
		val = GC_BG(msg->gc) << 16 | (GC_BG(msg->gc) & 0xffff);
		break;
	    default:
		val = GC_BG(msg->gc);
		break;
	}
	
	do
	{
	    *ptr++ = val;
	} while(--i);
	
    }

    UNLOCK_BITMAP
}


VOID NVBM__Hidd_BitMap__PutPixel(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutPixel *msg)
{
    nvBitMap *bm = OOP_INST_DATA(cl, o);
    
    LOCK_BITMAP

    UBYTE *ptr = (UBYTE*)((IPTR)bm->framebuffer + bm->bpp * msg->x + bm->pitch * msg->y);

    if (bm->fbgfx)
    {
	ptr += (IPTR)_sd->Card.FrameBuffer;
        if (_sd->gpu_busy)
        {
            LOCK_HW
            NVSync(_sd);
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

HIDDT_Pixel NVBM__Hidd_BitMap__GetPixel(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetPixel *msg)
{
    HIDDT_Pixel pixel=0;
    nvBitMap *bm = OOP_INST_DATA(cl, o);
    
    LOCK_BITMAP
    
    UBYTE *ptr = (UBYTE*)((IPTR)bm->framebuffer + bm->bpp * msg->x + bm->pitch * msg->y);

    if (bm->fbgfx)
    {
	ptr += (IPTR)_sd->Card.FrameBuffer;
        if (_sd->gpu_busy)
        {
            LOCK_HW
            NVSync(_sd);
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

VOID NVBM__Hidd_BitMap__FillRect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    nvBitMap *bm = OOP_INST_DATA(cl, o);

    LOCK_BITMAP

    D(bug("[NVBitMap] FillRect(%p,%d,%d,%d,%d)\n",
	bm->framebuffer, msg->minX, msg->minY, msg->maxX, msg->maxY));

    if (bm->fbgfx)
    {
	LOCK_HW

	bm->usecount++;

	_sd->Card.DMAKickoffCallback = NVDMAKickoffCallback;
	_sd->gpu_busy = TRUE;

	NVDmaStart(&_sd->Card, SURFACE_FORMAT, 4);
	NVDmaNext(&_sd->Card, bm->surface_format);
	NVDmaNext(&_sd->Card, (bm->pitch << 16) | _sd->src_pitch);
	NVDmaNext(&_sd->Card, _sd->src_offset);
	NVDmaNext(&_sd->Card, bm->framebuffer);
	
	_sd->surface_format = bm->surface_format;
	_sd->dst_pitch 	= bm->pitch;
	_sd->dst_offset 	= bm->framebuffer;

	NVDmaStart(&_sd->Card, RECT_FORMAT, 1);
	NVDmaNext(&_sd->Card, bm->rect_format);
	_sd->rect_format = bm->rect_format;

        NVSetRopSolid(_sd, GC_DRMD(msg->gc), ~0 << bm->depth);
        NVDmaStart(&_sd->Card, RECT_SOLID_COLOR, 1);
        NVDmaNext(&_sd->Card, GC_FG(msg->gc));

        NVDmaStart(&_sd->Card, RECT_SOLID_RECTS(0), 2);
        NVDmaNext(&_sd->Card, (msg->minX << 16) | (msg->minY & 0xffff));
        NVDmaNext(&_sd->Card, ((msg->maxX - msg->minX + 1) << 16)
	    | ((msg->maxY - msg->minY + 1) & 0xffff));

	NVDmaKickoff(&_sd->Card);
	
//	if ((msg->maxX - msg->minX) * (msg->maxY - msg->minY) > 512)
//	    NVSync(_sd);
	
	UNLOCK_HW
    }
    else
    {
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }

    UNLOCK_BITMAP
}

ULONG NVBM__Hidd_BitMap__BytesPerLine(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_BytesPerLine *msg)
{
    nvBitMap *bm = OOP_INST_DATA(cl, o);

    return (bm->bpp * msg->width + 63) & ~63;
}

VOID NVBM__Hidd_BitMap__DrawLine(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawLine *msg)
{
    OOP_Object *gc = msg->gc;
    nvBitMap *bm = OOP_INST_DATA(cl, o);
  
    LOCK_BITMAP
  
    D(bug("[NVBitmap] DrawLine(%p, %d, %d, %d, %d)\n",
	bm->framebuffer, msg->x1, msg->y1, msg->x2, msg->y2));

    if ((GC_LINEPAT(gc) != (UWORD)~0) || !bm->fbgfx)
    {
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
    else
    {
	LOCK_HW
    
	bm->usecount++;


	_sd->Card.DMAKickoffCallback = NVDMAKickoffCallback;
	_sd->gpu_busy = TRUE;
	
	NVDmaStart(&_sd->Card, SURFACE_OFFSET_DST, 1);
	NVDmaNext(&_sd->Card, bm->framebuffer);
	_sd->dst_offset = bm->framebuffer;
	
	NVDmaStart(&_sd->Card, SURFACE_PITCH, 1);
	NVDmaNext(&_sd->Card, (bm->pitch << 16) | _sd->src_pitch);
	_sd->dst_pitch = bm->pitch;

	if (GC_DOCLIP(gc))
	{
	    NVDmaStart(&_sd->Card, CLIP_POINT, 2);
	    NVDmaNext(&_sd->Card, (GC_CLIPY1(gc) << 16) | GC_CLIPX1(gc));
	    NVDmaNext(&_sd->Card, ((GC_CLIPY2(gc)-GC_CLIPY1(gc)+1) << 16) |
				  (GC_CLIPX2(gc)-GC_CLIPX1(gc)+1));
	}
	
	NVSetRopSolid(_sd, GC_DRMD(gc), ~0 << bm->depth);
	NVDmaStart(&_sd->Card, LINE_FORMAT, 2);
	NVDmaNext(&_sd->Card, bm->line_format);
	NVDmaNext(&_sd->Card, GC_FG(gc));

	NVDmaStart(&_sd->Card, LINE_LINES(0), 4);
        NVDmaNext(&_sd->Card, (msg->y1 << 16) | (msg->x1 & 0xffff));
        NVDmaNext(&_sd->Card, (msg->y2 << 16) | (msg->x2 & 0xffff));
        NVDmaNext(&_sd->Card, (msg->y2 << 16) | (msg->x2 & 0xffff));
        NVDmaNext(&_sd->Card, ((msg->y2 + 1) << 16) | ((msg->x2 + 1) & 0xffff));

	if (GC_DOCLIP(gc))
	{
	    NVDmaStart(&_sd->Card, CLIP_POINT, 2);
	    NVDmaNext(&_sd->Card, 0x00000000);
	    NVDmaNext(&_sd->Card, 0xff00ff00);
	}
	
	NVDmaKickoff(&_sd->Card);
	
	UNLOCK_HW
    }

    UNLOCK_BITMAP
}

VOID NVBM__Hidd_BitMap__PutImageLUT(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImageLUT *msg)
{
    nvBitMap *bm = OOP_INST_DATA(cl, o);

    LOCK_BITMAP

    IPTR VideoData = bm->framebuffer;

    if (bm->fbgfx)
    {
	VideoData += (IPTR)_sd->Card.FrameBuffer;

        if (_sd->gpu_busy)
        {
	    LOCK_HW
	    NVSync(_sd);
	    UNLOCK_HW
	}
    }

    switch(bm->bpp)
    {
	case 2:
	    {
		struct pHidd_BitMap_CopyLUTMemBox16 __m = {
			    _sd->mid_CopyLUTMemBox16,
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
			    _sd->mid_CopyLUTMemBox32,
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

VOID NVBM__Hidd_BitMap__BlitColorExpansion(OOP_Class *cl, OOP_Object *o, 
		struct pHidd_BitMap_BlitColorExpansion *msg)
{
    nvBitMap *bm = OOP_INST_DATA(cl, o);
    
    LOCK_BITMAP
    
    if ((OOP_OCLASS(msg->srcBitMap) == _sd->planarbmclass) && bm->fbgfx)
    {
	struct planarbm_data	*planar	= OOP_INST_DATA(OOP_OCLASS(msg->srcBitMap), msg->srcBitMap);
	HIDDT_Pixel		bg, fg;
	ULONG			cemd;
	ULONG			expand;
	ULONG			skipleft = msg->srcX - (msg->srcX & ~31);
	ULONG			mask = ~0 << bm->depth;
	
	cemd = GC_COLEXP(msg->gc);
	bg   = GC_BG(msg->gc) | mask;
	fg   = GC_FG(msg->gc) | mask;

	ULONG bw = (msg->width + 31 + skipleft) & ~31;
	ULONG x = msg->destX, y = msg->destY, w = msg->width, h = msg->height;
	
	LOCK_HW

	bm->usecount++;

	_sd->gpu_busy = TRUE;

	NVDmaStart(&_sd->Card, SURFACE_FORMAT, 4);
	NVDmaNext(&_sd->Card, bm->surface_format);
        NVDmaNext(&_sd->Card, (bm->pitch << 16) | (_sd->src_pitch));
        NVDmaNext(&_sd->Card, _sd->src_offset);
        NVDmaNext(&_sd->Card, bm->framebuffer);

	NVDmaStart(&_sd->Card, RECT_FORMAT, 1);
	NVDmaNext(&_sd->Card, bm->rect_format);
	
	_sd->surface_format = bm->surface_format;
	_sd->dst_pitch = bm->pitch;
        _sd->dst_offset = bm->framebuffer;
	
	NVSetRopSolid(_sd, GC_DRMD(msg->gc), ~0 << bm->depth);

	if (cemd & vHidd_GC_ColExp_Transparent)
	{
	    NVDmaStart(&_sd->Card, RECT_EXPAND_ONE_COLOR_CLIP, 5);
	    NVDmaNext(&_sd->Card, (y << 16) | ((x) & 0xffff));
	    NVDmaNext(&_sd->Card, ((y + h) << 16) | ((x + w) & 0xffff));
	    NVDmaNext(&_sd->Card, fg);
	    NVDmaNext(&_sd->Card, (h << 16) | bw);
	    NVDmaNext(&_sd->Card, (y << 16) | ((x-skipleft) & 0xffff));
	    expand = RECT_EXPAND_ONE_COLOR_DATA(0);
	}
	else
	{
	    NVDmaStart(&_sd->Card, RECT_EXPAND_TWO_COLOR_CLIP, 7);
	    NVDmaNext(&_sd->Card, (y << 16) | ((x) & 0xffff));
	    NVDmaNext(&_sd->Card, ((y + h) << 16) | ((x + w) & 0xffff));
	    NVDmaNext(&_sd->Card, bg);
	    NVDmaNext(&_sd->Card, fg);
	    NVDmaNext(&_sd->Card, (h << 16) | bw);
	    NVDmaNext(&_sd->Card, (h << 16) | bw);
	    NVDmaNext(&_sd->Card, (y << 16) | ((x-skipleft) & 0xffff));
	    expand = RECT_EXPAND_TWO_COLOR_DATA(0);
	}

	ULONG i,j;
	ULONG *ptr = (ULONG*)planar->planes[0];

	ptr += (planar->bytesperrow * msg->srcY >> 2) + 
		(msg->srcX >> 5);

	if ((bw >> 5) * h < RECT_EXPAND_ONE_COLOR_DATA_MAX_DWORDS)
	{
	    NVDmaStart(&_sd->Card, expand, (bw >> 5) * h);
	    for (i = 0; i < h; i++)
	    {
		for (j=0; j < (bw >> 5); j++)
		{
		    NVDmaNext(&_sd->Card, ptr[j]);
		}
		ptr += planar->bytesperrow >> 2;
	    }
	}
	else
	{
	    for (i = 0; i < h; i++)
	    {
		NVDmaStart(&_sd->Card, expand, (bw >> 5));
	
		for (j=0; j < (bw >> 5); j++)
		{
		    NVDmaNext(&_sd->Card, ptr[j]);
		}
		ptr += planar->bytesperrow >> 2;
	    }
	}

	NVDmaStart(&_sd->Card, BLIT_POINT_SRC, 1);
	NVDmaNext(&_sd->Card, 0);
	NVDmaKickoff(&_sd->Card);

	UNLOCK_HW
    }
    else
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    UNLOCK_BITMAP
}

VOID NVBM__Hidd_BitMap__DrawRect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    OOP_Object *gc = msg->gc;
    nvBitMap *bm = OOP_INST_DATA(cl, o);
    UWORD addX, addY;

    LOCK_BITMAP

    D(bug("[NVBitmap] DrawRect(%p, %d, %d, %d, %d)\n",
	bm->framebuffer, msg->minX, msg->minY, msg->maxX, msg->maxY));

    if (msg->minX == msg->maxX) addX = 1; else addX = 0;
    if (msg->minY == msg->maxY) addY = 1; else addY = 0;

    if ((GC_LINEPAT(gc) != (UWORD)~0) || !bm->fbgfx)
    {
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
    else
    {
	LOCK_HW

	_sd->gpu_busy = TRUE;
        bm->usecount++;


        _sd->Card.DMAKickoffCallback = NVDMAKickoffCallback;

	NVDmaStart(&_sd->Card, SURFACE_OFFSET_DST, 1);
	NVDmaNext(&_sd->Card, bm->framebuffer);
	_sd->dst_offset = bm->framebuffer;
	
	NVDmaStart(&_sd->Card, SURFACE_PITCH, 1);
	NVDmaNext(&_sd->Card, (bm->pitch << 16) | _sd->src_pitch);
	_sd->dst_pitch = bm->pitch;

	if (GC_DOCLIP(gc))
	{
	    NVDmaStart(&_sd->Card, CLIP_POINT, 2);
	    NVDmaNext(&_sd->Card, (GC_CLIPY1(gc) << 16) | GC_CLIPX1(gc));
	    NVDmaNext(&_sd->Card, ((GC_CLIPY2(gc)-GC_CLIPY1(gc)+1) << 16) |
				  (GC_CLIPX2(gc)-GC_CLIPX1(gc)+1));
	}
	
	NVSetRopSolid(_sd, GC_DRMD(gc), ~0 << bm->depth);
	NVDmaStart(&_sd->Card, LINE_FORMAT, 2);
	NVDmaNext(&_sd->Card, bm->line_format);
	NVDmaNext(&_sd->Card, GC_FG(gc));

	NVDmaStart(&_sd->Card, LINE_LINES(0), 8);
	
        NVDmaNext(&_sd->Card, (msg->minY << 16) | (msg->minX & 0xffff));
        NVDmaNext(&_sd->Card, (msg->minY << 16) | (msg->maxX & 0xffff));

	NVDmaNext(&_sd->Card, ((msg->minY + addY) << 16) | (msg->maxX & 0xffff));
	NVDmaNext(&_sd->Card, ((msg->maxY << 16)) | (msg->maxX & 0xffff));

	NVDmaNext(&_sd->Card, ((msg->maxY << 16)) | ((msg->maxX - addX) & 0xffff));
	NVDmaNext(&_sd->Card, ((msg->maxY << 16)) | ((msg->minX) & 0xffff));
		
        NVDmaNext(&_sd->Card, ((msg->maxY - addY) << 16) | (msg->minX & 0xffff));
        NVDmaNext(&_sd->Card, ((msg->minY + addY) << 16) | (msg->minX & 0xffff));

	if (GC_DOCLIP(gc))
	{
	    NVDmaStart(&_sd->Card, CLIP_POINT, 2);
	    NVDmaNext(&_sd->Card, 0x00000000);
	    NVDmaNext(&_sd->Card, 0xff00ff00);
	}

	NVDmaKickoff(&_sd->Card);

	UNLOCK_HW
    }

    UNLOCK_BITMAP
}

VOID NVBM__Hidd_BitMap__DrawPolygon(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawPolygon *msg)
{
    OOP_Object *gc = msg->gc;
    nvBitMap *bm = OOP_INST_DATA(cl, o);
    ULONG i;

    LOCK_BITMAP
    
    if ((GC_LINEPAT(gc) != (UWORD)~0) || !bm->fbgfx)
    {
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
    else
    {
	LOCK_HW
    
	bm->usecount++;


        _sd->Card.DMAKickoffCallback = NVDMAKickoffCallback;
	_sd->gpu_busy = TRUE;

	NVDmaStart(&_sd->Card, SURFACE_OFFSET_DST, 1);
	NVDmaNext(&_sd->Card, bm->framebuffer);
	_sd->dst_offset = bm->framebuffer;
	
	NVDmaStart(&_sd->Card, SURFACE_PITCH, 1);
	NVDmaNext(&_sd->Card, (bm->pitch << 16) | _sd->src_pitch);
	_sd->dst_pitch = bm->pitch;

	if (GC_DOCLIP(gc))
	{
	    NVDmaStart(&_sd->Card, CLIP_POINT, 2);
	    NVDmaNext(&_sd->Card, (GC_CLIPY1(gc) << 16) | GC_CLIPX1(gc));
	    NVDmaNext(&_sd->Card, ((GC_CLIPY2(gc)-GC_CLIPY1(gc)+1) << 16) |
				  (GC_CLIPX2(gc)-GC_CLIPX1(gc)+1));
	}
	
	NVSetRopSolid(_sd, GC_DRMD(gc), ~0 << bm->depth);
	NVDmaStart(&_sd->Card, LINE_FORMAT, 2);
	NVDmaNext(&_sd->Card, bm->line_format);
	NVDmaNext(&_sd->Card, GC_FG(gc));
    
	for(i = 2; i < (2 * msg->n); i = i + 2)
        {
	    NVDmaStart(&_sd->Card, LINE_LINES(0), 2);
	    NVDmaNext(&_sd->Card, (msg->coords[i - 1] << 16) | msg->coords[i - 2]);
	    NVDmaNext(&_sd->Card, (msg->coords[i + 1] << 16) | msg->coords[i]);
        }

	if (GC_DOCLIP(gc))
	{
	    NVDmaStart(&_sd->Card, CLIP_POINT, 2);
	    NVDmaNext(&_sd->Card, 0x00000000);
	    NVDmaNext(&_sd->Card, 0x7f007f00);
	}

	NVDmaKickoff(&_sd->Card);

	UNLOCK_HW
    }

    UNLOCK_BITMAP
}

VOID NVBM__Hidd_BitMap__PutImage(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImage *msg)
{
    nvBitMap *bm = OOP_INST_DATA(cl, o);

    LOCK_BITMAP

    IPTR VideoData = bm->framebuffer;

    if (bm->fbgfx)
    {
	VideoData += (IPTR)_sd->Card.FrameBuffer;

        if (_sd->gpu_busy)
        {
	    LOCK_HW
	    NVSync(_sd);
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
				    _sd->mid_CopyMemBox8,
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
				    _sd->mid_CopyMemBox16,
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
				    _sd->mid_CopyMemBox32,
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
					    _sd->mid_PutMem32Image8,
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
					    _sd->mid_PutMem32Image16,
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
				    _sd->mid_CopyMemBox32,
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

VOID NVBM__Hidd_BitMap__GetImage(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImage *msg)
{
    nvBitMap *bm = OOP_INST_DATA(cl, o);

    LOCK_BITMAP
    
    IPTR VideoData = bm->framebuffer;

    if (bm->fbgfx)
    {
	VideoData += (IPTR)_sd->Card.FrameBuffer;
	if (_sd->gpu_busy)
	{
	    LOCK_HW
	    NVSync(_sd);
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
				        _sd->mid_CopyMemBox8,
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
				        _sd->mid_CopyMemBox16,
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
				        _sd->mid_CopyMemBox32,
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
			    _sd->mid_GetMem32Image8,
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
			    _sd->mid_GetMem32Image16,
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
				        _sd->mid_CopyMemBox32,
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

VOID NVBM__Hidd_BitMap__PutTemplate(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutTemplate *msg)
{
    nvBitMap *bm = OOP_INST_DATA(cl, o);

    LOCK_BITMAP
    
    IPTR VideoData = bm->framebuffer;

    if (bm->fbgfx)
    {
	VideoData += (IPTR)_sd->Card.FrameBuffer;
	if (_sd->gpu_busy)
	{
	    LOCK_HW
	    NVSync(_sd);
	    UNLOCK_HW
	}
    }	


    switch(bm->bpp)
    {
	case 1:
	    {
		struct pHidd_BitMap_PutMemTemplate8 __m = {
				_sd->mid_PutMemTemplate8,
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
				_sd->mid_PutMemTemplate16,
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
				_sd->mid_PutMemTemplate32,
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

VOID NVBM__Hidd_BitMap__PutPattern(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutPattern *msg)
{
    nvBitMap *bm = OOP_INST_DATA(cl, o);

    LOCK_BITMAP
    
    IPTR VideoData = bm->framebuffer;

    if (bm->fbgfx)
    {
	VideoData += (IPTR)_sd->Card.FrameBuffer;
	if (_sd->gpu_busy)
	{
	    LOCK_HW
	    NVSync(_sd);
	    UNLOCK_HW
	}
    }	


    switch(bm->bpp)
    {
	case 1:
	    {
		struct pHidd_BitMap_PutMemPattern8 __m = {
				_sd->mid_PutMemPattern8,
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
				_sd->mid_PutMemPattern16,
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
				_sd->mid_PutMemPattern32,
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


BOOL NVBM__Hidd_BitMap__ObtainDirectAccess(OOP_Class *cl, OOP_Object *o,
		struct pHidd_BitMap_ObtainDirectAccess *msg)
{
    nvBitMap *bm = OOP_INST_DATA(cl, o);
    LOCK_BITMAP

    IPTR VideoData = bm->framebuffer;

    if (bm->fbgfx)
    {
	VideoData += (IPTR)_sd->Card.FrameBuffer;
	if (_sd->gpu_busy)
	{
	    LOCK_HW
	    NVSync(_sd);
	    UNLOCK_HW
	}
    }	

    *msg->addressReturn = (UBYTE*)VideoData;
    *msg->widthReturn = bm->pitch / bm->bpp;
    *msg->heightReturn = bm->height;
    *msg->bankSizeReturn = *msg->memSizeReturn = bm->pitch * bm->height;

    return TRUE;
}

VOID NVBM__Hidd_BitMap__ReleaseDirectAccess(OOP_Class *cl, OOP_Object *o,
		struct pHidd_BitMap_ReleaseDirectAccess *msg)
{
    nvBitMap *bm = OOP_INST_DATA(cl, o);



    UNLOCK_BITMAP
}


static int Init_BMMethodIDs(LIBBASETYPEPTR LIBBASE)
{
    LIBBASE->sd.mid_CopyMemBox8		= OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_CopyMemBox8);
    LIBBASE->sd.mid_CopyMemBox16	= OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_CopyMemBox16);
    LIBBASE->sd.mid_CopyMemBox32	= OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_CopyMemBox32);
    LIBBASE->sd.mid_PutMem32Image8	= OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutMem32Image8);
    LIBBASE->sd.mid_PutMem32Image16	= OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutMem32Image16);
    LIBBASE->sd.mid_GetMem32Image8	= OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_GetMem32Image8);
    LIBBASE->sd.mid_GetMem32Image16	= OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_GetMem32Image16);
    LIBBASE->sd.mid_Clear		= OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_Clear);
    LIBBASE->sd.mid_PutMemTemplate8	= OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutMemTemplate8);
    LIBBASE->sd.mid_PutMemTemplate16	= OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutMemTemplate16);
    LIBBASE->sd.mid_PutMemTemplate32	= OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutMemTemplate32);
    LIBBASE->sd.mid_PutMemPattern8	= OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutMemPattern8);
    LIBBASE->sd.mid_PutMemPattern16	= OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutMemPattern16);
    LIBBASE->sd.mid_PutMemPattern32	= OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutMemPattern32);
    LIBBASE->sd.mid_CopyLUTMemBox16	= OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_CopyLUTMemBox16);
    LIBBASE->sd.mid_CopyLUTMemBox32	= OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_CopyLUTMemBox32);
    LIBBASE->sd.mid_GetImage = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_GetImage);
    
    return TRUE;
}

ADD2INITLIB(Init_BMMethodIDs, 0)
