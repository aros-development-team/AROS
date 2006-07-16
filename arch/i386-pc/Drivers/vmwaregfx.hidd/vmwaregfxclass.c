/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Class for VMWare.
    Lang: English.
*/

#define __OOP_NOATTRBASES__

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <aros/symbolsets.h>
#include <devices/inputevent.h>
#include <exec/alerts.h>
#include <exec/memory.h>
#include <hardware/custom.h>
#include <hidd/hidd.h>
#include <hidd/graphics.h>
#include <oop/oop.h>

#define DEBUG 0
#include <aros/debug.h>

#include "vmwaregfxclass.h"
#include "bitmap.h"
#include "hardware.h"

#include LC_LIBDEFS_FILE

static OOP_AttrBase HiddBitMapAttrBase;  
static OOP_AttrBase HiddPixFmtAttrBase;
static OOP_AttrBase HiddGfxAttrBase;
static OOP_AttrBase HiddSyncAttrBase;
static OOP_AttrBase HiddVMWareGfxAttrBase;
static OOP_AttrBase HiddVMWareGfxBitMapAttrBase;

static struct OOP_ABDescr attrbases[] =
{
    {IID_Hidd_BitMap,          &HiddBitMapAttrBase},
    {IID_Hidd_VMWareGfxBitMap, &HiddVMWareGfxBitMapAttrBase},
    {IID_Hidd_VMWareGfx,       &HiddVMWareGfxAttrBase},
    {IID_Hidd_PixFmt,          &HiddPixFmtAttrBase},
    {IID_Hidd_Sync,            &HiddSyncAttrBase},
    {IID_Hidd_Gfx,             &HiddGfxAttrBase},
    {NULL, NULL}
};

STATIC ULONG mask_to_shift(ULONG mask)
{
    ULONG i;
    
    for (i = 32; mask; i --) {
	mask >>= 1;
    }
	
    if (mask == 32) {
   	i = 0;
    }
	
    return i;
}

