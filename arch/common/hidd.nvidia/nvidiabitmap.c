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

	OOP_Object *pf;

	D(bug("[NVBitMap] Super called. o=%p\n", o));

	bm->onbm = TRUE;

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

	if (bm->framebuffer != 0xffffffff)
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
		    LOCK_ALL
		    LOCK_HW
		    
		    InitMode(sd, bm->state, width, height, depth, pixel, bm->framebuffer,
			hdisp, vdisp,
			hstart, hend, htotal,
			vstart, vend, vtotal);

		    LoadState(sd, bm->state);
		    DPMS(sd, sd->dpms);

		    UNLOCK_HW
		    UNLOCK_ALL

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
	ULONG fb;

	OOP_Object *pf;

	bm->onbm = FALSE;

	fb = GetTagData(aHidd_BitMap_FrameBuffer, FALSE, msg->attrList);

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

	if (!fb)
	{
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
	}
	else
	{
	    D(bug("[NVBitmap] Dummy framebuffer object without bitmap memory\n"));
	    bm->fbgfx = FALSE;
	    bm->framebuffer = 0;
	}
	
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

	if ((bm->framebuffer != 0xffffffff && bm->framebuffer != 0) || fb)
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

    LOCK_ALL

    LOCK_HW
    NVSync(sd);
    UNLOCK_HW
    
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

    UNLOCK_ALL

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
    bm->usecount++;

    D(bug("[NVBitMap] Clear()\n"));

    if (bm->fbgfx)
    {
	LOCK_HW

	sd->Card.DMAKickoffCallback = NVDMAKickoffCallback;

	if (bm->surface_format != sd->surface_format)
	{
	    NVDmaStart(&sd->Card, SURFACE_FORMAT, 1);
	    NVDmaNext(&sd->Card, bm->surface_format);
	    sd->surface_format = bm->surface_format;
	    D(bug("[NVBitMap] surface_format <- %d\n", sd->surface_format));
	}
        if (bm->pitch != sd->dst_pitch)
        {
	    NVDmaStart(&sd->Card, SURFACE_PITCH, 1);
	    NVDmaNext(&sd->Card, (bm->pitch << 16) | sd->src_pitch);
	    sd->dst_pitch = bm->pitch;
	    D(bug("[NVBitMap] pitch <- %08x\n", (bm->pitch << 16) | sd->src_pitch));
	}
	if (bm->rect_format != sd->rect_format)
	{
	    NVDmaStart(&sd->Card, RECT_FORMAT, 1);
	    NVDmaNext(&sd->Card, bm->rect_format);
	    sd->rect_format = bm->rect_format;
	    D(bug("[NVBitMap] rect_format <- %d\n", sd->rect_format));
	}
	if (bm->framebuffer != sd->dst_offset)
	{
	    NVDmaStart(&sd->Card, SURFACE_OFFSET_DST, 1);
	    NVDmaNext(&sd->Card, bm->framebuffer);
	    sd->dst_offset = bm->framebuffer;
	    D(bug("[NVBitMap] dst_offset <- %p\n", sd->dst_offset));
	}
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
}


static VOID bm__putpixel(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutPixel *msg)
{
    nvBitMap *bm = OOP_INST_DATA(cl, o);
    UBYTE *ptr = (UBYTE*)((IPTR)bm->framebuffer + bm->bpp * msg->x + bm->pitch * msg->y);
    bm->usecount++;

#if 0
D(bug("[NVBitMap] PutPixel %d:%d-%x (pitch %d, bpp %d) @ %p\n", msg->x, msg->y, msg->pixel, 
	bm->pitch, bm->bpp, ptr));
#endif
    if (bm->fbgfx)
	ptr += (IPTR)sd->Card.FrameBuffer;

    LOCK_HW
    
    NVSync(sd);

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
    
    UNLOCK_HW
}

static HIDDT_Pixel bm__getpixel(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetPixel *msg)
{
    HIDDT_Pixel pixel=0;
    nvBitMap *bm = OOP_INST_DATA(cl, o);
    UBYTE *ptr = (UBYTE*)((IPTR)bm->framebuffer + bm->bpp * msg->x + bm->pitch * msg->y);
    bm->usecount++;
 
    if (bm->fbgfx)
	ptr += (IPTR)sd->Card.FrameBuffer;
 
    LOCK_HW
    
    NVSync(sd);

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

    UNLOCK_HW
    
    /* Get pen number from colortab */
    return pixel;
}

