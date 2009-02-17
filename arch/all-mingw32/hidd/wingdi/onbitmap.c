/*
    Copyright  1995-2008, The AROS Development Team. All rights reserved.
    $Id: onbitmap.c 26918 2007-10-02 02:55:49Z rob $

    Desc: Bitmap class for GDI hidd.
    Lang: English.
*/

#define __OOP_NOATTRBASES__

#include <stdlib.h>
#include <stdio.h>

#include <string.h>
#include <signal.h>
#include <unistd.h>

#include <proto/kernel.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include <exec/memory.h>
#include <exec/lists.h>

#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <oop/oop.h>

#include <hidd/graphics.h>

#include <aros/symbolsets.h>

#define DEBUG 1
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

#include "gdigfx_intern.h"
#include "gdi.h"

#include "bitmap.h"
#include "winapi.h"

/****************************************************************************************/

static OOP_AttrBase HiddBitMapAttrBase;
OOP_AttrBase HiddPixFmtAttrBase;
static OOP_AttrBase HiddGDIGfxAB;
static OOP_AttrBase HiddGDIBitMapAB;

/****************************************************************************************/

static struct OOP_ABDescr attrbases[] = 
{
    { IID_Hidd_BitMap	, &HiddBitMapAttrBase 	},
    { IID_Hidd_PixFmt	, &HiddPixFmtAttrBase 	},
    /* Private bases */
    { IID_Hidd_GDIGfx	, &HiddGDIGfxAB	    	},
    { IID_Hidd_GDIBitMap, &HiddGDIBitMapAB  	},
    { NULL  	    	, NULL      	    	}
};

/****************************************************************************************/

/* Macro trick to reuse code between offscreen and onscreen bitmap hidd
(bitmap_common.c) */

#define GET_DC(data) USERCALL(GetDC, (data)->drawable)
#define FREE_DC(data, dc) USERCALL(ReleaseDC, (data)->drawable, dc)

#if ADJUST_XWIN_SIZE
#define MASTERWIN(data) (data)->masterxwindow
#else
#define MASTERWIN(data) DRAWABLE(data)
#endif

#define MNAME(x) GDIOnBM__ ## x

/****************************************************************************************/

/* !!! Include methods whose implementation is eqaul for windows and bitmaps
 (except the DRAWABLE) */

#include "bitmap_common.c"

#if GDISOFTMOUSE
static void init_empty_cursor(Window w, GC c, struct gdi_staticdata *xsd);
#endif
/*static Pixmap init_icon(Display *d, Window w, Colormap cm, LONG depth,
    	    	    	struct gdi_staticdata *xsd);
*/

void GfxIntHandler(struct NewWindowMsg *nw, struct Task *task)
{
    Signal(task, SIGF_BLIT);
}

/****************************************************************************************/

OOP_Object *GDIOnBM__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct NewWindowMsg nw;
    BOOL ok = TRUE;
    
    EnterFunc(bug("GDIGfx.OnBitMap::New()\n"));
    
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
    	struct bitmap_data *data;
        IPTR width, height;
        struct NewWindowMsg nw;
        struct Task *me;
        void *gfx_int;
	
        data = OOP_INST_DATA(cl, o);
	
	/* clear all data  */
        memset(data, 0, sizeof(struct bitmap_data));
	
	/* Get some info passed to us by the gdigfxhidd class */