OOP_Object *VMWareGFX__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg) {
struct TagItem pftags[] =
{
	{aHidd_PixFmt_RedShift,     0}, /*  0 */
	{aHidd_PixFmt_GreenShift,   0}, /*  1 */
	{aHidd_PixFmt_BlueShift,    0}, /*  2 */
	{aHidd_PixFmt_AlphaShift,   0}, /*  3 */
	{aHidd_PixFmt_RedMask,      0}, /*  4 */
	{aHidd_PixFmt_GreenMask,    0}, /*  5 */
	{aHidd_PixFmt_BlueMask,     0}, /*  6 */
	{aHidd_PixFmt_AlphaMask,    0}, /*  7 */
	{aHidd_PixFmt_ColorModel,   0}, /*  8 */
	{aHidd_PixFmt_Depth,        0}, /*  9 */
	{aHidd_PixFmt_BytesPerPixel,0}, /* 10 */
	{aHidd_PixFmt_BitsPerPixel, 0}, /* 11 */
	{aHidd_PixFmt_StdPixFmt,    0}, /* 12 */
	{aHidd_PixFmt_CLUTShift,    0}, /* 13 */
	{aHidd_PixFmt_CLUTMask,     0x0f}, /* 14 */
	{aHidd_PixFmt_BitMapType,   0}, /* 15 */
	{TAG_DONE, 0UL }
};
struct TagItem sync_mode1[] =
{
	{aHidd_Sync_PixelClock, 0},
	{aHidd_Sync_HDisp,    640},
	{aHidd_Sync_VDisp,    480},
	{aHidd_Sync_HSyncStart, 0},
	{aHidd_Sync_HSyncEnd,   0},
	{aHidd_Sync_HTotal,     0},
	{aHidd_Sync_VSyncStart, 0},
	{aHidd_Sync_VSyncEnd,   0},
	{aHidd_Sync_VTotal,     0},
	{TAG_DONE, 0UL}
};
struct TagItem sync_mode2[] =
{
	{aHidd_Sync_PixelClock, 0},
	{aHidd_Sync_HDisp,    800},
	{aHidd_Sync_VDisp,    600},
	{aHidd_Sync_HSyncStart, 0},
	{aHidd_Sync_HSyncEnd,   0},
	{aHidd_Sync_HTotal,     0},
	{aHidd_Sync_VSyncStart, 0},
	{aHidd_Sync_VSyncEnd,   0},
	{aHidd_Sync_VTotal,     0},
	{TAG_DONE, 0UL}
};
struct TagItem sync_mode3[] =
{
	{aHidd_Sync_PixelClock, 0},
	{aHidd_Sync_HDisp,    1024},
	{aHidd_Sync_VDisp,    768},
	{aHidd_Sync_HSyncStart, 0},
	{aHidd_Sync_HSyncEnd,   0},
	{aHidd_Sync_HTotal,     0},
	{aHidd_Sync_VSyncStart, 0},
	{aHidd_Sync_VSyncEnd,   0},
	{aHidd_Sync_VTotal,     0},
	{TAG_DONE, 0UL}
};
struct TagItem modetags[] =
{
	{aHidd_Gfx_PixFmtTags, (IPTR)pftags},
	{aHidd_Gfx_SyncTags,   (IPTR)sync_mode1},
	{aHidd_Gfx_SyncTags,   (IPTR)sync_mode2},
	{aHidd_Gfx_SyncTags,   (IPTR)sync_mode3},
	{TAG_DONE, 0UL}
};
struct TagItem yourtags[] =
{
	{aHidd_Gfx_ModeTags, (IPTR)modetags},
	{TAG_MORE, 0UL}
};
struct pRoot_New yourmsg;

	/* set pftags = 0 */
	if (!XSD(cl)->data.pseudocolor)
	{
		pftags[0].ti_Data = mask_to_shift(XSD(cl)->data.redmask);
		pftags[1].ti_Data = mask_to_shift(XSD(cl)->data.greenmask);
		pftags[2].ti_Data = mask_to_shift(XSD(cl)->data.bluemask);
	}
	else
	{
		pftags[0].ti_Data = 0;
		pftags[1].ti_Data = 0;
		pftags[2].ti_Data = 0;
	}
	pftags[3].ti_Data = 0;
	pftags[4].ti_Data = XSD(cl)->data.redmask;
	pftags[5].ti_Data = XSD(cl)->data.greenmask;
	pftags[6].ti_Data = XSD(cl)->data.bluemask;
	pftags[7].ti_Data = 0;
	D(bug("[VMWare] New: Masks red=%08x<<%d,green=%08x<<%d,blue%08x<<%d\n",
		    pftags[4].ti_Data, pftags[0].ti_Data,
		    pftags[5].ti_Data, pftags[1].ti_Data,
		    pftags[6].ti_Data, pftags[2].ti_Data));
	if (XSD(cl)->data.pseudocolor)
		pftags[8].ti_Data = vHidd_ColorModel_Palette;
	else
		pftags[8].ti_Data = vHidd_ColorModel_TrueColor;
	pftags[9].ti_Data = XSD(cl)->data.depth;
	pftags[10].ti_Data = XSD(cl)->data.bytesperpixel;
	pftags[11].ti_Data = XSD(cl)->data.bitsperpixel;
	pftags[12].ti_Data = vHidd_StdPixFmt_Native;
	pftags[15].ti_Data = vHidd_BitMapType_Chunky;
	yourtags[1].ti_Data = (IPTR)msg->attrList;
	yourmsg.mID = msg->mID;
	yourmsg.attrList = yourtags;
	msg = &yourmsg;
	EnterFunc(bug("VMWareGfx::New()\n"));
	o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	if (o)
	{
		D(bug("Got object from super\n"));
		XSD(cl)->vmwaregfxhidd = o;
		XSD(cl)->mouse.shape = NULL;
		ReturnPtr("VMWareGfx::New", OOP_Object *, o);
	}
	ReturnPtr("VMWareGfx::New", OOP_Object *, NULL);
}