static VOID bm__fillrect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    nvBitMap *bm = OOP_INST_DATA(cl, o);
    bm->usecount++;

    D(bug("[NVBitMap] FillRect(%p,%d,%d,%d,%d)\n",
	bm->framebuffer, msg->minX, msg->minY, msg->maxX, msg->maxY));

    if (bm->fbgfx)
    {
	LOCK_HW

	sd->Card.DMAKickoffCallback = NVDMAKickoffCallback;

	if (bm->surface_format != sd->surface_format)
	{
	    NVDmaStart(&sd->Card, SURFACE_FORMAT, 1);
	    NVDmaNext(&sd->Card, bm->surface_format);
	    sd->surface_format = bm->surface_format;
	    D(bug("[NVBitMap] surface_format <- %d\n", sd->surface_format));
	}
        if (bm->pitch != sd->dst_pitch)
        {
	    NVDmaStart(&sd->Card, SURFACE_PITCH, 1);
	    NVDmaNext(&sd->Card, (bm->pitch << 16) | sd->src_pitch);
	    sd->dst_pitch = bm->pitch;
	    D(bug("[NVBitMap] pitch <- %08x\n", (bm->pitch << 16) | sd->src_pitch));
	}
	if (bm->rect_format != sd->rect_format)
	{
	    NVDmaStart(&sd->Card, RECT_FORMAT, 1);
	    NVDmaNext(&sd->Card, bm->rect_format);
	    sd->rect_format = bm->rect_format;
	    D(bug("[NVBitMap] rect_format <- %d\n", sd->rect_format));
	}
	if (bm->framebuffer != sd->dst_offset)
	{
	    NVDmaStart(&sd->Card, SURFACE_OFFSET_DST, 1);
	    NVDmaNext(&sd->Card, bm->framebuffer);
	    sd->dst_offset = bm->framebuffer;
	    D(bug("[NVBitMap] dst_offset <- %p\n", sd->dst_offset));
	}
        NVSetRopSolid(sd, GC_DRMD(msg->gc), ~0 << bm->depth);
        NVDmaStart(&sd->Card, RECT_SOLID_COLOR, 1);
        NVDmaNext(&sd->Card, GC_FG(msg->gc));

        NVDmaStart(&sd->Card, RECT_SOLID_RECTS(0), 2);
        NVDmaNext(&sd->Card, (msg->minX << 16) | (msg->minY & 0xffff));
        NVDmaNext(&sd->Card, ((msg->maxX - msg->minX + 1) << 16)
	    | ((msg->maxY - msg->minY + 1) & 0xffff));

	NVSync(sd);
	
	UNLOCK_HW
    }
    else
    {
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
}

static ULONG bm__bpl(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_BytesPerLine *msg)
{
    nvBitMap *bm = OOP_INST_DATA(cl, o);
    bm->usecount++;

    return (bm->bpp * msg->width + 63) & ~63;
}

