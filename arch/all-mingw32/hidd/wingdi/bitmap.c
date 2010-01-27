/*
    Copyright  1995-2010, The AROS Development Team. All rights reserved.
    $Id: bitmap.c 26918 2007-10-02 02:55:49Z rob $

    Desc: Bitmap class for GDI hidd.
    Lang: English.
    
    Note: this implementation ignores GC_COLMASK. Windows GDI has no way to support it,
          however color masks seem to be not used anywhere in AROS.
*/

/****************************************************************************************/

#define __OOP_NOATTRBASES__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <proto/oop.h>
#include <proto/utility.h>
#include <exec/alerts.h>
#include <aros/macros.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <oop/oop.h>
#include <hidd/graphics.h>
#include <aros/symbolsets.h>

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

#include "gdi.h"

#include "bitmap.h"

/****************************************************************************************/

#define AO(x) 	    	  (aoHidd_BitMap_ ## x)
#define GOT_BM_ATTR(code) GOT_ATTR(code, aoHidd_BitMap, bitmap)

static OOP_AttrBase HiddBitMapAttrBase;
static OOP_AttrBase HiddPixFmtAttrBase;
static OOP_AttrBase HiddGDIBitMapAB;

static struct OOP_ABDescr attrbases[] = 
{
    { IID_Hidd_BitMap	, &HiddBitMapAttrBase 	},
    { IID_Hidd_PixFmt	, &HiddPixFmtAttrBase 	},
    /* Private bases */
    { IID_Hidd_GDIBitMap, &HiddGDIBitMapAB    	},
    { NULL  	    	, NULL      	    	}
};

/****************************************************************************************/

#define REFRESH(left, top, right, bottom)					  \
if (data->window) {								  \
    RECT r = {left, top, right, bottom};					  \
										  \
    USERCALL(RedrawWindow, data->window, &r, NULL, RDW_INVALIDATE|RDW_UPDATENOW); \
}										  \
Permit()

/****************************************************************************************/

VOID GDIBM__Hidd_BitMap__PutPixel(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutPixel *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    
    DB2(bug("[GDI] hidd.bitmap.gdibitmap::PutPixel(0x%p): (%lu, %lu) = 0x%08lX\n", o, msg->x, msg->y, msg->pixel));
    Forbid();
    GDICALL(SetROP2, data->dc, R2_COPYPEN);
    GDICALL(SetPixel, data->dc, msg->x, msg->y, msg->pixel);
    REFRESH(msg->x, msg->y, msg->x+1, msg->y+1);
    CHECK_STACK
}

/****************************************************************************************/

HIDDT_Pixel GDIBM__Hidd_BitMap__GetPixel(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetPixel *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    HIDDT_Pixel     	pixel;

    Forbid();
    pixel = GDICALL(GetPixel, data->dc, msg->x, msg->y);
    Permit();
    CHECK_STACK

    return pixel;
}

/****************************************************************************************/

static void FillRect(struct bitmap_data *data, ULONG col, ULONG mode, ULONG minX, ULONG minY, ULONG width, ULONG height)
{
    APTR br, orig_br;
    DB2(bug("[GDI] Brush color 0x%08lX, mode 0x%08lX\n", col, mode));

    Forbid();
    br = GDICALL(CreateSolidBrush, col);
    if (br) {
        orig_br = GDICALL(SelectObject, data->dc, br);
        GDICALL(PatBlt, data->dc, minX, minY, width, height, mode);
        GDICALL(SelectObject, data->dc, orig_br);
        GDICALL(DeleteObject, br);
    }
    /* Note the absence of Permit() because we are supposed to REFRESH() after drawing */
}

/* Table of raster operations (ROPs) corresponding to AROS GC drawmodes */
static ULONG Fill_DrawModeTable[] = {
    BLACKNESS,
    0x00A000C9,
    0x00500325,
    PATCOPY,
    0x000A0329,
    0x00AA0029,
    PATINVERT,
    0x00FA0089,
    0x000500A9,
    0x00A50065, /* PDnx - not sure */
    DSTINVERT,
    0x00F50225,
    0x000F0001,
    0x00AF0229,
    0x005F00E9,
    WHITENESS
};

