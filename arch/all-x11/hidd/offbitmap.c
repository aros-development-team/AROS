/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Offscreen bitmap class for X11 hidd.
    Lang: English.
*/

/****************************************************************************************/

#define __OOP_NOATTRBASES__

#include <stdlib.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

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

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

#include "x11gfx_intern.h"
#include "x11.h"

#include "bitmap.h"

/****************************************************************************************/

static OOP_AttrBase HiddBitMapAttrBase;
static OOP_AttrBase HiddPixFmtAttrBase;
static OOP_AttrBase HiddX11GfxAB;
static OOP_AttrBase HiddX11BitMapAB;

static struct OOP_ABDescr attrbases[] = 
{
    { IID_Hidd_BitMap	, &HiddBitMapAttrBase 	},
    { IID_Hidd_PixFmt	, &HiddPixFmtAttrBase 	},
    /* Private bases */
    { IID_Hidd_X11Gfx	, &HiddX11GfxAB	    	},
    { IID_Hidd_X11BitMap, &HiddX11BitMapAB    	},
    { NULL  	    	, NULL      	    	}
};

/****************************************************************************************/

/* Macro trick to reuse code between offscreen and onscreen bitmap hidd
(bitmap_common.c) */

#define DRAWABLE(data)  (data)->drawable.pixmap

#define MNAME(x)    	X11OffBM__ ## x

/* !!! Include methods whose implementation is eqaul for windows and pixmaps
 (except the DRAWABLE) */


#include "bitmap_common.c"

/****************************************************************************************/

#define AO(x) 	    	  (aoHidd_BitMap_ ## x)
#define GOT_BM_ATTR(code) GOT_ATTR(code, aoHidd_BitMap, bitmap)

/****************************************************************************************/

OOP_Object *X11OffBM__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    OOP_Object  *friend = NULL, *pixfmt;
    Drawable 	 friend_drawable = 0, d = 0;
    Display 	*display;
    ULONG   	 width, height, depth;
    IPTR    	 attrs[num_Hidd_BitMap_Attrs];
    int     	 screen;
    BOOL    	 ok = TRUE;
    
    DECLARE_ATTRCHECK(bitmap);
    
    /* Parse the attributes */
    if (0 != OOP_ParseAttrs(msg->attrList, attrs, num_Hidd_BitMap_Attrs,
    	    	    	    &ATTRCHECK(bitmap), HiddBitMapAttrBase))
    {
    	kprintf("!!! X11Gfx::OffBitMap() FAILED TO PARSE ATTRS !!!\n");
	
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
    
    /* Get the X11 window from the friend bitmap */
    if (NULL != friend)
    {
	OOP_GetAttr(friend, aHidd_X11BitMap_Drawable, &friend_drawable);
    }
    else
    {
	friend_drawable = XSD(cl)->dummy_window_for_creating_pixmaps;
    }
	
    if (0 == friend_drawable)
    {
	kprintf("ALERT!!! FRIEND BITMAP HAS NO DRAWABLE in config/x11/hidd/offbitmap.c\n");
	Alert(AT_DeadEnd);
    }
	
    display = (Display *)GetTagData(aHidd_X11Gfx_SysDisplay, 0, msg->attrList);
    screen  =            GetTagData(aHidd_X11Gfx_SysScreen,  0, msg->attrList);

    /*  We must only create depths that are supported by the friend drawable
	Currently we only support the default depth, and depth 1
    */
    if (depth != 1)
    {
    	LOCK_X11	
	depth = DefaultDepth(display, screen);
    	UNLOCK_X11
    }
    else
    {
    	#warning "Need this because of stipple bug in XFree86 :-("
	width += 32;
    }
    
    D(bug("Creating X Pixmap, %p, %d, %d, %d\n", friend_drawable, width, height, depth));
	
    LOCK_X11	
    d = XCreatePixmap( display, friend_drawable, width, height, depth);	
    XFlush(display);
    UNLOCK_X11

    if (0 == d)
    	return NULL;
    
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    
    if (NULL == o)
    {
    	/* Delete the drawable */
	goto dispose_pixmap;
    }
    else
    {
    	struct bitmap_data *data;
	XGCValues   	    gcval;
	
        data = OOP_INST_DATA(cl, o);
	
	/* clear all data  */
        memset(data, 0, sizeof(struct bitmap_data));
	
	/* Get some info passed to us by the x11gfxhidd class */
	DRAWABLE(data) = d;
	data->display = display;
	data->screen  = screen;
	data->cursor  = (Cursor)   GetTagData(aHidd_X11Gfx_SysCursor,  0, msg->attrList);
	data->colmap  = (Colormap) GetTagData(aHidd_X11Gfx_ColorMap,   0, msg->attrList);

	    /* Create X11 GC */
	D(bug("Creating GC\n"));
	 
	gcval.plane_mask = AllPlanes;
	gcval.graphics_exposures = False;
	 
    	LOCK_X11	    
	data->gc = XCreateGC( data->display, DRAWABLE(data),
	    	    	      GCPlaneMask | GCGraphicsExposures, &gcval);

    	UNLOCK_X11
			
	if (data->gc)
	{
    	    LOCK_X11	    
	    XFlush(data->display);
    	    UNLOCK_X11
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


    } /* if (object allocated by superclass) */
    
    
    ReturnPtr("X11Gfx.OffBitMap::New()", OOP_Object *, o);

dispose_pixmap:    
    LOCK_X11
    XFreePixmap(display, d);
    XFlush(display);
    UNLOCK_X11
    
    ReturnPtr("X11Gfx.OffBitMap::New()", OOP_Object *, NULL);
    
}

/****************************************************************************************/

VOID X11OffBM__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);

    EnterFunc(bug("X11Gfx.BitMap::Dispose()\n"));
    
    if (data->gc)
    {
    	LOCK_X11
    	XFreeGC(data->display, data->gc);
    	UNLOCK_X11	
    }
    
    if (DRAWABLE(data))
    {
    	LOCK_X11	
    	XFreePixmap( GetSysDisplay(), DRAWABLE(data));
	XFlush( GetSysDisplay() );
    	UNLOCK_X11	
    }
    
    OOP_DoSuperMethod(cl, o, msg);
    
    ReturnVoid("X11Gfx.BitMap::Dispose");
}

/****************************************************************************************/

VOID X11OffBM__Hidd_BitMap__Clear(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_Clear *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    ULONG   	    	width, height;
        
    /* Get width & height from bitmap superclass */
  
    OOP_GetAttr(o, aHidd_BitMap_Width,  &width);
    OOP_GetAttr(o, aHidd_BitMap_Height, &height);
    
    LOCK_X11 
    XSetForeground(data->display, data->gc, GC_BG(msg->gc));
    XFillRectangle(data->display, DRAWABLE(data), data->gc,
    	    	   0 , 0, width, height);    
    XFlush(data->display);
    UNLOCK_X11
    
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

AROS_SET_LIBFUNC(X11OffBM_Init, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    return OOP_ObtainAttrBases(attrbases);

    AROS_SET_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_SET_LIBFUNC(X11OffBM_Expunge, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    OOP_ReleaseAttrBases(attrbases);
    return TRUE;

    AROS_SET_LIBFUNC_EXIT
}

/****************************************************************************************/

ADD2INITLIB(X11OffBM_Init, 0);
ADD2EXPUNGELIB(X11OffBM_Expunge, 0);