static VOID bm__drawline(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawLine *msg)
{
    OOP_Object *gc = msg->gc;
    nvBitMap *bm = OOP_INST_DATA(cl, o);
    
    bm->usecount++;

    D(bug("[NVBitmap] DrawLine(%p, %d, %d, %d, %d)\n",
	bm->framebuffer, msg->x1, msg->y1, msg->x2, msg->y2));

    if ((GC_LINEPAT(gc) != (UWORD)~0) || !bm->fbgfx)
    {
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
    else
    {
	LOCK_HW
	
	sd->Card.DMAKickoffCallback = NVDMAKickoffCallback;

	if (sd->dst_offset != bm->framebuffer)
	{
	    NVDmaStart(&sd->Card, SURFACE_OFFSET_DST, 1);
	    NVDmaNext(&sd->Card, bm->framebuffer);
	    sd->dst_offset = bm->framebuffer;
	    D(bug("[NVBitMap] dst_offset <- %p\n", sd->dst_offset));
	}

	if (GC_DOCLIP(gc))
	{
	    NVDmaStart(&sd->Card, CLIP_POINT, 2);
	    NVDmaNext(&sd->Card, (GC_CLIPY1(gc) << 16) | GC_CLIPX1(gc));
	    NVDmaNext(&sd->Card, ((GC_CLIPY2(gc)-GC_CLIPY1(gc)+1) << 16) |
				  (GC_CLIPX2(gc)-GC_CLIPX1(gc)+1));
	}
	
	NVSetRopSolid(sd, GC_DRMD(gc), ~0 << bm->depth);
	NVDmaStart(&sd->Card, LINE_COLOR, 1);
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
	    NVDmaNext(&sd->Card, 0xffffffff);
	}
	
	NVSync(sd);
	
	UNLOCK_HW
    }
}
#if 0
static VOID bm__putimagelut(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImageLUT *msg)
{
    D(bug("[NVBitMap] PutImageLUT(%p, %d, %d:%d, %d:%d, %p)\n",
    msg->pixels, msg->modulo, msg->x, msg->y, msg->width, msg->height, msg->pixlut));

    nvBitMap *bm = OOP_INST_DATA(cl, o);
    ULONG *ptr = sd->scratch_buffer;
	//(ULONG*)AllocBitmapArea(sd, msg->width, 1, 4, TRUE);

    WORD                    x, y;
    UBYTE                   *pixarray = (UBYTE *)msg->pixels;
    HIDDT_PixelLUT          *pixlut = msg->pixlut;
    HIDDT_Pixel             *lut = pixlut ? pixlut->pixels : NULL;
    OOP_Object              *gc = msg->gc;
    ULONG		    stretch_format;
    
    bm->usecount++;
    
    if (ptr != 0xffffffff)
    {
	ULONG *cpuptr = (ULONG*)((IPTR)ptr + sd->Card.FrameBuffer);
    
	LOCK_HW

	sd->Card.DMAKickoffCallback = NVDMAKickoffCallback;

	if (bm->depth <= 8)
	{
	    stretch_format = STRETCH_BLIT_FORMAT_DEPTH8;
	}
        else if (bm->depth == 15)
	{
	    stretch_format = STRETCH_BLIT_FORMAT_DEPTH15;
	}
	else if (bm->depth == 16)
	{
	    stretch_format = STRETCH_BLIT_FORMAT_DEPTH16;
	}
        else
	{
	    stretch_format = STRETCH_BLIT_FORMAT_DEPTH24;
	}

	NVSetRopSolid(sd, GC_DRMD(gc), ~0);
	
	if ((bm->surface_format != sd->surface_format))
	{
	    NVDmaStart(&sd->Card, SURFACE_FORMAT, 1);
	    NVDmaNext(&sd->Card, bm->surface_format);
	    sd->surface_format = bm->surface_format;
	    D(bug("[NVidia] surface_format <- %d\n", sd->surface_format));
	}

	if (bm->depth == 15)
	{
	    NVDmaStart(&sd->Card, SURFACE_FORMAT, 1);
	    NVDmaNext(&sd->Card, SURFACE_FORMAT_DEPTH15);
	    sd->surface_format = SURFACE_FORMAT_DEPTH16;
	}
    	
	if (bm->pitch != sd->dst_pitch)
	{
	    NVDmaStart(&sd->Card, SURFACE_PITCH, 1);
	    NVDmaNext(&sd->Card, (bm->pitch << 16) | sd->src_pitch);
	    sd->dst_pitch = bm->pitch;
	    D(bug("[NVidia] pitch <- %08x\n", (sd->dst_pitch << 16) | sd->src_pitch));
	}

	if (bm->framebuffer != sd->dst_offset)
	{
	    NVDmaStart(&sd->Card, SURFACE_OFFSET_DST, 1);
	    NVDmaNext(&sd->Card, bm->framebuffer);
	    sd->dst_offset = bm->framebuffer;
	    D(bug("[NVidia] dst_offset=%p\n", sd->dst_offset));
	}
	
	for (y=0; y < msg->height; y++)
	{
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
	    //(bug("line %d\n", y));
	    pixarray += msg->modulo;

	    NVDmaStart(&sd->Card, RECT_SOLID_COLOR, 1);
	    NVDmaNext(&sd->Card, 0);

	    NVDmaStart(&sd->Card, STRETCH_BLIT_FORMAT, 1);
	    NVDmaNext(&sd->Card, stretch_format);

	    NVDmaStart(&sd->Card, STRETCH_BLIT_CLIP_POINT, 6);
	    NVDmaNext(&sd->Card, 0x00000000);    // dst_CLip
	    NVDmaNext(&sd->Card, 0xffffffff);    // dst_Clip
	    NVDmaNext(&sd->Card, ((msg->y + y) << 16) | (msg->x));// dst_y | dst_x
	    NVDmaNext(&sd->Card, (1 << 16)| (msg->width));// dst_h | dst_w
	    NVDmaNext(&sd->Card, 1 << 20);  // src_w / dst_w 1:1
	    NVDmaNext(&sd->Card, 1 << 20);  // src_h / dst_h 1:1

	    NVDmaStart(&sd->Card, STRETCH_BLIT_SRC_SIZE, 4);
	    NVDmaNext(&sd->Card, (1 << 16) | ((msg->width+1) & 0xfffe));// src_h | src_w
	    NVDmaNext(&sd->Card, 
		(STRETCH_BLIT_SRC_FORMAT_FILTER_POINT_SAMPLE << 24) |   // BILINEAR | _POINT_SAMPLE
		(STRETCH_BLIT_SRC_FORMAT_ORIGIN_CORNER << 16) |
		((msg->width * sizeof(HIDDT_Pixel)+63)& ~63));		    // src_pitch
	    NVDmaNext(&sd->Card, ptr);		    // src_offset
	    NVDmaNext(&sd->Card, 0); // src_y | src_x

	    NVDmaKickoff(&sd->Card);
	}
	
	if (bm->depth == 15)
	{
	    NVDmaStart(&sd->Card, SURFACE_FORMAT, 1);
	    NVDmaNext(&sd->Card, SURFACE_FORMAT_DEPTH16);
	}
	UNLOCK_HW

	NVSync(sd);

/*	NVSetRopSolid(sd, vHidd_GC_DrawMode_Copy, ~0);

	NVDmaStart(&sd->Card, SURFACE_OFFSET_DST, 1);
	NVDmaNext(&sd->Card, 0x00040000);
	sd->dst_offset = 0x40000;
	
        NVDmaStart(&sd->Card, SURFACE_PITCH, 1);
	NVDmaNext(&sd->Card, (800 * 2 << 16) | sd->src_pitch);
	sd->dst_pitch = 800*4;

	NVDmaStart(&sd->Card, RECT_FORMAT, 1);
	NVDmaNext(&sd->Card, RECT_FORMAT_DEPTH16);

	NVDmaStart(&sd->Card, RECT_EXPAND_ONE_COLOR_CLIP, 5)
	NVDmaNext(&sd->Card, (4 << 16) | 3);
	NVDmaNext(&sd->Card, (8 << 16) | 36);
	NVDmaNext(&sd->Card, 0xff000000);
	NVDmaNext(&sd->Card, (4 << 16) | 32);
	NVDmaNext(&sd->Card, (4 << 16) | 3);

	NVDmaStart(&sd->Card, RECT_EXPAND_ONE_COLOR_DATA(0), 4);
	NVDmaNext(&sd->Card, 0xaaaaaaaa);
	NVDmaNext(&sd->Card, 0xaaaaaaaa);
	NVDmaNext(&sd->Card, 0x55555555);
	NVDmaNext(&sd->Card, 0x55555555);

	NVDmaStart(&sd->Card, BLIT_POINT_SRC, 1);
	NVDmaNext(&sd->Card, 0);

	NVDmaKickoff(&sd->Card);
	NVSync(sd);
*/	
    }
    else OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}