VOID GDIBM__Hidd_BitMap__FillRect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    ULONG col = GC_FG(msg->gc);
    ULONG mode = Fill_DrawModeTable[GC_DRMD(msg->gc)];

    D(bug("[GDI] hidd.bitmap.gdibitmap::FillRect(0x%p, %d,%d,%d,%d)\n", o, msg->minX, msg->minY, msg->maxX, msg->maxY));
    FillRect(data, col, mode, msg->minX, msg->minY, msg->maxX - msg->minX + 1, msg->maxY - msg->minY + 1);
    REFRESH(msg->minX, msg->minY, msg->maxX + 1, msg->maxY + 1);
    CHECK_STACK
}

/****************************************************************************************/

/* Raster operations for drawing primitives */
static ULONG R2_DrawModeTable[] = {
    R2_BLACK,
    R2_MASKPEN,     /* bitmap AND pen	    */
    R2_MASKPENNOT,  /* NOT bitmap AND pen   */
    R2_COPYPEN,     /* pen		    */
    R2_MASKNOTPEN,  /* bitmap AND NOT pen   */
    R2_NOP,	    /* bitmap		    */
    R2_XORPEN,	    /* bitmap XOR pen	    */
    R2_MERGEPEN,    /* bitmap OR pen	    */
    R2_NOTMERGEPEN, /* NOT (bitmap OR pen)  */
    R2_NOTXORPEN,   /* NOT (bitmap XOR pen) */
    R2_NOT,	    /* NOT bitmap	    */
    R2_MERGEPENNOT, /* NOT bitmap OR pen    */
    R2_NOTCOPYPEN,  /* NOT pen		    */
    R2_MERGENOTPEN, /* NOT pen OR bitmap    */
    R2_NOTMASKPEN,  /* NOT (pen AND bitmap) */
    R2_WHITE
};

ULONG GDIBM__Hidd_BitMap__DrawPixel(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawPixel *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    ULONG col, mode;

    DB2(bug("[GDI] hidd.bitmap.gdibitmap::DrawPixel(0x%p): (%lu, %lu)\n", o, msg->x, msg->y));    
    col = GC_FG(msg->gc);
    mode = R2_DrawModeTable[GC_DRMD(msg->gc)];
    
    Forbid();
    GDICALL(SetROP2, data->dc, mode);
    GDICALL(SetPixel, data->dc, msg->x, msg->y, col);
    REFRESH(msg->x, msg->y, msg->x+1, msg->y+1);
    CHECK_STACK
    return 0;    
}

/****************************************************************************************/
	
VOID GDIBM__Hidd_BitMap__PutImage(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImage *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    HIDDT_PixelFormat *src_pixfmt, *dst_pixfmt;
    OOP_Object *gfxhidd;
    HIDDT_StdPixFmt src_stdpf;
    APTR buf, src, dst;
    ULONG bufmod, bufsize;
    ULONG y;
    BITMAPINFOHEADER bitmapinfo = {
        sizeof(BITMAPINFOHEADER),
        0, 0,
        1,
        32,
        BI_RGB,
        0, 0, 0,  0, 0
    };

/*  EnterFunc(bug("GDIGfx.BitMap::PutImage(pa=%p, x=%d, y=%d, w=%d, h=%d)\n",
    	    	  msg->pixels, msg->x, msg->y, msg->width, msg->height));*/
    
    switch(msg->pixFmt) {
    case vHidd_StdPixFmt_Native:
        /* TODO: What if we are on < 32 bit ? */
    case vHidd_StdPixFmt_Native32:
        src_stdpf = vHidd_StdPixFmt_RGB032;
    	break;
    default:
        src_stdpf = msg->pixFmt;
    }

    bufmod = msg->width * sizeof(HIDDT_Pixel);
    bufsize = bufmod * msg->height;
    buf = AllocMem(bufsize, MEMF_ANY);
    if (buf) {
        OOP_GetAttr(o, aHidd_BitMap_GfxHidd, (IPTR *)&gfxhidd);
        src_pixfmt = HIDD_Gfx_GetPixFmt(gfxhidd, src_stdpf);
        /* DIB pixels are expected to be 0x00RRGGBB (vHidd_StdPixFmt_BGR032) */
        dst_pixfmt = HIDD_Gfx_GetPixFmt(gfxhidd, vHidd_StdPixFmt_BGR032);
        src = msg->pixels;
        dst = buf;
        HIDD_BM_ConvertPixels(o, &src, src_pixfmt, msg->modulo, &dst, dst_pixfmt, bufmod,
			      msg->width, msg->height, NULL);
    	bitmapinfo.biWidth = msg->width;
    	bitmapinfo.biHeight = -msg->height; /* Minus here means top-down bitmap */
    	Forbid();
        GDICALL(StretchDIBits, data->dc, msg->x, msg->y, msg->width, msg->height, 0, 0, msg->width, msg->height, buf, &bitmapinfo, DIB_RGB_COLORS, SRCCOPY);
        REFRESH(msg->x, msg->y, msg->x + msg->width, msg->y + msg->height);
        FreeMem(buf, bufsize);
    }
    CHECK_STACK
}