VOID VMWareGFX__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg) {
	if (XSD(cl)->mouse.shape != NULL)
		FreeVec(XSD(cl)->mouse.shape);
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID VMWareGFX__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg) {
ULONG idx;
BOOL found = FALSE;

	if (IS_GFX_ATTR(msg->attrID, idx))
	{
		switch (idx)
		{
		case aoHidd_Gfx_SupportsHWCursor:
			*msg->storage = (IPTR)TRUE;
			found = TRUE;
			break;
		}
	}
	if (!found)
		OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

OOP_Object *VMWareGFX__Hidd_Gfx__NewBitMap(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_NewBitMap *msg) {
BOOL displayable;
BOOL framebuffer;
OOP_Class *classptr = NULL;
struct TagItem tags[2];
struct pHidd_Gfx_NewBitMap yourmsg;

	EnterFunc(bug("VMWareGfx::NewBitMap()\n"));
	displayable = GetTagData(aHidd_BitMap_Displayable, FALSE, msg->attrList);
	framebuffer = GetTagData(aHidd_BitMap_FrameBuffer, FALSE, msg->attrList);
	if (framebuffer)
		classptr = XSD(cl)->onbmclass;
	else if (displayable)
		classptr = XSD(cl)->offbmclass;
	else
	{
		HIDDT_ModeID modeid;
		modeid = (HIDDT_ModeID)GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);
		if (modeid != vHidd_ModeID_Invalid)
			classptr = XSD(cl)->offbmclass;
		else
		{
			HIDDT_StdPixFmt stdpf;
			stdpf = (HIDDT_StdPixFmt)GetTagData(aHidd_BitMap_StdPixFmt, vHidd_StdPixFmt_Unknown, msg->attrList);
			if (stdpf == vHidd_StdPixFmt_Unknown)
			{
				OOP_Object *friend;
				friend = (OOP_Object *)GetTagData(aHidd_BitMap_Friend, NULL, msg->attrList);
				if (friend != NULL)
				{
					OOP_Object *gfxhidd;
					OOP_GetAttr(friend, aHidd_BitMap_GfxHidd, (IPTR *)&gfxhidd);
					if (gfxhidd == o)
					{
						classptr = XSD(cl)->offbmclass;
					}
				}
			}
		}
	}
	if (classptr != NULL)
	{
		tags[0].ti_Tag = aHidd_BitMap_ClassPtr;
		tags[0].ti_Data = (IPTR)classptr;
		tags[1].ti_Tag = TAG_MORE;
		tags[1].ti_Data = (IPTR)msg->attrList;
		yourmsg.mID = msg->mID;
		yourmsg.attrList = tags;
		msg = &yourmsg;
	}
	ReturnPtr("VMWareGfx::NewBitMap", OOP_Object *, (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg));
}

