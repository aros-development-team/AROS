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
#define DEBUG 0
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

#include "gdigfx_intern.h"
#include "gdi.h"

#include "bitmap.h"

/****************************************************************************************/

#define AO(x) 	    	  (aoHidd_BitMap_ ## x)
#define GOT_BM_ATTR(code) GOT_ATTR(code, aoHidd_BitMap, bitmap)

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

/* Table of raster operations (ROPs) corresponding to AROS GC drawmodes */
static ULONG DrawModeTable[] = {
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

static void FillRect(OOP_Class *cl, struct bitmap_data *data, OOP_Object *gc, ULONG minX, ULONG minY, ULONG maxX, ULONG maxY)
{
    APTR br, orig_br;
    ULONG col, mode;
    
    col = GC_FG(gc);
    mode = DrawModeTable[GC_DRMD(gc)];
    D(bug("[GDI] Brush color 0x%08lX, mode 0x%08lX\n", col, mode));

    LOCK_GDI
    br = GDICALL(CreateSolidBrush, col);
    if (br) {
        orig_br = GDICALL(SelectObject, data->dc, br);
        GDICALL(PatBlt, data->dc, minX, minY, maxX - minX + 1, maxY - minY + 1, mode);
        GDICALL(SelectObject, data->dc, orig_br);
        GDICALL(DeleteObject, br);
    }
    REFRESH(minX, minY, maxX + 1, maxY + 1)
    UNLOCK_GDI
}

VOID GDIBM__Hidd_BitMap__FillRect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);

    D(bug("[GDI] hidd.bitmap.gdibitmap::FillRect(0x%p, %d,%d,%d,%d)\n", o, msg->minX, msg->minY, msg->maxX, msg->maxY));
    FillRect(cl, data, msg->gc, msg->minX, msg->minY, msg->maxX, msg->maxY);
}

/****************************************************************************************/

ULONG GDIBM__Hidd_BitMap__DrawPixel(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawPixel *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    
    /* Unfortunately GDI supports raster operations only in BitBlt() and in PatBlt() so we
       have to emulate all functions using them. However it's necessary to overload as many
       methods as possible because GetPixel()/PutPixel() are REALLY slow.
       Here we implement DrawPixel() as filling 1x1 rectangle */
    FillRect(cl, data, msg->gc, msg->x, msg->y, msg->x, msg->y);    
    return 0;    
}

/****************************************************************************************/

VOID GDIBM__Hidd_BitMap__BlitColorExpansion(OOP_Class *cl, OOP_Object *o,
					    struct pHidd_BitMap_BlitColorExpansion *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    HIDDT_Pixel fg, bg;
    ULONG cemd;
    APTR d = NULL;
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
    cemd = GC_COLEXP(msg->gc);

    LOCK_GDI
    /* First we convert a source bitmap to 1-plane mask. We do it by creating a monochrome bitmap and copying our mask to it. */
    mask_dc = GDICALL(CreateCompatibleDC, data->display);
    if (mask_dc) {
        mask_bm = GDICALL(CreateCompatibleBitmap, mask_dc, msg->width, msg->height);
        if (mask_bm) {
            mask_dc_bm = GDICALL(SelectObject, mask_dc, mask_bm);
            if (mask_dc_bm) {
                /* During this first blit, pixels equal to BkColor, become WHITE. Others become BLACK. This converts
                   our truecolor display-compatible bitmap to a monochrome bitmap. A monochrome bitmap can be effectively
                   used for masking in blit operations. AND operations with WHITE will leave pixels intact, AND with BLACK
                   gives black. OR with black also leaves intact. */
                GDICALL(SetBkColor, d, 0);
                GDICALL(BitBlt, mask_dc, 0, 0, msg->width, msg->height, d, msg->srcX, msg->srcY, SRCCOPY);
                /* Prepare a background first if needed */
                if (cemd & vHidd_GC_ColExp_Opaque) {
                    /* In opaque mode we first prepare a background. We do it by making a solid brush of background color
                       and then blitting mask with MERGECOPY operation. MERGECOPY means "dest = source AND brush".
                       Remember that source here is monochrome version of our mask.
                       After this operation we get a background with black holes into which we will blit a foregroing
                       color later */
                    br = GDICALL(CreateSolidBrush, bg);
                    if (br) {
                        dc_br = GDICALL(SelectObject, data->dc, br);
                        if (dc_br) {
                            GDICALL(BitBlt, data->dc, msg->destX, msg->destY, msg->width, msg->height, mask_dc, 0, 0, MERGECOPY);
                            GDICALL(SelectObject, data->dc, dc_br);
                        }
                        GDICALL(DeleteObject, br);
                    }
                } else
                    /* In transparent mode we simply clear masked area using "dest = dest AND src" operation */
                    GDICALL(BitBlt, data->dc, msg->destX, msg->destY, msg->width, msg->height, mask_dc, 0, 0, SRCAND);
                br = GDICALL(CreateSolidBrush, fg);
    		if (br) {
    		    dc_br = GDICALL(SelectObject, data->dc, br);
    		    if (dc_br) {
    		        /* This unnamed operation code means "dest = dest OR (brush AND (NOT source))". In converts pixels which
    		           are BLACK in our mask info foreground (brush) color and then merges the result with what we already
    		           have in our destination bitmap */
    		        GDICALL(BitBlt, data->dc, msg->destX, msg->destY, msg->width, msg->height, mask_dc, 0, 0, 0x00BA0B09);
    		    	GDICALL(SelectObject, data->dc, dc_br);
    		    }
    		    GDICALL(DeleteObject, br);
    		}
    		GDICALL(SelectObject, mask_dc, mask_dc_bm);
    	    }
    	    GDICALL(DeleteObject, mask_bm);
	}
	GDICALL(DeleteDC, mask_dc);
    }
    /* TODO: an alternative way to implement this function is to attach a palette to a monochrome bitmap and then directly blit it in
       opaque mode. Probably it will be even faster than this two-op version. */
    REFRESH(msg->destX, msg->destY, msg->destX + msg->width, msg->destY + msg->height);
    UNLOCK_GDI
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
    IPTR width, height;
    APTR br;
    RECT rect = {0, 0, 0, 0};
    
    EnterFunc(bug("[GDI] hidd.bitmap.gdibitmap::Clear()\n"));
    
    /* Get width & height from bitmap superclass */
  
    OOP_GetAttr(o, aHidd_BitMap_Width,  &width);
    OOP_GetAttr(o, aHidd_BitMap_Height, &height);
    rect.right = width;
    rect.bottom = height;
    
    D(bug("[GDI] Brush color 0x%08lX\n", GC_BG(msg->gc)));
    LOCK_GDI
    br = GDICALL(CreateSolidBrush, GC_BG(msg->gc));
    if (br) {
        USERCALL(FillRect, data->dc, &rect, br);
        GDICALL(DeleteObject, br);
    }
    REFRESH(0, 0, width , height)
    UNLOCK_GDI    
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