/****************************************************************************************/

/* TODO: These little stubs help to detect methods calls */

VOID GDIBM__Hidd_BitMap__PutImageLUT(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImageLUT *msg)
{
    EnterFunc(bug("GDIGfx.BitMap::PutImageLUT(pa=%p, x=%d, y=%d, w=%d, h=%d)\n",
    	    	  msg->pixels, msg->x, msg->y, msg->width, msg->height));
	
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    CHECK_STACK
}

VOID GDIBM__Hidd_BitMap__GetImageLUT(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImageLUT *msg)
{
    EnterFunc(bug("GDIGfx.BitMap::GetImageLUT(pa=%p, x=%d, y=%d, w=%d, h=%d)\n",
    	    	  msg->pixels, msg->x, msg->y, msg->width, msg->height));
	
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    CHECK_STACK
}

/****************************************************************************************/

VOID GDIBM__Hidd_BitMap__GetImage(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImage *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    HIDDT_PixelFormat *src_pixfmt, *dst_pixfmt;
    OOP_Object *gfxhidd;
    HIDDT_StdPixFmt dst_stdpf;
    APTR tmp_dc, tmp_bitmap, dc_bitmap;
    APTR buf, src, dst;
    ULONG bufmod, bufsize;
    ULONG y;
    BITMAPINFOHEADER bitmapinfo = {
        sizeof(BITMAPINFOHEADER),
        0, 0,
        1,
        32,
        BI_RGB,
        0, 0, 0,  0, 0
    };

/*  EnterFunc(bug("GDIGfx.BitMap::GetImage(pa=%p, x=%d, y=%d, w=%d, h=%d)\n",
    	    	  msg->pixels, msg->x, msg->y, msg->width, msg->height));*/

    switch(msg->pixFmt) {
    case vHidd_StdPixFmt_Native:
        /* TODO: What if we are on < 32 bit ? */
    case vHidd_StdPixFmt_Native32:
        dst_stdpf = vHidd_StdPixFmt_RGB032;
    	break;
    default:
        dst_stdpf = msg->pixFmt;
    }

    bufmod = msg->width * sizeof(HIDDT_Pixel);
    bufsize = bufmod * msg->height;
    buf = AllocMem(bufsize, MEMF_ANY);
    if (buf) {
        /* First we have to extract requested rectangle into temporary bitmap because GetDIBits() can limit only scan lines number */
    	Forbid();
    	tmp_dc = GDICALL(CreateCompatibleDC, data->display);
    	if (tmp_dc) {
            tmp_bitmap = GDICALL(CreateCompatibleBitmap, data->display, msg->width, msg->height);
            if (tmp_bitmap) {
            	dc_bitmap = GDICALL(SelectObject, tmp_dc, tmp_bitmap);
            	if (dc_bitmap) {
                    GDICALL(BitBlt, tmp_dc, 0, 0, msg->width, msg->height, data->dc, msg->x, msg->y, SRCCOPY);
                    bitmapinfo.biWidth = msg->width;
    		    bitmapinfo.biHeight = -msg->height; /* Minus here means top-down bitmap */
        	    GDICALL(GetDIBits, tmp_dc, tmp_bitmap, 0, msg->height, buf, &bitmapinfo, DIB_RGB_COLORS);
        	    GDICALL(SelectObject, tmp_dc, dc_bitmap);
            	}
            	GDICALL(DeleteObject, tmp_bitmap);
            }
            GDICALL(DeleteDC, tmp_dc);
    	}
    	Permit();
        OOP_GetAttr(o, aHidd_BitMap_GfxHidd, (IPTR *)&gfxhidd);
	/* DIB pixels will be 0x00RRGGBB (vHidd_StdPixFmt_BGR032) */        
        src_pixfmt = HIDD_Gfx_GetPixFmt(gfxhidd, vHidd_StdPixFmt_BGR032);
        dst_pixfmt = HIDD_Gfx_GetPixFmt(gfxhidd, dst_stdpf);
        dst = msg->pixels;
        src = buf;
        HIDD_BM_ConvertPixels(o, &src, src_pixfmt, bufmod, &dst, dst_pixfmt, msg->modulo,
			      msg->width, msg->height, NULL);	
    	FreeMem(buf, bufsize);
    }
    CHECK_STACK
}

