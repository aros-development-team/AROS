/*
    Copyright  1995-2009, The AROS Development Team. All rights reserved.
    $Id: bitmap.c 26918 2007-10-02 02:55:49Z rob $

    Desc: Bitmap class for GDI hidd.
    Lang: English.
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
#define DEBUG 1
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

#include "gdigfx_intern.h"
#include "gdi.h"

#include "bitmap.h"

/****************************************************************************************/

static OOP_AttrBase HiddBitMapAttrBase;
static OOP_AttrBase HiddPixFmtAttrBase;
static OOP_AttrBase HiddGDIGfxAB;
static OOP_AttrBase HiddGDIBitMapAB;

static struct OOP_ABDescr attrbases[] = 
{
    { IID_Hidd_BitMap	, &HiddBitMapAttrBase 	},
    { IID_Hidd_PixFmt	, &HiddPixFmtAttrBase 	},
    /* Private bases */
    { IID_Hidd_GDIGfx	, &HiddGDIGfxAB	    	},
    { IID_Hidd_GDIBitMap, &HiddGDIBitMapAB    	},
    { NULL  	    	, NULL      	    	}
};

/****************************************************************************************/

#define REFRESH(left, top, right, bottom)	   \
if (data->window) {				   \
    RECT r = {left, top, right, bottom};	   \
        					   \
    USERCALL(RedrawWindow, data->window, &r, NULL, RDW_INVALIDATE|RDW_UPDATENOW); \
}

BOOL GDIBM__Hidd_BitMap__SetColors(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_SetColors *msg)
{
    struct bitmap_data  *data = OOP_INST_DATA(cl, o);
    HIDDT_PixelFormat 	*pf;    
    ULONG   	    	 xc_i, col_i;

    pf = BM_PIXFMT(o);

    if (vHidd_ColorModel_StaticPalette == HIDD_PF_COLMODEL(pf) ||
    	vHidd_ColorModel_TrueColor == HIDD_PF_COLMODEL(pf) )
    {	 
	 /* Superclass takes care of this case */

	 return OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }

    /* Ve have a vHidd_GT_Palette bitmap */    

    if (!OOP_DoSuperMethod(cl, o, (OOP_Msg)msg)) return FALSE;
/*    
    if (data->flags & BMDF_COLORMAP_ALLOCED)
    {
    	LOCK_GDI	

	for ( xc_i = msg->firstColor, col_i = 0;
    	      col_i < msg->numColors; 
	      xc_i ++, col_i ++ )
	{
            XColor xcol;

	    xcol.red   = msg->colors[col_i].red;
	    xcol.green = msg->colors[col_i].green;
	    xcol.blue  = msg->colors[col_i].blue;
	    xcol.pad   = 0;
	    xcol.pixel = xc_i;
	    xcol.flags = DoRed | DoGreen | DoBlue;
	    
	    XCALL(XStoreColor, data->display, data->colmap, &xcol);

	}
	
    	UNLOCK_GDI	
	
    }*/ /* if (data->flags & BMDF_COLORMAP_ALLOCED) */

    return TRUE;
}

/****************************************************************************************/

VOID GDIBM__Hidd_BitMap__PutPixel(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutPixel *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    
    DB2(bug("[GDI] hidd.bitmap.gdibitmap::PutPixel(0x%p): (%lu, %lu) = 0x%08lX\n", o, msg->x, msg->y, msg->pixel));
    LOCK_GDI
    GDICALL(SetPixel, data->dc, msg->x, msg->y, msg->pixel);
    REFRESH(msg->x, msg->y, msg->x+1, msg->y+1)
    UNLOCK_GDI
}

/****************************************************************************************/

HIDDT_Pixel GDIBM__Hidd_BitMap__GetPixel(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetPixel *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    APTR dc;
    HIDDT_Pixel     	pixel;

    LOCK_GDI
    pixel = GDICALL(GetPixel, data->dc, msg->x, msg->y);
    UNLOCK_GDI

    return pixel;
}