#endif

static VOID _bm__putimagelut(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImageLUT *msg)
{
    if (cl == sd->offbmclass)
    {
	D(bug("offbmclass: "));
    }

    D(bug("[NVBitMap] PutImageLUT(%p, %d, %d:%d, %d:%d, %p)\n",
    msg->pixels, msg->modulo, msg->x, msg->y, msg->width, msg->height, msg->pixlut));

    nvBitMap *bm = OOP_INST_DATA(cl, o);
    ULONG *ptr = (ULONG*)sd->scratch_buffer;
//	(ULONG*)AllocBitmapArea(sd, msg->width, 1, 4, TRUE);

    WORD                    x, y;
    UBYTE                   *pixarray = (UBYTE *)msg->pixels;
    HIDDT_PixelLUT          *pixlut = msg->pixlut;
    HIDDT_Pixel             *lut = pixlut ? pixlut->pixels : NULL;
    OOP_Object              *gc = msg->gc;
   
    bm->usecount++;

    if ((ptr != (ULONG*)0xffffffff) && bm->fbgfx)
    {
	ULONG *cpuptr = (ULONG*)((IPTR)ptr + sd->Card.FrameBuffer);
    
	LOCK_HW

	NVSetRopSolid(sd, GC_DRMD(gc), ~0 << bm->depth);
	
	if ((bm->surface_format != sd->surface_format))
	{
	    NVDmaStart(&sd->Card, SURFACE_FORMAT, 1);
	    NVDmaNext(&sd->Card, bm->surface_format);
	    sd->surface_format = bm->surface_format;
	    D(bug("[NVidia] surface_format <- %d\n", sd->surface_format));
	}

	if (bm->pitch != sd->dst_pitch)
	{
	    NVDmaStart(&sd->Card, SURFACE_PITCH, 1);
	    NVDmaNext(&sd->Card, (bm->pitch << 16) | ((msg->width * bm->bpp + 63) & ~63));
	    sd->dst_pitch = bm->pitch;
	    sd->src_pitch = (msg->width * bm->bpp + 63) & ~63;
	    D(bug("[NVidia] pitch <- %08x\n", (sd->dst_pitch << 16) | sd->src_pitch));
	}

	if (bm->framebuffer != sd->dst_offset)
	{
	    NVDmaStart(&sd->Card, SURFACE_OFFSET_DST, 1);
	    NVDmaNext(&sd->Card, bm->framebuffer);
	    sd->dst_offset = bm->framebuffer;
	    D(bug("[NVidia] dst_offset=%p\n", sd->dst_offset));
	}

	NVDmaStart(&sd->Card, SURFACE_OFFSET_SRC, 1);
	NVDmaNext(&sd->Card, (IPTR)ptr);
	sd->src_offset = (IPTR)ptr;
	
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
}

static VOID bm__blitcolexp(OOP_Class *cl, OOP_Object *o, 
		struct pHidd_BitMap_BlitColorExpansion *msg)
{
#if 0
    nvBitMap	*bm = OOP_INST_DATA(cl, o);
    HIDDT_Pixel	fg,bg;
    ULONG	cemd;
    LONG	x,y;
    ULONG	mod;*/
    ULONG	bpp;
    
    ULONG	bpl;
    ULONG	depth,w,h;
    OOP_Object	*pix;

    OOP_GetAttr(msg->srcBitMap, aHidd_BitMap_Width, (APTR)&w);
    OOP_GetAttr(msg->srcBitMap, aHidd_BitMap_Height, (APTR)&h);
    OOP_GetAttr(msg->srcBitMap, aHidd_BitMap_PixFmt, (APTR)&pix);
    OOP_GetAttr(pix, aHidd_PixFmt_Depth, &depth);
    OOP_GetAttr(pix, aHidd_PixFmt_StdPixFmt, &bpp);
    
   
//    bug("Bitmap w=%d h=%d, depth=%d, type=%d\n",
//	w,h,depth, bpp);
#endif
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

}

static VOID bm__drawrect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    OOP_Object *gc = msg->gc;
    nvBitMap *bm = OOP_INST_DATA(cl, o);
    UWORD addX, addY;

    D(bug("[NVBitmap] DrawRect(%p, %d, %d, %d, %d)\n",
	bm->framebuffer, msg->minX, msg->minY, msg->maxX, msg->maxY));

    bm->usecount++;

    if (msg->minX == msg->maxX) addX = 1; else addX = 0;
    if (msg->minY == msg->maxY) addY = 1; else addY = 0;

    if ((GC_LINEPAT(gc) != (UWORD)~0) || !bm->fbgfx)
    {
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
    else
    {
	LOCK_HW

        sd->Card.DMAKickoffCallback = NVDMAKickoffCallback;

	if (sd->dst_offset != bm->framebuffer)
	{
	    NVDmaStart(&sd->Card, SURFACE_OFFSET_DST, 1);
	    NVDmaNext(&sd->Card, bm->framebuffer);
	    sd->dst_offset = bm->framebuffer;
	    D(bug("[NVBitMap] dst_offset <- %p\n", sd->dst_offset));
	}

	if (GC_DOCLIP(gc))
	{
	    NVDmaStart(&sd->Card, CLIP_POINT, 2);
	    NVDmaNext(&sd->Card, (GC_CLIPY1(gc) << 16) | GC_CLIPX1(gc));
	    NVDmaNext(&sd->Card, ((GC_CLIPY2(gc)-GC_CLIPY1(gc)+1) << 16) |
				  (GC_CLIPX2(gc)-GC_CLIPX1(gc)+1));
	}
	
	NVSetRopSolid(sd, GC_DRMD(gc), ~0);
	NVDmaStart(&sd->Card, LINE_COLOR, 1);
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
	    NVDmaNext(&sd->Card, 0xffffffff);
	}

	NVSync(sd);

	UNLOCK_HW
    }
}