VOID VMWareGFX__Hidd_Gfx__CopyBox(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CopyBox *msg) {
UBYTE *src = NULL;
UBYTE *dst = NULL;
HIDDT_DrawMode mode;
struct Box box;

	EnterFunc(bug("VMWareGfx.BitMap::CopyBox\n"));
	mode = GC_DRMD(msg->gc);
	OOP_GetAttr(msg->src, aHidd_VMWareGfxBitMap_Drawable, (IPTR *)&src);
	OOP_GetAttr(msg->dest, aHidd_VMWareGfxBitMap_Drawable, (IPTR *)&dst);
	if (((dst == NULL) || (src == NULL))) /* no vmwaregfx bitmap */
	{
		OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	}
	else if (dst == src)
	{
		struct BitmapData *data;
		data = OOP_INST_DATA(OOP_OCLASS(msg->src), msg->src);
		switch (mode)
		{
		case vHidd_GC_DrawMode_Clear:
			clearCopyVMWareGfx(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
			break;
		case vHidd_GC_DrawMode_And:
			andCopyVMWareGfx(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
			break;
		case vHidd_GC_DrawMode_AndReverse:
			andReverseCopyVMWareGfx(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
			break;
		case vHidd_GC_DrawMode_Copy:
			copyCopyVMWareGfx(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
			break;
		case vHidd_GC_DrawMode_AndInverted:
			andInvertedCopyVMWareGfx(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
			break;
		case vHidd_GC_DrawMode_NoOp:
			noOpCopyVMWareGfx(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
			break;
		case vHidd_GC_DrawMode_Xor:
			xorCopyVMWareGfx(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
			break;
		case vHidd_GC_DrawMode_Or:
			orCopyVMWareGfx(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
			break;
		case vHidd_GC_DrawMode_Nor:
			norCopyVMWareGfx(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
			break;
		case vHidd_GC_DrawMode_Equiv:
			equivCopyVMWareGfx(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
			break;
		case vHidd_GC_DrawMode_Invert:
			invertCopyVMWareGfx(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
			break;
		case vHidd_GC_DrawMode_OrReverse:
			orReverseCopyVMWareGfx(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
			break;
		case vHidd_GC_DrawMode_CopyInverted:
			copyInvertedCopyVMWareGfx(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
			break;
		case vHidd_GC_DrawMode_OrInverted:
			orInvertedCopyVMWareGfx(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
			break;
		case vHidd_GC_DrawMode_Nand:
			nandCopyVMWareGfx(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
			break;
		case vHidd_GC_DrawMode_Set:
			setCopyVMWareGfx(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
			break;
		default:
			OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
		}
	}
	else
	{
		struct BitmapData *srcbd = OOP_INST_DATA(OOP_OCLASS(msg->src), msg->src);
		struct BitmapData *dstbd = OOP_INST_DATA(OOP_OCLASS(msg->dest), msg->dest);
		UBYTE *sbuffer;
		ULONG srestadd;
		UBYTE *dbuffer;
		ULONG drestadd;
		ULONG ycnt = msg->height;
		ULONG xcnt;
		LONG offset;
		/* get src/dest video data start addresses and skip sizes */
		if (srcbd->VideoData == srcbd->data->vrambase)
		{
			offset = (msg->srcX*srcbd->bytesperpix)+(msg->srcY*srcbd->data->bytesperline);
			srestadd = (srcbd->data->bytesperline - (msg->width*srcbd->bytesperpix));
			displayCursorVMWareGfx(&XSD(cl)->data, 0);
			XSD(cl)->mouse.visible = 0;
		}
		else
		{
			offset = (msg->srcX+(msg->srcY*srcbd->width))*srcbd->bytesperpix;
			srestadd = (srcbd->width - msg->width)*srcbd->bytesperpix;
		}
		sbuffer = srcbd->VideoData+offset;
		if (dstbd->VideoData == dstbd->data->vrambase)
		{
			offset = (msg->destX*dstbd->bytesperpix)+(msg->destY*dstbd->data->bytesperline);
			drestadd = (dstbd->data->bytesperline - (msg->width*dstbd->bytesperpix));
			displayCursorVMWareGfx(&XSD(cl)->data, 0);
			XSD(cl)->mouse.visible = 0;
		}
		else
		{
			offset = (msg->destX+(msg->destY*dstbd->width))*dstbd->bytesperpix;
			drestadd = (dstbd->width - msg->width)*dstbd->bytesperpix;
		}
		dbuffer = dstbd->VideoData+offset;
		switch (mode)
		{
		case vHidd_GC_DrawMode_Copy:
			while (ycnt--)
			{
				ULONG pixel;
				xcnt = msg->width;
				while (xcnt)
				{
					/* get pixel from source */
				    	switch (srcbd->bytesperpix)
					{
					    case 1:
						pixel = (ULONG)*((UBYTE *)sbuffer);
						sbuffer++;
						break;
					    case 2:
						pixel = (ULONG)*((UWORD *)sbuffer);
						sbuffer += 2;
						break;
					    case 4:
						pixel = (ULONG)*((ULONG *)sbuffer);
						sbuffer += 4;
						break;
					    default:
						D(bug("[VMWare] Copy: Unknown number of bytes per pixel (%d) in source!\n",srcbd->bytesperpix));
						pixel = 0;
						break;
					}
					/* write pixel to destination */
					switch (dstbd->bytesperpix)
					{
					    case 1:
						*((UBYTE *)dbuffer) = (UBYTE)pixel;
						dbuffer++;
						break;
					    case 2:
						*((UWORD *)dbuffer) = (UWORD)pixel;
						dbuffer += 2;
						break;
					    case 4:	
						*((ULONG *)dbuffer) = (ULONG)pixel;
						dbuffer += 4;
						break;
					    default:
						D(bug("[VMWare] Copy: Unknown number of bytes per pixel (%d) in destination!\n",dstbd->bytesperpix));
						break;
					}
					xcnt--;
				}
				sbuffer += srestadd;
				dbuffer += drestadd;
			}
			if (dstbd->VideoData == dstbd->data->vrambase)
			{
				box.x1 = msg->destX;
				box.y1 = msg->destY;
				box.x2 = box.x1+msg->width-1;
				box.y2 = box.y1+msg->height-1;
				refreshAreaVMWareGfx(dstbd->data, &box);
			}
			break;
		default:
			kprintf("mode = %ld src=%lx dst=%lx\n", mode, src, dst);
			OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
		}
		if (XSD(cl)->mouse.visible == 0)
		{
			displayCursorVMWareGfx(&XSD(cl)->data, 1);
			XSD(cl)->mouse.visible = 1;
		}
	}
	ReturnVoid("VMWareGfx.BitMap::CopyBox");
}

BOOL VMWareGFX__Hidd_Gfx__SetCursorShape(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorShape *msg) {
struct VMWareGfx_staticdata *data = XSD(cl);

	if (msg->shape == NULL)
	{
		displayCursorVMWareGfx(&XSD(cl)->data, 0);
		data->mouse.oopshape = NULL;
		if (data->mouse.shape != NULL)
			FreeVec(data->mouse.shape);
		data->mouse.shape = NULL;
		return TRUE;
	}
	else
	{
		OOP_Object *pfmt;
		OOP_Object *colmap;
		HIDDT_StdPixFmt pixfmt;
		HIDDT_Color color;
		OOP_GetAttr(msg->shape, aHidd_BitMap_Width, &data->mouse.width);
		OOP_GetAttr(msg->shape, aHidd_BitMap_Height, &data->mouse.height);
		OOP_GetAttr(msg->shape, aHidd_BitMap_PixFmt, (IPTR *)&pfmt);
		OOP_GetAttr(pfmt, aHidd_PixFmt_StdPixFmt, &pixfmt);
		OOP_GetAttr(msg->shape, aHidd_BitMap_ColorMap, (IPTR *)&colmap);
		data->mouse.oopshape = msg->shape;
#if 0
		data->mouse.shape = cursor_shape;
		data->mouse.width = 11;
		data->mouse.height = 11;
		defineCursorVMWareGfx(&XSD(cl)->data, &data->mouse);
		return TRUE;
#else
		/* convert shape to vmware needs */
		if (data->mouse.shape != NULL)
			FreeVec(data->mouse.shape);
		data->mouse.shape = AllocVec(SVGA_PIXMAP_SIZE(data->mouse.width, data->mouse.height, data->data.bitsperpixel)*4, MEMF_PUBLIC);
		if (data->mouse.shape != NULL)
		{
			LONG xcnt;
			LONG ycnt;
			LONG linebytes;
			LONG bytecnt;
			LONG pixelbytes;
			UBYTE *shape;
			linebytes = SVGA_PIXMAP_SCANLINE_SIZE(data->mouse.width, data->data.bitsperpixel)*4;
			pixelbytes = (data->data.bitsperpixel+3)/8;
			shape = data->mouse.shape;
			for (ycnt=0;ycnt<data->mouse.height;ycnt++)
			{
				for (xcnt=0;xcnt<data->mouse.width;xcnt++)
				{
					HIDDT_Pixel pixel;
					pixel = HIDD_BM_GetPixel(msg->shape, xcnt, ycnt);
					if (pixfmt == vHidd_StdPixFmt_LUT8)
					{
						if (HIDD_CM_GetColor(colmap, pixel, &color))
						{
							pixel =
								(
									(((color.red  <<16)>>mask_to_shift(data->data.redmask))   & data->data.redmask  )+
									(((color.green<<16)>>mask_to_shift(data->data.greenmask)) & data->data.greenmask)+
									(((color.blue <<16)>>mask_to_shift(data->data.bluemask))  & data->data.bluemask )
								);
						}
					}
					if (pixelbytes == 1)
					{
						*((UBYTE *)shape) = (UBYTE)pixel;
						shape++;
					}
					else if (pixelbytes == 2)
					{
						*((UWORD *)shape) = (UWORD)pixel;
						shape += 2;
					}
					else if (pixelbytes == 4)
					{
						*((ULONG *)shape) = (ULONG)pixel;
						shape += 4;
					}
				}
				for (bytecnt=linebytes-(data->mouse.width*pixelbytes);bytecnt;bytecnt--)
				{
					*((UBYTE *)shape) = 0; /* fill up to long boundary */
					shape++;
				}
			}
			defineCursorVMWareGfx(&XSD(cl)->data, &data->mouse);
			return TRUE;
		}
#endif
	}
	return FALSE;
}

BOOL VMWareGFX__Hidd_Gfx__SetCursorPos(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorPos *msg) {
struct Box box;

	XSD(cl)->mouse.x = msg->x;
	XSD(cl)->mouse.y = msg->y;
	if (XSD(cl)->mouse.x<0)
		XSD(cl)->mouse.x=0;
	if (XSD(cl)->mouse.y<0)
		XSD(cl)->mouse.y=0;
#warning "check visible width/height"
	moveCursorVMWareGfx(&XSD(cl)->data, msg->x, msg->y);
	return TRUE;
}

VOID VMWareGFX__Hidd_Gfx__SetCursorVisible(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorVisible *msg) {

	XSD(cl)->mouse.visible = msg->visible;
	displayCursorVMWareGfx(&XSD(cl)->data, msg->visible ? 1 : 0);
}


AROS_SET_LIBFUNC(VMWareGfx_InitStatic, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    EnterFunc(bug("VMWareGfx_InitStatic\n"));

    LIBBASE->vsd.mouse.x=0;
    LIBBASE->vsd.mouse.y=0;
    LIBBASE->vsd.mouse.shape = NULL;

    if (!OOP_ObtainAttrBases(attrbases))
    {
	D(bug("VMWareGfx_InitStatic attrbases init failed\n"));
	return FALSE;
    }
    
    D(bug("VMWareGfx_InitStatic ok\n"));

    ReturnInt("VMWareGfx_InitStatic", UNLONG, TRUE);
    
    AROS_SET_LIBFUNC_EXIT
}

AROS_SET_LIBFUNC(VMWareGfx_ExpungeStatic, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    EnterFunc(bug("VMWareGfx_ExpungeStatic\n"));

    OOP_ReleaseAttrBases(attrbases);
    ReturnInt("VMWareGfx_ExpungeStatic", ULONG, TRUE);
    
    AROS_SET_LIBFUNC_EXIT
}

ADD2INITLIB(VMWareGfx_InitStatic, 0)
ADD2EXPUNGELIB(VMWareGfx_ExpungeStatic, 0)