/****************************************************************************************/

/* Raster operations for a background */
static ULONG Copy_DrawModeTable[] = {
    BLACKNESS,
    MERGECOPY,  /* PSa  - src AND brush       */
    0x0030032A, /* PSna - NOT src AND brush   */
    PATCOPY,	/* P    - brush		      */
    0x000C0324, /* SPna - src AND NOT brush   */
    SRCCOPY,    /* S    - src		      */
    0x003C004A, /* PSx  - brush XOR src       */
    0x00FC008A, /* PSo  - brush OR src        */
    0x000300AA, /* PSon - NOT (brush OR src)  */
    0x00C3006A, /* PSxn - NOT (brush XOR src) */
    NOTSRCCOPY, /* Sn   - NOT src	      */
    0x00F3022A, /* PSno - NOT src OR brush    */
    0x000F0001, /* Pn   - NOT brush	      */
    0x00CF0224, /* SPno - NOT brush OR src    */
    0x003F00EA, /* PSan - NOT (brush AND src) */
    WHITENESS
};

/* Raster operations for a background */
static ULONG MaskedFill_DrawModeTable[] = {
    BLACKNESS,
    0x008003E9, /* PSDaa  - dest AND brush AND src       */
    0x00400F0A, /* PSDnaa - NOT dest AND brush AND src   */
    MERGECOPY,  /* PSa    - brush AND src	         */
    0x00080F08, /* SDPnaa - NOT brush AND dest AND src   */
    SRCAND,	/* DSa    - dest AND src	         */
    0x00480368, /* SDPxa  - brush XOR dest AND src       */
    0x00C803A8, /* SDPoa  - brush OR dest AND src        */
    0x00040C88, /* SDPona - NOT (brush OR dest) AND src  */
    0x00840C48, /* SDPxna - NOT (brush XOR dest) AND src */
    SRCERASE,   /* SDna   - NOT dest AND src	         */
    0x00C40E04, /* SPDnoa - NOT dest OR brush AND src    */
    0x000C0324, /* SPna   - NOT brush AND src		 */
    0x008C0E08, /* SDPnoa - NOT brush OR dest AND src	 */
    0x004C0CC8, /* SDPana - NOT (brush AND dest) AND src */
    SRCCOPY	/* WHITENESS AND src			 */
};