/****************************************************************************************/

VOID GDIBM__Hidd_BitMap__FillRect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    APTR br, orig_br;
    ULONG col, mode;
    
    D(bug("[GDI] hidd.bitmap.gdibitmap::FillRect(0x%p, %d,%d,%d,%d)\n", o, msg->minX, msg->minY, msg->maxX, msg->maxY));
    
    switch (GC_DRMD(msg->gc)) {
    case vHidd_GC_DrawMode_Clear:
        col = GC_BG(msg->gc);
    	mode = BLACKNESS;
    	break;
    case vHidd_GC_DrawMode_Copy:
        col = GC_FG(msg->gc);
    	mode = PATCOPY;
    	break;
    case vHidd_GC_DrawMode_NoOp:
        return;
    case vHidd_GC_DrawMode_Xor:
        col = GC_FG(msg->gc);
    	mode = PATINVERT;
    	break;
    case vHidd_GC_DrawMode_Equiv:
        col = !GC_FG(msg->gc);
    	mode = PATINVERT;
    	break;
    case vHidd_GC_DrawMode_Invert:
        col = 0xFFFFFFFF;
    	mode = PATINVERT;
    	break;
    case vHidd_GC_DrawMode_CopyInverted:
        col = !GC_FG(msg->gc);
    	mode = PATCOPY;
    	break;
    case vHidd_GC_DrawMode_Set:
        mode = WHITENESS;
    	break;
    default:
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
    LOCK_GDI
    D(bug("[GDI] Brush color 0x%08lX, mode 0x%08lX\n", col, mode));
    br = GDICALL(CreateSolidBrush, col);
    if (br) {
        orig_br = GDICALL(SelectObject, data->dc, br);
        GDICALL(PatBlt, data->dc, msg->minX, msg->minY, msg->maxX - msg->minX + 1, msg->maxY - msg->minY + 1, mode);
        GDICALL(SelectObject, data->dc, orig_br);
        GDICALL(DeleteObject, br);
    }
    REFRESH(msg->minX, msg->minY, msg->maxX + 1 , msg->maxY + 1)
    UNLOCK_GDI    
}

/****************************************************************************************/