/*	data->display = (Display *)GetTagData(aHidd_GDIGfx_SysDisplay, 0, msg->attrList);
	data->screen  =            GetTagData(aHidd_GDIGfx_SysScreen,  0, msg->attrList);
	data->cursor  = (Cursor)   GetTagData(aHidd_GDIGfx_SysCursor,  0, msg->attrList);
	data->colmap  = (Colormap) GetTagData(aHidd_GDIGfx_ColorMap,   0, msg->attrList);*/

	/* Get attr values */
	OOP_GetAttr(o, aHidd_BitMap_Width,  &width);
	OOP_GetAttr(o, aHidd_BitMap_Height, &height);

	/* Open a window to be used for viewing */
	nw.xsize = width;
	nw.ysize = height;
	nw.window = NULL;
	me = FindTask(NULL);
	D(bug("Creating a window: %lux%lu\n", width, height));
	gfx_int = KrnAddExceptionHandler(2, GfxIntHandler, &nw, me);
	if (gfx_int) {
	    /* Send a message to the GDI thread to create a window */
	    if (NATIVECALL(GDI_PutMsg, NULL, NOTY_WINCREATE, (IPTR)&nw, 0))
	        Wait(SIGF_BLIT);
	    KrnRemExceptionHandler(gfx_int);
	}
	D(bug("Created window 0x%p\n", nw.window));
    	if (nw.window) {
    	    data->drawable = nw.window;
    	} else {
            OOP_MethodID disp_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
	    
    	    OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);
	    o = NULL;
    	}
    } /* if (object allocated by superclass) */

    ReturnPtr("GDIGfx.OnBitMap::New()", OOP_Object *, o);
}

/****************************************************************************************/

VOID GDIOnBM__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    
    EnterFunc(bug("GDIGfx.BitMap::Dispose()\n"));
    
    NATIVECALL(GDI_PutMsg, data->drawable, WM_CLOSE, 0, 0);
    OOP_DoSuperMethod(cl, o, msg);
    
    ReturnVoid("GDIGfx.BitMap::Dispose");
}

/****************************************************************************************/

VOID GDIOnBM__Hidd_BitMap__Clear(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_Clear *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    ULONG   	    	width, height;
    
//  XSetWindowAttributes winattr;
        
    /* Get width & height from bitmap */
  
/*  OOP_GetAttr(o, aHidd_BitMap_Width,  &width);
    OOP_GetAttr(o, aHidd_BitMap_Height, &height);
    
    winattr.background_pixel = GC_BG(msg->gc);

    LOCK_GDI    
    XCALL(XChangeWindowAttributes, data->display, DRAWABLE(data),
    	    	    	    CWBackPixel, &winattr);
    
    XCALL(XClearArea, data->display, DRAWABLE(data),
	    0, 0,
	    width, height,
	    FALSE);
    
    XCALL(XFlush, data->display);
    UNLOCK_GDI            */
}

/****************************************************************************************/

#undef XSD
#define XSD(cl) (&LIBBASE->xsd)

#define NUM_ROOT_METHODS 4

#if USE_GDI_DRAWFUNCS
#   define NUM_BITMAP_METHODS 13
#else
#   define NUM_BITMAP_METHODS 11
#endif

/****************************************************************************************/

static int GDIOnBM_Init(LIBBASETYPEPTR LIBBASE)
{
    return OOP_ObtainAttrBases(attrbases);
}

/****************************************************************************************/