VOID GDIBM__Hidd_BitMap__BlitColorExpansion(OOP_Class *cl, OOP_Object *o,
					    struct pHidd_BitMap_BlitColorExpansion *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    HIDDT_Pixel fg, bg;
    ULONG drmd, cemd;
    APTR d = NULL;
    APTR buf_dc, buf_bm, buf_dc_bm;
    APTR mask_dc, mask_bm, mask_dc_bm;
    APTR br, dc_br;
    
/*  EnterFunc(bug("GDIGfx.BitMap::BlitColorExpansion(%p, %d, %d, %d, %d, %d, %d)\n",
    	    	  msg->srcBitMap, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height));*/

    OOP_GetAttr(msg->srcBitMap, aHidd_GDIBitMap_DeviceContext, (IPTR *)&d);
/*  D(bug("BlitColorExpansion(): Source DC: 0x%p\n", d));*/
    
    if (!d)
    {
    	/* We know nothing about the source bitmap. Let the superclass handle this */
    	/* TODO: accelerate this also, generate a bit mask from superclass' bitmap */
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	return;
    }
    
    fg = GC_FG(msg->gc);
    bg = GC_BG(msg->gc);
    drmd = GC_DRMD(msg->gc);
    cemd = GC_COLEXP(msg->gc);

    Forbid();
    /* First we create a buffer for foreground pixels */
    buf_dc = GDICALL(CreateCompatibleDC, data->display);
    if (buf_dc) {
        buf_bm = GDICALL(CreateCompatibleBitmap, data->display, msg->width, msg->height);
        if (buf_bm) {
            buf_dc_bm = GDICALL(SelectObject, buf_dc, buf_bm);
            if (buf_dc_bm) {    
		    /* Then we convert a source bitmap to 1-plane mask. We do it by creating a monochrome bitmap and copying our mask to it. */
		    mask_dc = GDICALL(CreateCompatibleDC, data->display);
		    if (mask_dc) {
			/* The bitmap is compatible with memory DC, not display DC. This is what gives us 1-plane bitmap */
			mask_bm = GDICALL(CreateCompatibleBitmap, mask_dc, msg->width, msg->height);
			if (mask_bm) {
			    mask_dc_bm = GDICALL(SelectObject, mask_dc, mask_bm);
			    if (mask_dc_bm) {
				GDICALL(SetBkColor, d, 0);
				/* During this first blit, pixels equal to BkColor, become WHITE. Others become BLACK. This converts
				   our truecolor display-compatible bitmap to a monochrome bitmap. A monochrome bitmap can be effectively
				   used for masking in blit operations. AND operations with WHITE will leave pixels intact, AND with BLACK
				   gives black. OR with black also leaves intact. */
				GDICALL(BitBlt, mask_dc, 0, 0, msg->width, msg->height, d, msg->srcX, msg->srcY, SRCCOPY);
				/* Now we are ready to do the actual painting. We will separately create foreground image, background image,
				   and then merge them.
				   The first thing is to paint foreground pixels with foreground brush according to draw mode and store them in buffer. */
				br = GDICALL(CreateSolidBrush, fg);
				if (br) {
				    dc_br = GDICALL(SelectObject, buf_dc, br);
				    if (dc_br) {
				        /* First we apply foreground color and DrawMode to the whole rectangle. The result is stored in the buffer bitmap */
				        GDICALL(BitBlt, buf_dc, 0, 0, msg->width, msg->height, data->dc, msg->destX, msg->destY, Copy_DrawModeTable[drmd]);
					/* Second we mask out background pixels using our mask with DSna (dest AND NOT src) opcode. Buffer's background will be black then */
					GDICALL(BitBlt, buf_dc, 0, 0, msg->width, msg->height, mask_dc, 0, 0, 0x00220326);
					GDICALL(SelectObject, buf_dc, dc_br);
				    }
				    GDICALL(DeleteObject, br);
				}
				/* Then we prepare a background */
				if (cemd & vHidd_GC_ColExp_Opaque) {
				    /* In opaque mode we can paint a background in place. We do it by making a solid brush of background color
				       and then blitting mask with "dest = source AND (dest DRMD brush)" operation. Remember that source
				       here is monochrome version of our mask. After this operation we get a background with black holes
				       into which we will blit foregroing pixels later */
				    br = GDICALL(CreateSolidBrush, bg);
				    if (br) {
					dc_br = GDICALL(SelectObject, data->dc, br);
					if (dc_br) {
					    GDICALL(BitBlt, data->dc, msg->destX, msg->destY, msg->width, msg->height, mask_dc, 0, 0, MaskedFill_DrawModeTable[drmd]);
					    GDICALL(SelectObject, data->dc, dc_br);
					}
					GDICALL(DeleteObject, br);
				    }
				} else
				    /* In transparent mode we simply clear masked area using "dest = dest AND src" operation */
				    GDICALL(BitBlt, data->dc, msg->destX, msg->destY, msg->width, msg->height, mask_dc, 0, 0, SRCAND);
				/* And at last we merge our buffer with the prepared destination bitmap using OR operation. Remember that the
				   destination now has completed background but black holes instead of foreground */
				GDICALL(BitBlt, data->dc, msg->destX, msg->destY, msg->width, msg->height, buf_dc, 0, 0, SRCPAINT);
				GDICALL(SelectObject, mask_dc, mask_dc_bm);
			    }
			    GDICALL(DeleteObject, mask_bm);
			}
			GDICALL(DeleteDC, mask_dc);
		    }
		    GDICALL(SelectObject, buf_dc, buf_dc_bm);
	    }
	    GDICALL(DeleteObject, buf_bm);
	}
	GDICALL(DeleteDC, buf_dc);
    }
    REFRESH(msg->destX, msg->destY, msg->destX + msg->width, msg->destY + msg->height);
    CHECK_STACK
}

