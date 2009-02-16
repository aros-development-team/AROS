/*
    Copyright  1995-2006, The AROS Development Team. All rights reserved.
    $Id: offbitmap.c 26918 2007-10-02 02:55:49Z rob $

    Desc: Offscreen bitmap class for GDI hidd.
    Lang: English.
*/

/****************************************************************************************/

#define __OOP_NOATTRBASES__

#include <stdlib.h>
#include <stdio.h>

#include <string.h>

#include <proto/oop.h>
#include <proto/utility.h>

#include <exec/memory.h>
#include <exec/lists.h>

#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <oop/oop.h>
#include <exec/alerts.h>

#include <hidd/graphics.h>

#include <aros/symbolsets.h>

#define SDEBUG 1
#define DEBUG 1
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

#include "gdigfx_intern.h"
#include "gdi.h"

#include "bitmap.h"
#include "winapi.h"

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

/* Macro trick to reuse code between offscreen and onscreen bitmap hidd
(bitmap_common.c) */

#define GET_DC(data)  (data)->drawable
#define FREE_DC(data, dc)

#define MNAME(x)    	GDIOffBM__ ## x

/* !!! Include methods whose implementation is eqaul for windows and bitmaps
 (except the DRAWABLE) */

#include "bitmap_common.c"

/****************************************************************************************/

#define AO(x) 	    	  (aoHidd_BitMap_ ## x)
#define GOT_BM_ATTR(code) GOT_ATTR(code, aoHidd_BitMap, bitmap)

/****************************************************************************************/

OOP_Object *GDIOffBM__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    OOP_Object  *friend = NULL, *pixfmt;
    APTR 	 friend_drawable = NULL;
    APTR	 display;
    ULONG   	 width, height;
    IPTR	 depth;
    IPTR    	 attrs[num_Hidd_BitMap_Attrs];
    int     	 screen;
    BOOL    	 ok = TRUE;
    struct bitmap_data *data;
    
    DECLARE_ATTRCHECK(bitmap);

    EnterFunc(bug("GDIOffBM::New()\n"));    
    /* Parse the attributes */
    if (0 != OOP_ParseAttrs(msg->attrList, attrs, num_Hidd_BitMap_Attrs,
    	    	    	    &ATTRCHECK(bitmap), HiddBitMapAttrBase))
    {
    	D(kprintf("!!! GDIGfx::OffBitMap() FAILED TO PARSE ATTRS !!!\n"));
	
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

    /* Get the GDI object from the friend bitmap */
    if (NULL != friend)
    {
	OOP_GetAttr(friend, aHidd_GDIBitMap_Drawable, (IPTR *)&friend_drawable);
    }

    display = (APTR)GetTagData(aHidd_GDIGfx_SysDisplay, 0, msg->attrList);
    
    D(bug("Creating GDI bitmap, %p, %d, %d, %d\n", friend_drawable, width, height, depth));

    data = OOP_INST_DATA(cl, o);
    /* clear all data  */
    memset(data, 0, sizeof(struct bitmap_data));
    	
    LOCK_GDI
    data->drawable = GDICALL(CreateCompatibleDC, display);
    if (data->drawable) {
        data->bitmap = GDICALL(CreateCompatibleBitmap, display, width, height);
        if (data->bitmap)
            data->dc_bitmap = GDICALL(SelectObject, data->drawable, data->bitmap);
    }
    UNLOCK_GDI

    if (!data->drawable)
    	return NULL;
    if (!data->dc_bitmap)
        goto dispose_bitmap;
    
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    
    if (NULL == o)
    {
    	/* Delete the drawable */
	goto dispose_bitmap;
    }
    else
    {
	/* Get some info passed to us by the gdigfxhidd class */
	data->display = display;
/*	data->cursor  = (Cursor)   GetTagData(aHidd_GDIGfx_SysCursor,  0, msg->attrList);
	data->colmap  = (Colormap) GetTagData(aHidd_GDIGfx_ColorMap,   0, msg->attrList);

	D(bug("Creating GC\n"));
	 
	gcval.plane_mask = AllPlanes;
	gcval.graphics_exposures = False;
	 
    	LOCK_GDI	    
	data->gc = XCALL(XCreateGC,  data->display, DRAWABLE(data),
	    	    	      GCPlaneMask | GCGraphicsExposures, &gcval);

    	UNLOCK_GDI
			
	if (data->gc)
	{
    	    LOCK_GDI	    
	    XCALL(XFlush, data->display);
    	    UNLOCK_GDI
	}
	else
	{
	    ok = FALSE;
	}
	
		
    	if (!ok)
	{
	    OOP_MethodID disp_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
	    
    	    OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);
	    o = NULL;
    	}
*/

    } /* if (object allocated by superclass) */
    
    
    ReturnPtr("GDIGfx.OffBitMap::New()", OOP_Object *, o);

dispose_bitmap:    
    LOCK_GDI
    if (data->dc_bitmap)
    	GDICALL(SelectObject, data->drawable, data->dc_bitmap);
    if (data->bitmap)
        GDICALL(DeleteObject, data->bitmap);
    if (data->drawable)
        GDICALL(DeleteDC, data->drawable);
    UNLOCK_GDI
    
    ReturnPtr("GDIGfx.OffBitMap::New()", OOP_Object *, NULL);
    
}

/****************************************************************************************/

VOID GDIOffBM__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);

    EnterFunc(bug("GDIGfx.BitMap::Dispose()\n"));
    
/*  if (data->gc)
    {
    	LOCK_GDI
    	XCALL(XFreeGC, data->display, data->gc);
    	UNLOCK_GDI	
    }*/
    
    LOCK_GDI
    if (data->dc_bitmap)
    	GDICALL(SelectObject, data->drawable, data->dc_bitmap);
    if (data->bitmap)
        GDICALL(DeleteObject, data->bitmap);
    if (data->drawable)
        GDICALL(DeleteDC, data->drawable);
    UNLOCK_GDI
    
    OOP_DoSuperMethod(cl, o, msg);
    
    ReturnVoid("GDIGfx.BitMap::Dispose");
}

/****************************************************************************************/

VOID GDIOffBM__Hidd_BitMap__Clear(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_Clear *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    IPTR   	    	width, height;
        
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

#undef SDEBUG
#undef DEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

/****************************************************************************************/


#undef XSD
#define XSD(cl) (&LIBBASE->xsd)

/****************************************************************************************/

static int GDIOffBM_Init(LIBBASETYPEPTR LIBBASE)
{
    return OOP_ObtainAttrBases(attrbases);
}

/****************************************************************************************/

static int GDIOffBM_Expunge(LIBBASETYPEPTR LIBBASE)
{
    OOP_ReleaseAttrBases(attrbases);
    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(GDIOffBM_Init, 0);
ADD2EXPUNGELIB(GDIOffBM_Expunge, 0);
