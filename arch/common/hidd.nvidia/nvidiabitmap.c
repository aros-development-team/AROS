/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
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

#include "nv.h"
#include "nv_dma.h"

#define DEBUG 0
#include <aros/debug.h>

#define sd ((struct staticdata*)cl->UserData)

#undef HiddPCIDeviceAttrBase
#undef HiddGfxAttrBase
#undef HiddPixFmtAttrBase
#undef HiddSyncAttrBase
#undef HiddBitMapAttrBase
#define SysBase			(sd->sysbase)
#define OOPBase			(sd->oopbase)
#define UtilityBase		(sd->utilitybase)
#define HiddPCIDeviceAttrBase	(sd->pciAttrBase)
#define HiddNVidiaBitMapAttrBase (sd->nvBitMapAttrBase)
#define HiddBitMapAttrBase	(sd->bitMapAttrBase)
#define HiddPixFmtAttrBase	(sd->pixFmtAttrBase)
#define HiddGfxAttrBase		(sd->gfxAttrBase)
#define HiddSyncAttrBase	(sd->syncAttrBase)

/* Class methods */

static OOP_Object *onbm__new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
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

	bm->framebuffer = AllocBitmapArea(sd, bm->width, bm->height,
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
	    bm->state = (struct CardState *)AllocPooled(sd->memPool, 
					sizeof(struct CardState));
	    
	    if (bm->state)
	    {
		LOCK_HW

		InitMode(sd, bm->state, 640, 480, 16, 25200, bm->framebuffer, 
		    640, 480,
		    656, 752, 800,
		    490, 492, 525);

		LoadState(sd, bm->state);
		DPMS(sd, sd->dpms);

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

		getmodemsg->mID = OOP_GetMethodID(CLID_Hidd_Gfx, moHidd_Gfx_GetMode);
		OOP_DoMethod(sd->nvobject, (OOP_Msg)getmodemsg);

		OOP_GetAttr(sync, aHidd_Sync_PixelClock, 	&pixel);
		OOP_GetAttr(sync, aHidd_Sync_HDisp, 		&hdisp);
		OOP_GetAttr(sync, aHidd_Sync_VDisp, 		&vdisp);
		OOP_GetAttr(sync, aHidd_Sync_HSyncStart, 	&hstart);
		OOP_GetAttr(sync, aHidd_Sync_VSyncStart, 	&vstart);
		OOP_GetAttr(sync, aHidd_Sync_HSyncEnd,		&hend);
		OOP_GetAttr(sync, aHidd_Sync_VSyncEnd,		&vend);
		OOP_GetAttr(sync, aHidd_Sync_HTotal,		&htotal);
		OOP_GetAttr(sync, aHidd_Sync_VTotal,		&vtotal);

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

static OOP_Object *offbm__new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
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

        bm->framebuffer = AllocBitmapArea(sd, bm->width, bm->height,
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


static VOID bm__del(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    nvBitMap *bm = OOP_INST_DATA(cl, o);

    LOCK_BITMAP
    LOCK_HW
    NVDmaKickoff(&sd->Card);
    NVSync(sd);
    
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

    OOP_DoSuperMethod(cl, o, msg);
}

static void bm__get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    nvBitMap *bm = OOP_INST_DATA(cl, o);
    ULONG idx;

    if (IS_NVBM_ATTR(msg->attrID, idx))
    {
	switch (idx)
	{
	    case aoHidd_nvBitMap_Drawable:
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

static VOID bm__clear(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_Clear *msg)
{
    nvBitMap *bm = OOP_INST_DATA(cl, o);
    
    LOCK_BITMAP

    D(bug("[NVBitMap] Clear()\n"));

    if (bm->fbgfx)
    {
	LOCK_HW

	bm->usecount++;

	sd->Card.DMAKickoffCallback = NVDMAKickoffCallback;

	NVDmaStart(&sd->Card, SURFACE_FORMAT, 4);
	NVDmaNext(&sd->Card, bm->surface_format);
	NVDmaNext(&sd->Card, (bm->pitch << 16) | sd->src_pitch);
	NVDmaNext(&sd->Card, sd->src_offset);
	NVDmaNext(&sd->Card, bm->framebuffer);
	
	sd->surface_format = bm->surface_format;
	sd->dst_pitch 	= bm->pitch;
	sd->dst_offset 	= bm->framebuffer;

	NVDmaStart(&sd->Card, RECT_FORMAT, 1);
	NVDmaNext(&sd->Card, bm->rect_format);
	sd->rect_format = bm->rect_format;
			
	NVSetRopSolid(sd, vHidd_GC_DrawMode_Copy, ~0 << bm->depth);
        NVDmaStart(&sd->Card, RECT_SOLID_COLOR, 1);
        NVDmaNext(&sd->Card, GC_BG(msg->gc));

        NVDmaStart(&sd->Card, RECT_SOLID_RECTS(0), 2);
        NVDmaNext(&sd->Card, 0);
        NVDmaNext(&sd->Card, (bm->width << 16) | (bm->height));

	NVSync(sd);

	UNLOCK_HW   
    }
    else
    {
	ULONG *ptr = (ULONG*)bm->framebuffer;
	ULONG val=0;
	int i = bm->width * bm->height * bm->bpp / 4;

	if (sd->gpu_busy)
	{
	    LOCK_HW
	    NVSync(sd);
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


static VOID bm__putpixel(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutPixel *msg)
{
    nvBitMap *bm = OOP_INST_DATA(cl, o);
    
    LOCK_BITMAP

    UBYTE *ptr = (UBYTE*)((IPTR)bm->framebuffer + bm->bpp * msg->x + bm->pitch * msg->y);

    if (bm->fbgfx)
    {
	ptr += (IPTR)sd->Card.FrameBuffer;
        if (sd->gpu_busy)
        {
            LOCK_HW
            NVSync(sd);
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

static HIDDT_Pixel bm__getpixel(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetPixel *msg)
{
    HIDDT_Pixel pixel=0;
    nvBitMap *bm = OOP_INST_DATA(cl, o);
    
    LOCK_BITMAP
    
    UBYTE *ptr = (UBYTE*)((IPTR)bm->framebuffer + bm->bpp * msg->x + bm->pitch * msg->y);

    if (bm->fbgfx)
    {
	ptr += (IPTR)sd->Card.FrameBuffer;
        if (sd->gpu_busy)
        {
            LOCK_HW
            NVSync(sd);
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

static VOID bm__fillrect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    nvBitMap *bm = OOP_INST_DATA(cl, o);

    LOCK_BITMAP

    D(bug("[NVBitMap] FillRect(%p,%d,%d,%d,%d)\n",
	bm->framebuffer, msg->minX, msg->minY, msg->maxX, msg->maxY));

    if (bm->fbgfx)
    {
	LOCK_HW

	bm->usecount++;

	sd->Card.DMAKickoffCallback = NVDMAKickoffCallback;
	sd->gpu_busy = TRUE;

	NVDmaStart(&sd->Card, SURFACE_FORMAT, 4);
	NVDmaNext(&sd->Card, bm->surface_format);
	NVDmaNext(&sd->Card, (bm->pitch << 16) | sd->src_pitch);
	NVDmaNext(&sd->Card, sd->src_offset);
	NVDmaNext(&sd->Card, bm->framebuffer);
	
	sd->surface_format = bm->surface_format;
	sd->dst_pitch 	= bm->pitch;
	sd->dst_offset 	= bm->framebuffer;

	NVDmaStart(&sd->Card, RECT_FORMAT, 1);
	NVDmaNext(&sd->Card, bm->rect_format);
	sd->rect_format = bm->rect_format;

        NVSetRopSolid(sd, GC_DRMD(msg->gc), ~0 << bm->depth);
        NVDmaStart(&sd->Card, RECT_SOLID_COLOR, 1);
        NVDmaNext(&sd->Card, GC_FG(msg->gc));

        NVDmaStart(&sd->Card, RECT_SOLID_RECTS(0), 2);
        NVDmaNext(&sd->Card, (msg->minX << 16) | (msg->minY & 0xffff));
        NVDmaNext(&sd->Card, ((msg->maxX - msg->minX + 1) << 16)
	    | ((msg->maxY - msg->minY + 1) & 0xffff));

	NVDmaKickoff(&sd->Card);
	
//	if ((msg->maxX - msg->minX) * (msg->maxY - msg->minY) > 512)
//	    NVSync(sd);
	
	UNLOCK_HW
    }
    else
    {
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }

    UNLOCK_BITMAP
}

static ULONG bm__bpl(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_BytesPerLine *msg)
{
    nvBitMap *bm = OOP_INST_DATA(cl, o);

    return (bm->bpp * msg->width + 63) & ~63;
}

static VOID bm__drawline(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawLine *msg)
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


	sd->Card.DMAKickoffCallback = NVDMAKickoffCallback;
	sd->gpu_busy = TRUE;
	
	NVDmaStart(&sd->Card, SURFACE_OFFSET_DST, 1);
	NVDmaNext(&sd->Card, bm->framebuffer);
	sd->dst_offset = bm->framebuffer;
	
	NVDmaStart(&sd->Card, SURFACE_PITCH, 1);
	NVDmaNext(&sd->Card, (bm->pitch << 16) | sd->src_pitch);
	sd->dst_pitch = bm->pitch;

	if (GC_DOCLIP(gc))
	{
	    NVDmaStart(&sd->Card, CLIP_POINT, 2);
	    NVDmaNext(&sd->Card, (GC_CLIPY1(gc) << 16) | GC_CLIPX1(gc));
	    NVDmaNext(&sd->Card, ((GC_CLIPY2(gc)-GC_CLIPY1(gc)+1) << 16) |
				  (GC_CLIPX2(gc)-GC_CLIPX1(gc)+1));
	}
	
	NVSetRopSolid(sd, GC_DRMD(gc), ~0 << bm->depth);
	NVDmaStart(&sd->Card, LINE_FORMAT, 2);
	NVDmaNext(&sd->Card, bm->line_format);
	NVDmaNext(&sd->Card, GC_FG(gc));

	NVDmaStart(&sd->Card, LINE_LINES(0), 4);
        NVDmaNext(&sd->Card, (msg->y1 << 16) | (msg->x1 & 0xffff));
        NVDmaNext(&sd->Card, (msg->y2 << 16) | (msg->x2 & 0xffff));
        NVDmaNext(&sd->Card, (msg->y2 << 16) | (msg->x2 & 0xffff));
        NVDmaNext(&sd->Card, ((msg->y2 + 1) << 16) | ((msg->x2 + 1) & 0xffff));

	if (GC_DOCLIP(gc))
	{
	    NVDmaStart(&sd->Card, CLIP_POINT, 2);
	    NVDmaNext(&sd->Card, 0x00000000);
	    NVDmaNext(&sd->Card, 0xff00ff00);
	}
	
	NVDmaKickoff(&sd->Card);
	
	UNLOCK_HW
    }

    UNLOCK_BITMAP
}

static VOID bm__putimagelut(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImageLUT *msg)
{
    D(bug("[NVBitMap] PutImageLUT(%p, %d, %d:%d, %d:%d, %p)\n",
    msg->pixels, msg->modulo, msg->x, msg->y, msg->width, msg->height, msg->pixlut));

    nvBitMap *bm = OOP_INST_DATA(cl, o);
    
    LOCK_BITMAP 
    
    ULONG *ptr = (ULONG*)sd->scratch_buffer;

    WORD                    x, y;
    UBYTE                   *pixarray = (UBYTE *)msg->pixels;
    HIDDT_PixelLUT          *pixlut = msg->pixlut;
    HIDDT_Pixel             *lut = pixlut ? pixlut->pixels : NULL;
    OOP_Object              *gc = msg->gc;
   
    if ((ptr != (ULONG*)0xffffffff) && bm->fbgfx)
    {
	ULONG *cpuptr = (ULONG*)((IPTR)ptr + sd->Card.FrameBuffer);
    
	LOCK_HW
    
	bm->usecount++;


	sd->gpu_busy = TRUE;

	NVSetRopSolid(sd, GC_DRMD(gc), ~0 << bm->depth);
	
	NVDmaStart(&sd->Card, SURFACE_FORMAT, 4);
	NVDmaNext(&sd->Card, bm->surface_format);
        NVDmaNext(&sd->Card, (bm->pitch << 16) | ((msg->width * bm->bpp + 63) & ~63));
        NVDmaNext(&sd->Card, (IPTR)ptr);
        NVDmaNext(&sd->Card, bm->framebuffer);
	
	NVDmaStart(&sd->Card, RECT_FORMAT, 1);
	NVDmaNext(&sd->Card, bm->rect_format);

	sd->rect_format = bm->rect_format;
	sd->surface_format = bm->surface_format;
	sd->dst_pitch = bm->pitch;
	sd->src_pitch = (msg->width * bm->bpp + 63) & ~63;
	sd->src_offset = (IPTR)ptr;
        sd->dst_offset = bm->framebuffer;
	
	for (y=0; y < msg->height; y++)
	{    
	    sd->Card.DMAKickoffCallback = NVDMAKickoffCallback;

	    if (lut)
	    {
		if (bm->bpp == 4)
		{
		    for (x=0; x < msg->width; x++)
		    {
		        cpuptr[x] = lut[pixarray[x]];
		    }
		}
		else if (bm->bpp == 2)
		{
		    for (x=0; x < msg->width; x++)
		    {
			((UWORD*)cpuptr)[x] = lut[pixarray[x]];
		    }
		}
		else
		{
		    for (x=0; x < msg->width; x++)
		    {
			((UBYTE*)cpuptr)[x] = lut[pixarray[x]];
		    }
		}

	    }
	    else
	    {
		for (x=0; x < msg->width; x++)
		{
		    cpuptr[x] = pixarray[x];
		}
	    }
	    pixarray += msg->modulo;

	    NVDmaStart(&sd->Card, BLIT_POINT_SRC, 3);
	    NVDmaNext(&sd->Card, (0 << 16) | (0 & 0xffff));
	    NVDmaNext(&sd->Card, ((msg->y + y) << 16) | (msg->x & 0xffff));
	    NVDmaNext(&sd->Card, (1 << 16) | (msg->width & 0xffff));
	    
	    NVSync(sd);
	}
	
	UNLOCK_HW
    }
    else OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    UNLOCK_BITMAP
}

static VOID bm__blitcolexp(OOP_Class *cl, OOP_Object *o, 
		struct pHidd_BitMap_BlitColorExpansion *msg)
{
    nvBitMap *bm = OOP_INST_DATA(cl, o);
    
    LOCK_BITMAP
    
    if ((OOP_OCLASS(msg->srcBitMap) == sd->planarbmclass) && bm->fbgfx)
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

	sd->gpu_busy = TRUE;

	NVDmaStart(&sd->Card, SURFACE_FORMAT, 4);
	NVDmaNext(&sd->Card, bm->surface_format);
        NVDmaNext(&sd->Card, (bm->pitch << 16) | (sd->src_pitch));
        NVDmaNext(&sd->Card, sd->src_offset);
        NVDmaNext(&sd->Card, bm->framebuffer);

	NVDmaStart(&sd->Card, RECT_FORMAT, 1);
	NVDmaNext(&sd->Card, bm->rect_format);
	
	sd->surface_format = bm->surface_format;
	sd->dst_pitch = bm->pitch;
        sd->dst_offset = bm->framebuffer;
	
	NVSetRopSolid(sd, GC_DRMD(msg->gc), ~0 << bm->depth);

	if (cemd & vHidd_GC_ColExp_Transparent)
	{
	    NVDmaStart(&sd->Card, RECT_EXPAND_ONE_COLOR_CLIP, 5);
	    NVDmaNext(&sd->Card, (y << 16) | ((x) & 0xffff));
	    NVDmaNext(&sd->Card, ((y + h) << 16) | ((x + w) & 0xffff));
	    NVDmaNext(&sd->Card, fg);
	    NVDmaNext(&sd->Card, (h << 16) | bw);
	    NVDmaNext(&sd->Card, (y << 16) | ((x-skipleft) & 0xffff));
	    expand = RECT_EXPAND_ONE_COLOR_DATA(0);
	}
	else
	{
	    NVDmaStart(&sd->Card, RECT_EXPAND_TWO_COLOR_CLIP, 7);
	    NVDmaNext(&sd->Card, (y << 16) | ((x) & 0xffff));
	    NVDmaNext(&sd->Card, ((y + h) << 16) | ((x + w) & 0xffff));
	    NVDmaNext(&sd->Card, bg);
	    NVDmaNext(&sd->Card, fg);
	    NVDmaNext(&sd->Card, (h << 16) | bw);
	    NVDmaNext(&sd->Card, (h << 16) | bw);
	    NVDmaNext(&sd->Card, (y << 16) | ((x-skipleft) & 0xffff));
	    expand = RECT_EXPAND_TWO_COLOR_DATA(0);
	}

	ULONG i,j;
	ULONG *ptr = (ULONG*)planar->planes[0];

	ptr += (planar->bytesperrow * msg->srcY >> 2) + 
		(msg->srcX >> 5);

	if ((bw >> 5) * h < RECT_EXPAND_ONE_COLOR_DATA_MAX_DWORDS)
	{
	    NVDmaStart(&sd->Card, expand, (bw >> 5) * h);
	    for (i = 0; i < h; i++)
	    {
		for (j=0; j < (bw >> 5); j++)
		{
		    NVDmaNext(&sd->Card, ptr[j]);
		}
		ptr += planar->bytesperrow >> 2;
	    }
	}
	else
	{
	    for (i = 0; i < h; i++)
	    {
		NVDmaStart(&sd->Card, expand, (bw >> 5));
	
		for (j=0; j < (bw >> 5); j++)
		{
		    NVDmaNext(&sd->Card, ptr[j]);
		}
		ptr += planar->bytesperrow >> 2;
	    }
	}

	NVDmaStart(&sd->Card, BLIT_POINT_SRC, 1);
	NVDmaNext(&sd->Card, 0);
	NVDmaKickoff(&sd->Card);

	UNLOCK_HW
    }
    else
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    UNLOCK_BITMAP
}

static VOID bm__drawrect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawRect *msg)
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

	sd->gpu_busy = TRUE;
        bm->usecount++;


        sd->Card.DMAKickoffCallback = NVDMAKickoffCallback;

	NVDmaStart(&sd->Card, SURFACE_OFFSET_DST, 1);
	NVDmaNext(&sd->Card, bm->framebuffer);
	sd->dst_offset = bm->framebuffer;
	
	NVDmaStart(&sd->Card, SURFACE_PITCH, 1);
	NVDmaNext(&sd->Card, (bm->pitch << 16) | sd->src_pitch);
	sd->dst_pitch = bm->pitch;

	if (GC_DOCLIP(gc))
	{
	    NVDmaStart(&sd->Card, CLIP_POINT, 2);
	    NVDmaNext(&sd->Card, (GC_CLIPY1(gc) << 16) | GC_CLIPX1(gc));
	    NVDmaNext(&sd->Card, ((GC_CLIPY2(gc)-GC_CLIPY1(gc)+1) << 16) |
				  (GC_CLIPX2(gc)-GC_CLIPX1(gc)+1));
	}
	
	NVSetRopSolid(sd, GC_DRMD(gc), ~0 << bm->depth);
	NVDmaStart(&sd->Card, LINE_FORMAT, 2);
	NVDmaNext(&sd->Card, bm->line_format);
	NVDmaNext(&sd->Card, GC_FG(gc));

	NVDmaStart(&sd->Card, LINE_LINES(0), 8);
	
        NVDmaNext(&sd->Card, (msg->minY << 16) | (msg->minX & 0xffff));
        NVDmaNext(&sd->Card, (msg->minY << 16) | (msg->maxX & 0xffff));

	NVDmaNext(&sd->Card, ((msg->minY + addY) << 16) | (msg->maxX & 0xffff));
	NVDmaNext(&sd->Card, ((msg->maxY << 16)) | (msg->maxX & 0xffff));

	NVDmaNext(&sd->Card, ((msg->maxY << 16)) | ((msg->maxX - addX) & 0xffff));
	NVDmaNext(&sd->Card, ((msg->maxY << 16)) | ((msg->minX) & 0xffff));
		
        NVDmaNext(&sd->Card, ((msg->maxY - addY) << 16) | (msg->minX & 0xffff));
        NVDmaNext(&sd->Card, ((msg->minY + addY) << 16) | (msg->minX & 0xffff));

	if (GC_DOCLIP(gc))
	{
	    NVDmaStart(&sd->Card, CLIP_POINT, 2);
	    NVDmaNext(&sd->Card, 0x00000000);
	    NVDmaNext(&sd->Card, 0xff00ff00);
	}

	NVDmaKickoff(&sd->Card);

	UNLOCK_HW
    }

    UNLOCK_BITMAP
}

static VOID bm__drawpoly(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawPolygon *msg)
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


        sd->Card.DMAKickoffCallback = NVDMAKickoffCallback;
	sd->gpu_busy = TRUE;

	NVDmaStart(&sd->Card, SURFACE_OFFSET_DST, 1);
	NVDmaNext(&sd->Card, bm->framebuffer);
	sd->dst_offset = bm->framebuffer;
	
	NVDmaStart(&sd->Card, SURFACE_PITCH, 1);
	NVDmaNext(&sd->Card, (bm->pitch << 16) | sd->src_pitch);
	sd->dst_pitch = bm->pitch;

	if (GC_DOCLIP(gc))
	{
	    NVDmaStart(&sd->Card, CLIP_POINT, 2);
	    NVDmaNext(&sd->Card, (GC_CLIPY1(gc) << 16) | GC_CLIPX1(gc));
	    NVDmaNext(&sd->Card, ((GC_CLIPY2(gc)-GC_CLIPY1(gc)+1) << 16) |
				  (GC_CLIPX2(gc)-GC_CLIPX1(gc)+1));
	}
	
	NVSetRopSolid(sd, GC_DRMD(gc), ~0 << bm->depth);
	NVDmaStart(&sd->Card, LINE_FORMAT, 2);
	NVDmaNext(&sd->Card, bm->line_format);
	NVDmaNext(&sd->Card, GC_FG(gc));
    
	for(i = 2; i < (2 * msg->n); i = i + 2)
        {
	    NVDmaStart(&sd->Card, LINE_LINES(0), 2);
	    NVDmaNext(&sd->Card, (msg->coords[i - 1] << 16) | msg->coords[i - 2]);
	    NVDmaNext(&sd->Card, (msg->coords[i + 1] << 16) | msg->coords[i]);
        }

	if (GC_DOCLIP(gc))
	{
	    NVDmaStart(&sd->Card, CLIP_POINT, 2);
	    NVDmaNext(&sd->Card, 0x00000000);
	    NVDmaNext(&sd->Card, 0x7f007f00);
	}

	NVDmaKickoff(&sd->Card);

	UNLOCK_HW
    }

    UNLOCK_BITMAP
}

static VOID bm__putimage(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImage *msg)
{
    nvBitMap *bm = OOP_INST_DATA(cl, o);

    LOCK_BITMAP

    IPTR VideoData = bm->framebuffer;

    if (bm->fbgfx)
    {
	VideoData += (IPTR)sd->Card.FrameBuffer;

        if (sd->gpu_busy)
        {
	    LOCK_HW
	    NVSync(sd);
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

static VOID bm__getimage(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImage *msg)
{
    nvBitMap *bm = OOP_INST_DATA(cl, o);

    LOCK_BITMAP
    
    IPTR VideoData = bm->framebuffer;

    if (bm->fbgfx)
    {
	VideoData += (IPTR)sd->Card.FrameBuffer;
	if (sd->gpu_busy)
	{
	    LOCK_HW
	    NVSync(sd);
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

#undef sd
/* Class related functions */

#define NUM_ROOT_METHODS    3
#define	NUM_BM_METHODS	    12

OOP_Class *init_onbitmapclass(struct staticdata *sd)
{
    OOP_Class *cl = NULL;

    struct OOP_MethodDescr root_descr[NUM_ROOT_METHODS + 1] = {
	{ OOP_METHODDEF(onbm__new), moRoot_New },
	{ OOP_METHODDEF(bm__get),   moRoot_Get },
	{ OOP_METHODDEF(bm__del),   moRoot_Dispose },
	{ NULL, 0 }
    };

    struct OOP_MethodDescr gfx_descr[NUM_BM_METHODS + 1] = {
	{ OOP_METHODDEF(bm__putpixel),	moHidd_BitMap_PutPixel },
	{ OOP_METHODDEF(bm__getpixel),	moHidd_BitMap_GetPixel },
	{ OOP_METHODDEF(bm__clear),	moHidd_BitMap_Clear },
	{ OOP_METHODDEF(bm__fillrect),	moHidd_BitMap_FillRect },
	{ OOP_METHODDEF(bm__bpl),	moHidd_BitMap_BytesPerLine },
	{ OOP_METHODDEF(bm__drawline),	moHidd_BitMap_DrawLine },
	{ OOP_METHODDEF(bm__putimagelut),moHidd_BitMap_PutImageLUT },
	{ OOP_METHODDEF(bm__blitcolexp),moHidd_BitMap_BlitColorExpansion },
	{ OOP_METHODDEF(bm__drawrect),	moHidd_BitMap_DrawRect },
	{ OOP_METHODDEF(bm__drawpoly),	moHidd_BitMap_DrawPolygon },
	{ OOP_METHODDEF(bm__putimage),	moHidd_BitMap_PutImage },
	{ OOP_METHODDEF(bm__getimage),	moHidd_BitMap_GetImage },
	{ NULL, 0 }
    };

    struct OOP_InterfaceDescr ifdescr[] = {
	{ root_descr,	IID_Root,	NUM_ROOT_METHODS },
	{ gfx_descr,	IID_Hidd_BitMap,NUM_BM_METHODS	 },
	{ NULL, NULL, 0 }
    };

    OOP_AttrBase MetaAttrBase = OOP_ObtainAttrBase(IID_Meta);

    struct TagItem tags[] = {
	{ aMeta_SuperID,	(IPTR)CLID_Hidd_BitMap },
	{ aMeta_InterfaceDescr,	(IPTR)ifdescr },
	{ aMeta_InstSize,	sizeof(nvBitMap) },
	{ TAG_DONE, 0UL }
    };

    EnterFunc(bug("[NVidia] hidd.bitmap.nv (onbm) class init.\n"));

    if (MetaAttrBase)
    {
	cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);

	sd->mid_CopyMemBox8	= OOP_GetMethodID(CLID_Hidd_BitMap, moHidd_BitMap_CopyMemBox8);
	sd->mid_CopyMemBox16	= OOP_GetMethodID(CLID_Hidd_BitMap, moHidd_BitMap_CopyMemBox16);
	sd->mid_CopyMemBox32	= OOP_GetMethodID(CLID_Hidd_BitMap, moHidd_BitMap_CopyMemBox32);
	sd->mid_PutMem32Image8	= OOP_GetMethodID(CLID_Hidd_BitMap, moHidd_BitMap_PutMem32Image8);
	sd->mid_PutMem32Image16 = OOP_GetMethodID(CLID_Hidd_BitMap, moHidd_BitMap_PutMem32Image16);
	sd->mid_GetMem32Image8	= OOP_GetMethodID(CLID_Hidd_BitMap, moHidd_BitMap_GetMem32Image8);
	sd->mid_GetMem32Image16 = OOP_GetMethodID(CLID_Hidd_BitMap, moHidd_BitMap_GetMem32Image16);

	if (cl)
	{
	    cl->UserData = sd;
	    sd->onbmclass = cl;

	    OOP_AddClass(cl);
	}
	
	OOP_ReleaseAttrBase(IID_Meta);
    }

    D(bug("[NVidia] init_nvonbitmapclass=%p\n", cl));

    return cl;
}

#define NUM_OFFROOT_METHODS	3
#define	NUM_OFFBM_METHODS	12

OOP_Class *init_offbitmapclass(struct staticdata *sd)
{
    OOP_Class *cl = NULL;

    struct OOP_MethodDescr root_descr[NUM_OFFROOT_METHODS + 1] = {
	{ OOP_METHODDEF(offbm__new),	moRoot_New },
	{ OOP_METHODDEF(bm__get),	moRoot_Get },
	{ OOP_METHODDEF(bm__del),	moRoot_Dispose },
	{ NULL, 0 }
    };

    struct OOP_MethodDescr gfx_descr[NUM_OFFBM_METHODS + 1] = {
	{ OOP_METHODDEF(bm__putpixel),	moHidd_BitMap_PutPixel },
	{ OOP_METHODDEF(bm__getpixel),	moHidd_BitMap_GetPixel },
	{ OOP_METHODDEF(bm__clear),	moHidd_BitMap_Clear },
	{ OOP_METHODDEF(bm__fillrect),	moHidd_BitMap_FillRect },
	{ OOP_METHODDEF(bm__bpl),	moHidd_BitMap_BytesPerLine },
	{ OOP_METHODDEF(bm__drawline),	moHidd_BitMap_DrawLine },
	{ OOP_METHODDEF(bm__putimagelut),moHidd_BitMap_PutImageLUT },
	{ OOP_METHODDEF(bm__blitcolexp),moHidd_BitMap_BlitColorExpansion },
	{ OOP_METHODDEF(bm__drawrect),	moHidd_BitMap_DrawRect },
	{ OOP_METHODDEF(bm__drawpoly),	moHidd_BitMap_DrawPolygon },
	{ OOP_METHODDEF(bm__putimage),	moHidd_BitMap_PutImage },
	{ OOP_METHODDEF(bm__getimage),	moHidd_BitMap_GetImage },
	{ NULL, 0 }
    };

    struct OOP_InterfaceDescr ifdescr[] = {
	{ root_descr,	IID_Root,	NUM_OFFROOT_METHODS },
	{ gfx_descr,	IID_Hidd_BitMap,NUM_OFFBM_METHODS	 },
	{ NULL, NULL, 0 }
    };

    OOP_AttrBase MetaAttrBase = OOP_ObtainAttrBase(IID_Meta);

    struct TagItem tags[] = {
	{ aMeta_SuperID,	(IPTR)CLID_Hidd_BitMap },
	{ aMeta_InterfaceDescr,	(IPTR)ifdescr },
	{ aMeta_InstSize,	sizeof(nvBitMap) },
	{ TAG_DONE, 0UL }
    };

    EnterFunc(bug("[NVidia] hidd.bitmap.nv (offbm) class init.\n"));

    if (MetaAttrBase)
    {
	cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);

	if (cl)
	{
	    cl->UserData = sd;
	    sd->offbmclass = cl;

	    OOP_AddClass(cl);
	}
	
	OOP_ReleaseAttrBase(IID_Meta);
    }

    D(bug("[NVidia] init_nvoffbitmapclass=%p\n", cl));

    return cl;
}