/****************************************************************************************/

VOID GDIBM__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    ULONG   	    	idx;
    
    if (IS_GDIBM_ATTR(msg->attrID, idx))
    {
	switch (idx)
	{
	    case aoHidd_GDIBitMap_DeviceContext:
	    	*msg->storage = (IPTR)data->dc;
		break;
	    case aoHidd_GDIBitMap_Window:
	        *msg->storage = (IPTR)data->window;
		break;
	    case aoHidd_GDIBitMap_DisplayWidth:
		*msg->storage = (IPTR)data->win_width;
		break;
	    case aoHidd_GDIBitMap_DisplayHeight:
		*msg->storage = (IPTR)data->win_height;
		break;
	    default:
	    	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
		break;
	}
    }
    else
    	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

VOID GDIBM__Root__Set(OOP_Class *cl, OOP_Object *obj, struct pRoot_Set *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, obj);
    struct TagItem  *tag, *tstate;
    ULONG   	    idx;

    tstate = msg->attrList;
    while((tag = NextTagItem((const struct TagItem **)&tstate)))
    {
        if(IS_GDIBM_ATTR(tag->ti_Tag, idx))
        {
            switch(idx)
            {
                case aoHidd_GDIBitMap_Window:
		    data->window = (APTR)tag->ti_Data;
		    break;
	    }
	}
    }
}

/****************************************************************************************/

OOP_Object *GDIBM__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    OOP_Object  *friend = NULL, *pixfmt;
/*  APTR 	 friend_drawable = NULL;*/
    APTR	 display, my_dc, my_bitmap, orig_bitmap;
    ULONG   	 width, height;
    ULONG	 win_width, win_height;
    IPTR	 depth;
    IPTR    	 attrs[num_Hidd_BitMap_Attrs];
    int     	 screen;
    BOOL    	 ok = TRUE;
    struct bitmap_data *data;
    
    DECLARE_ATTRCHECK(bitmap);

    EnterFunc(bug("GDIBM::New()\n"));    
    /* Parse the attributes */
    if (0 != OOP_ParseAttrs(msg->attrList, attrs, num_Hidd_BitMap_Attrs,
    	    	    	    &ATTRCHECK(bitmap), HiddBitMapAttrBase))
    {
    	D(kprintf("!!! GDIGfx::BitMap() FAILED TO PARSE ATTRS !!!\n"));
	
	return NULL;
    }
    
    if (GOT_BM_ATTR(Friend))
    	friend = (OOP_Object *)attrs[AO(Friend)];
    else 
    	friend = NULL;
	
    width  = attrs[AO(Width)];
    height = attrs[AO(Height)];
    pixfmt = (OOP_Object *)attrs[AO(PixFmt)];

    OOP_GetAttr(pixfmt, aHidd_PixFmt_Depth, &depth);

    /* Get the device context from the friend bitmap */