VOID GDIBM__Hidd_BitMap__BlitColorExpansion(OOP_Class *cl, OOP_Object *o,
					    struct pHidd_BitMap_BlitColorExpansion *msg
)
{
    struct bitmap_data  *data = OOP_INST_DATA(cl, o);
    HIDDT_Pixel     	 fg, bg;
    ULONG   	    	 cemd;
    LONG    	    	 x, y;    
    APTR 	    	 d = 0;
    
    EnterFunc(bug("GDIGfx.BitMap::BlitColorExpansion(%p, %d, %d, %d, %d, %d, %d)\n",
    	    	  msg->srcBitMap, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height));

/*  OOP_GetAttr(msg->srcBitMap, aHidd_GDIBitMap_Window, (IPTR *)&d);
    
    if (0 == d)
    {*/
    	/* We know nothing about the source bitmap. Let the superclass handle this */
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	return;
/*  }
    
    fg = GC_FG(msg->gc);
    bg = GC_BG(msg->gc);
    cemd = GC_COLEXP(msg->gc);

    if (0 != d)
    {
    	LOCK_GDI    

	XCALL(XSetForeground, data->display, data->gc, fg);
		
    	if (cemd & vHidd_GC_ColExp_Opaque)  
	{
	    XCALL(XSetBackground, data->display, data->gc, bg);
	    XCALL(XSetFunction, data->display, data->gc, GXcopy);
	    
	    XCALL(XCopyPlane, data->display, d, DRAWABLE(data), data->gc,
	    	       msg->srcX, msg->srcY, msg->width, msg->height,
		       msg->destX, msg->destY, 0x01);
	}
	else
	{*/
	    /* Do transparent blit */
/*	    
	    XGCValues val;
	    
	    val.stipple		= d;
	    val.ts_x_origin	= msg->destX - msg->srcX;
	    val.ts_y_origin	= msg->destY - msg->srcY;
	    val.fill_style	= FillStippled;

	    XCALL(XSetFunction, data->display, data->gc, GC_DRMD(msg->gc));
	    
	    XCALL(XChangeGC, data->display, data->gc,
	    	      GCStipple|GCTileStipXOrigin|GCTileStipYOrigin|GCFillStyle,
		      &val);
		      
	    XCALL(XFillRectangle, data->display, DRAWABLE(data), data->gc,
	    	    	   msg->destX, msg->destY, msg->width, msg->height);
	    
	    XCALL(XSetFillStyle, data->display, data->gc, FillSolid);

	}
	
    	UNLOCK_GDI	

    }*/
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

VOID GDIBM__Hidd_BitMap__DrawLine(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawLine *msg)
{
/*  struct bitmap_data  *data = OOP_INST_DATA(cl, o);
    OOP_Object      	*gc = msg->gc;
    
    if (GC_LINEPAT(gc) != (UWORD)~0)
    {*/
    	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	
	return;
/*  }
    
    LOCK_GDI
    
    if (GC_DOCLIP(gc))
    {
    	XRectangle cr;
	
	cr.x = GC_CLIPX1(gc);
	cr.y = GC_CLIPY1(gc);
	cr.width  = GC_CLIPX2(gc) - cr.x + 1;
	cr.height = GC_CLIPY2(gc) - cr.y + 1;
    
    	XCALL(XSetClipRectangles, data->display, data->gc,
	    	    	   0, 0, &cr, 1, Unsorted);
    }
    
    XCALL(XSetForeground, data->display, data->gc, GC_FG(gc));
    XCALL(XSetFunction, data->display, data->gc, GC_DRMD(gc));
    
    XCALL(XDrawLine, data->display, DRAWABLE(data), data->gc,
    	      msg->x1, msg->y1, msg->x2, msg->y2);
	
    if (GC_DOCLIP(gc))
    {
    	XCALL(XSetClipMask, data->display, data->gc, None);
    }	
    
    XFLUSH(data->display);
    
    UNLOCK_GDI*/
}

/****************************************************************************************/

VOID GDIBM__Hidd_BitMap__DrawEllipse(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawEllipse *msg)
{
    EnterFunc(bug("[GDI] hidd.bitmap.gdibitmap::DrawEllipse()\n"));
/*  struct bitmap_data  *data = OOP_INST_DATA(cl, o);
    OOP_Object      	*gc = msg->gc;
    
    LOCK_GDI
    
    if (GC_DOCLIP(gc))
    {
    	XRectangle cr;
	
    	D(kprintf("GDI::Drawllipse: clip %d %d %d %d\n"
	    	    , GC_CLIPX1(gc), GC_CLIPY1(gc), GC_CLIPX2(gc), GC_CLIPY2(gc)));
		
	cr.x = GC_CLIPX1(gc);
	cr.y = GC_CLIPY1(gc);
	cr.width  = GC_CLIPX2(gc) - cr.x + 1;
	cr.height = GC_CLIPY2(gc) - cr.y + 1;
    
    	XCALL(XSetClipRectangles, data->display, data->gc,
	    	    	   0, 0, &cr, 1, Unsorted);
    }
    
    XCALL(XSetForeground, data->display, data->gc, GC_FG(gc));
    XCALL(XSetFunction, data->display, data->gc, GC_DRMD(gc));
    
    D(kprintf("GDI::Drawllipse: coord %d %d %d %d\n"
	    	, msg->x, msg->y, msg->rx, msg->ry));
    
    XCALL(XDrawArc, data->display, DRAWABLE(data), data->gc,
    	     msg->x - msg->rx, msg->y - msg->ry,
	     msg->rx * 2, msg->ry * 2, 0, 360 * 64);
	
    if (GC_DOCLIP(gc))
    {
    	XCALL(XSetClipMask, data->display, data->gc, None);
    }	
    
    XFLUSH(data->display);
    
    UNLOCK_GDI*/
}

/****************************************************************************************/

#define AO(x) 	    	  (aoHidd_BitMap_ ## x)
#define GOT_BM_ATTR(code) GOT_ATTR(code, aoHidd_BitMap, bitmap)

/****************************************************************************************/

OOP_Object *GDIBM__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    OOP_Object  *friend = NULL, *pixfmt;
/*  APTR 	 friend_drawable = NULL;*/
    APTR	 display, my_dc, my_bitmap, orig_bitmap;
    ULONG   	 width, height;
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

    display = (APTR)GetTagData(aHidd_GDIGfx_SysDisplay, 0, msg->attrList);

    D(bug("Creating GDI bitmap: %ldx%ldx%ld\n", width, height, depth));

    LOCK_GDI
    my_dc = GDICALL(CreateCompatibleDC, display);
    D(bug("[GDI] Memory device context: 0x%p\n", my_dc));
    if (my_dc) {
        my_bitmap = GDICALL(CreateCompatibleBitmap, display, width, height);
        D(bug("[GDI] Memory bitmap: 0x%p\n", my_bitmap));
        if (my_bitmap)
            orig_bitmap = GDICALL(SelectObject, my_dc, my_bitmap);
        D(bug("[GDI] Olriginal DC bitmap: 0x%p\n", orig_bitmap));
    }
    UNLOCK_GDI

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
/*	data->cursor  = (Cursor)   GetTagData(aHidd_GDIGfx_SysCursor,  0, msg->attrList);*/
	data->dc = my_dc;
	data->bitmap = my_bitmap;
	data->dc_bitmap = orig_bitmap;
    	ReturnPtr("GDIGfx.BitMap::New()", OOP_Object *, o);
    } /* if (object allocated by superclass) */