static VOID bm__drawpoly(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawPolygon *msg)
{
    OOP_Object *gc = msg->gc;
    nvBitMap *bm = OOP_INST_DATA(cl, o);
    ULONG i;

    if ((GC_LINEPAT(gc) != (UWORD)~0) || !bm->fbgfx)
    {
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
    else
    {
	LOCK_HW

        sd->Card.DMAKickoffCallback = NVDMAKickoffCallback;

	if (sd->dst_offset != bm->framebuffer)
	{
	    NVDmaStart(&sd->Card, SURFACE_OFFSET_DST, 1);
	    NVDmaNext(&sd->Card, bm->framebuffer);
	    sd->dst_offset = bm->framebuffer;
	    D(bug("[NVBitMap] dst_offset <- %p\n", sd->dst_offset));
	}

	if (GC_DOCLIP(gc))
	{
	    NVDmaStart(&sd->Card, CLIP_POINT, 2);
	    NVDmaNext(&sd->Card, (GC_CLIPY1(gc) << 16) | GC_CLIPX1(gc));
	    NVDmaNext(&sd->Card, ((GC_CLIPY2(gc)-GC_CLIPY1(gc)+1) << 16) |
				  (GC_CLIPX2(gc)-GC_CLIPX1(gc)+1));
	}
	
	NVSetRopSolid(sd, GC_DRMD(gc), ~0);
	NVDmaStart(&sd->Card, LINE_COLOR, 1);
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
	    NVDmaNext(&sd->Card, 0xffffffff);
	}

	NVSync(sd);

	UNLOCK_HW
    }
}

#undef sd
/* Class related functions */

#define NUM_ROOT_METHODS    3
#define	NUM_BM_METHODS	    10

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
	{ OOP_METHODDEF(_bm__putimagelut),	moHidd_BitMap_PutImageLUT },
	{ OOP_METHODDEF(bm__blitcolexp),    moHidd_BitMap_BlitColorExpansion },
	{ OOP_METHODDEF(bm__drawrect),	    moHidd_BitMap_DrawRect },
	{ OOP_METHODDEF(bm__drawpoly),	    moHidd_BitMap_DrawPolygon },
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
#define	NUM_OFFBM_METHODS	10

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
	{ OOP_METHODDEF(_bm__putimagelut),	moHidd_BitMap_PutImageLUT },
	{ OOP_METHODDEF(bm__blitcolexp),    moHidd_BitMap_BlitColorExpansion },
	{ OOP_METHODDEF(bm__drawrect),	    moHidd_BitMap_DrawRect },
	{ OOP_METHODDEF(bm__drawpoly),	    moHidd_BitMap_DrawPolygon },
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