/*  if (NULL != friend)
    {
	OOP_GetAttr(friend, aHidd_GDIBitMap_Drawable, (IPTR *)&friend_drawable);
    }*/

    display = (APTR)GetTagData(aHidd_GDIBitMap_SysDisplay, 0, msg->attrList);
    win_width = GetTagData(aHidd_GDIBitMap_DisplayWidth, 0, msg->attrList);
    win_height = GetTagData(aHidd_GDIBitMap_DisplayHeight, 0, msg->attrList);

    D(bug("Creating GDI bitmap: %ldx%ldx%ld\n", width, height, depth));

    Forbid();
    my_dc = GDICALL(CreateCompatibleDC, display);
    D(bug("[GDI] Memory device context: 0x%p\n", my_dc));
    if (my_dc) {
        my_bitmap = GDICALL(CreateCompatibleBitmap, display, width, height);
        D(bug("[GDI] Memory bitmap: 0x%p\n", my_bitmap));
        if (my_bitmap)
            orig_bitmap = GDICALL(SelectObject, my_dc, my_bitmap);
        D(bug("[GDI] Olriginal DC bitmap: 0x%p\n", orig_bitmap));
    }
    Permit();

    if (!my_dc)
    	return NULL;
    if (!orig_bitmap)
        goto dispose_bitmap;
    
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    D(bug("[GDI] Object created by superclass: 0x%p\n", o));
    if (o) {
        data = OOP_INST_DATA(cl, o);
        /* clear all data  */
        memset(data, 0, sizeof(struct bitmap_data));
	/* Get some info passed to us by the gdigfxhidd class */
	data->display = display;
	data->dc = my_dc;
	data->bitmap = my_bitmap;
	data->dc_bitmap = orig_bitmap;
	data->win_width = win_width;
	data->win_height = win_height;
	CHECK_STACK
    	ReturnPtr("GDIGfx.BitMap::New()", OOP_Object *, o);
    } /* if (object allocated by superclass) */
dispose_bitmap:    
    Forbid();
    if (orig_bitmap)
    	GDICALL(SelectObject, my_dc, orig_bitmap);
    if (my_bitmap)
        GDICALL(DeleteObject, my_bitmap);
    GDICALL(DeleteDC, my_dc);
    Permit();
    
    ReturnPtr("GDIGfx.BitMap::New()", OOP_Object *, NULL);
    
}

/****************************************************************************************/

VOID GDIBM__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);

    EnterFunc(bug("GDIGfx.BitMap::Dispose()\n"));
    
    Forbid();
    if (data->dc_bitmap)
    	GDICALL(SelectObject, data->dc, data->dc_bitmap);
    if (data->bitmap)
        GDICALL(DeleteObject, data->bitmap);
    if (data->dc)
        GDICALL(DeleteDC, data->dc);
    Permit();
    
    OOP_DoSuperMethod(cl, o, msg);

    CHECK_STACK    
    ReturnVoid("GDIGfx.BitMap::Dispose");
}

/****************************************************************************************/

VOID GDIBM__Hidd_BitMap__Clear(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_Clear *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    IPTR width, height;
    
    EnterFunc(bug("[GDI] hidd.bitmap.gdibitmap::Clear()\n"));
    
    /* Get width & height from bitmap superclass */
  
    OOP_GetAttr(o, aHidd_BitMap_Width,  &width);
    OOP_GetAttr(o, aHidd_BitMap_Height, &height);
    
    FillRect(data, GC_BG(msg->gc), PATCOPY, 0, 0, width, height);
    REFRESH(0, 0, width, height);
}

/****************************************************************************************/

static int GDIBM_Init(LIBBASETYPEPTR LIBBASE)
{
    return OOP_ObtainAttrBases(attrbases);
}

/****************************************************************************************/

static int GDIBM_Expunge(LIBBASETYPEPTR LIBBASE)
{
    OOP_ReleaseAttrBases(attrbases);
    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(GDIBM_Init, 0);
ADD2EXPUNGELIB(GDIBM_Expunge, 0);