dispose_bitmap:    
    LOCK_GDI
    if (orig_bitmap)
    	GDICALL(SelectObject, my_dc, orig_bitmap);
    if (my_bitmap)
        GDICALL(DeleteObject, my_bitmap);
    GDICALL(DeleteDC, my_dc);
    UNLOCK_GDI
    
    ReturnPtr("GDIGfx.BitMap::New()", OOP_Object *, NULL);
    
}

/****************************************************************************************/

VOID GDIBM__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);

    EnterFunc(bug("GDIGfx.BitMap::Dispose()\n"));
    
    LOCK_GDI
    if (data->dc_bitmap)
    	GDICALL(SelectObject, data->dc, data->dc_bitmap);
    if (data->bitmap)
        GDICALL(DeleteObject, data->bitmap);
    if (data->dc)
        GDICALL(DeleteDC, data->dc);
    UNLOCK_GDI
    
    OOP_DoSuperMethod(cl, o, msg);
    
    ReturnVoid("GDIGfx.BitMap::Dispose");
}

/****************************************************************************************/

VOID GDIBM__Hidd_BitMap__Clear(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_Clear *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    IPTR   	    	width, height;
    
    EnterFunc(bug("[GDI] hidd.bitmap.gdibitmap::Clear()\n"));
    
    /* Get width & height from bitmap superclass */
  
    OOP_GetAttr(o, aHidd_BitMap_Width,  &width);
    OOP_GetAttr(o, aHidd_BitMap_Height, &height);
    
/*  LOCK_GDI 
    XCALL(XSetForeground, data->display, data->gc, GC_BG(msg->gc));
    XCALL(XFillRectangle, data->display, DRAWABLE(data), data->gc,
    	    	   0 , 0, width, height);    
    XCALL(XFlush, data->display);
    UNLOCK_GDI*/
    
}

/****************************************************************************************/

#undef XSD
#define XSD(cl) (&LIBBASE->xsd)

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