static int GDIOnBM_Expunge(LIBBASETYPEPTR LIBBASE)
{
    OOP_ReleaseAttrBases(attrbases);
    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(GDIOnBM_Init, 0);
ADD2EXPUNGELIB(GDIOnBM_Expunge, 0);

#if GDISOFTMOUSE

/****************************************************************************************/

static void init_empty_cursor(Window w, GC gc, struct gdi_staticdata *xsd)
{
    Pixmap  p, mask; 
    int     width, height;
    
    width = height = 1;

    LOCK_GDI    
    p = XCALL(XCreatePixmap,  xsd->display, w, width, height, 1);
    UNLOCK_GDI    


    if (0 != p)
    {
    	LOCK_GDI    
	mask = XCALL(XCreatePixmap,  xsd->display
		, w
		, width
		, height
		, 1
    	);
	XCALL(XFlush, xsd->display);	
    	UNLOCK_GDI    
	
	if (0 != mask)
	{
	    /* Define cursor for window */
	    XColor fg, bg;
	    Cursor c;
	    int    x, y;

    	    LOCK_GDI
	    XCALL(XSetForeground, xsd->display, gc, 0);
	    XCALL(XSetFunction, xsd->display, gc, GXcopy);	    
    	#if 0	    
	    XCALL(XFillRectangle, xsd->display, p, gc, 1, 1, 1, 1);
	    for (y = 0; y < height; y ++)
	    {
	    	for (x = 0; x < width; x ++)
		{
		    XCALL(XDrawPoint, xsd->display, mask, gc, x, y);
		}
	    }
    	#endif	    
    	    UNLOCK_GDI	
	        
	    fg.pixel	= BlackPixel(xsd->display, DefaultScreen(xsd->display));
	    fg.red	= 0x0000;
	    fg.green	= 0x0000;
	    fg.blue	= 0x0000;
	    fg.flags	= DoRed | DoGreen | DoBlue;

	    bg.pixel	= WhitePixel(xsd->display, DefaultScreen(xsd->display));
	    bg.red	= 0xFFFF;
	    bg.green	= 0xFFFF;
	    bg.blue	= 0xFFFF;
	    bg.flags	= DoRed | DoGreen | DoBlue;

    	    LOCK_GDI
	    c = XCALL(XCreatePixmapCursor, xsd->display, p, mask, &fg, &bg, 0, 0);
    	    UNLOCK_GDI
	    	    
	    if (0 != c)
	    {
    	    	LOCK_GDI	    
	    	XCALL(XDefineCursor, xsd->display, w, c);
    	    	UNLOCK_GDI		
	    }
	    
    	    LOCK_GDI	    
	    XCALL(XFreePixmap, xsd->display, mask);
    	    UNLOCK_GDI	    
	}

    	LOCK_GDI	
	XCALL(XFreePixmap, xsd->display, p);
    	UNLOCK_GDI
    }
  	
}

/****************************************************************************************/

#endif

/****************************************************************************************/
#ifdef NOT_YET
static Pixmap init_icon(Display *d, Window w, Colormap cm, LONG depth, struct gdi_staticdata *xsd)
{
    #include "icon.h"

    #define SHIFT_PIX(pix, shift)	\
	(( (shift) < 0) ? (pix) >> (-shift) : (pix) << (shift) )
    
    Pixmap   icon = XCALL(XCreatePixmap, d, w, width, height, depth);
    char    *data = header_data;
    LONG     red_shift, green_shift, blue_shift;
    GC       gc;
    
    red_shift   = 24 - xsd->red_shift;
    green_shift = 24 - xsd->green_shift;
    blue_shift  = 24 - xsd->blue_shift;
    
    if (icon)
    {
    	gc = XCALL(XCreateGC, d, icon, 0, 0);
	
	if (gc)
	{
    	    WORD x, y;

	    for(y = 0; y < height; y++)
	    {
	    	for(x = 0; x < width; x++)
		{
	    	    ULONG rgb[3];
		    ULONG pixel = 0;

		    HEADER_PIXEL(data,rgb);

		    if (xsd->vi.class == TrueColor)
    	    	    {
			pixel = (SHIFT_PIX(rgb[0] & 0xFF, red_shift)   & xsd->vi.red_mask)   |
		    		(SHIFT_PIX(rgb[1] & 0xFF, green_shift) & xsd->vi.green_mask) |
				(SHIFT_PIX(rgb[2] & 0xFF, blue_shift)  & xsd->vi.blue_mask);		    	
		    }
		    else if (xsd->vi.class == PseudoColor)
		    {
		    	XColor xcol;
			
			xcol.red   = (rgb[0] << 8) + rgb[0];
			xcol.green = (rgb[1] << 8) + rgb[1];
			xcol.blue  = (rgb[2] << 8) + rgb[2];
			xcol.flags = DoRed | DoGreen | DoBlue;
			
			if (XCALL(XAllocColor, d, cm, &xcol))
			{
			    pixel = xcol.pixel;
			}
		    }
		    
    	    	    XCALL(XSetForeground, d, gc, pixel);
		    XCALL(XDrawPoint, d, icon, gc, x, y);
		}
	    }
	    
	    XCALL(XFreeGC, d, gc);
	    
	} /* if (gc) */
	
    } /* if (icon) */
    
    return icon;
}
#endif
/****************************************************************************************/
