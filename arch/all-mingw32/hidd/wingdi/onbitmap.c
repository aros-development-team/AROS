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
/****************************************************************************************/

OOP_Object *GDIOnBM__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    BOOL ok = TRUE;
    
    D(bug("GDIGfx.OnBitMap::New()\n"));
    
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
    	struct bitmap_data  	*data;
//	Window      	    	 rootwin;	
        IPTR 	    	    	 width, height, depth;
//	XSetWindowAttributes 	 winattr;
	int 	    	    	 visualclass;
	unsigned long 	    	 valuemask;
	
        data = OOP_INST_DATA(cl, o);
	
	/* clear all data  */
        memset(data, 0, sizeof(struct bitmap_data));
	
	/* Get some info passed to us by the gdigfxhidd class */
/*	data->display = (Display *)GetTagData(aHidd_GDIGfx_SysDisplay, 0, msg->attrList);
	data->screen  =            GetTagData(aHidd_GDIGfx_SysScreen,  0, msg->attrList);
	data->cursor  = (Cursor)   GetTagData(aHidd_GDIGfx_SysCursor,  0, msg->attrList);
	data->colmap  = (Colormap) GetTagData(aHidd_GDIGfx_ColorMap,   0, msg->attrList);*/

	/* stegerg*/

/*	visualclass   =		   GetTagData(aHidd_GDIGfx_VisualClass, TrueColor, msg->attrList);
		
        if ( visualclass == PseudoColor)
	{
	    Colormap cm;
	    
    	    LOCK_GDI
	    cm = XCALL(XCreateColormap, GetSysDisplay(),
				 RootWindow(GetSysDisplay(), GetSysScreen()),
				 XSD(cl)->vi.visual,
				 AllocAll);				 
    	    UNLOCK_GDI
	    
	    if (cm)
	    {
	        data->colmap = cm;
		data->flags |= BMDF_COLORMAP_ALLOCED;
	    }
	}*/
	
	/* end stegerg */
	
	/* Get attr values */
	OOP_GetAttr(o, aHidd_BitMap_Width,  &width);
	OOP_GetAttr(o, aHidd_BitMap_Height, &height);

	/* Open an X window to be used for viewing */
	    
	D(bug("Displayable bitmap\n"));
	    
	/* Listen for all sorts of events */
//	winattr.event_mask = 0;
	/* Mouse buttons .. */
//	winattr.event_mask |= ButtonPressMask | ButtonReleaseMask;
	/* Mouse movement .. */
//	winattr.event_mask |= PointerMotionMask;
	/* Key press & release .. */
//	winattr.event_mask |= KeyPressMask | KeyReleaseMask;
	    
	/* We must allways have this one */
//	winattr.event_mask |= StructureNotifyMask;
//	winattr.event_mask |= SubstructureNotifyMask;
	
//	winattr.event_mask |= FocusChangeMask;
#ifdef NOT_YET
	/* Use backing store for now. (Uses lots of mem) */
	winattr.backing_store = Always;

    	LOCK_GDI	
    
	winattr.cursor = GetSysCursor();
	winattr.save_under = True;
	
	winattr.background_pixel = BlackPixel(GetSysDisplay(), GetSysScreen());
	rootwin = DefaultRootWindow (GetSysDisplay());
	D(bug("Creating Window: root win=%p\n", rootwin));
	depth = DefaultDepth(GetSysDisplay(), GetSysScreen());
	
	valuemask = CWBackingStore | CWCursor | CWSaveUnder |
	    	    CWEventMask    | CWBackPixel;
	
	if (data->flags & BMDF_COLORMAP_ALLOCED)
	{
	    winattr.colormap = data->colmap;
	    valuemask |= CWColormap;
	}

    #if ADJUST_XWIN_SIZE
	{
	    XSetWindowAttributes winattr;
	    unsigned long 	 valuemask = 0;

    	    if (XSD(cl)->fullscreen)
	    {
    	    	winattr.override_redirect = True;
    	    	valuemask |= CWOverrideRedirect;
	    }
	    
	    if (data->flags & BMDF_COLORMAP_ALLOCED)
	    {
		winattr.colormap = data->colmap;
		valuemask |= CWColormap;
	    }
	    
	    MASTERWIN(data) = XCALL(XCreateWindow,  GetSysDisplay(),
	    	    	      	    	     rootwin,
					     0,	/* leftedge 	*/
			    	    	     0,	/* topedge	*/
			    	    	     width,
			    	    	     height,
			    	    	     0,	/* BorderWidth	*/
			    	    	     depth,
			    	    	     InputOutput,
			    	    	     DefaultVisual(GetSysDisplay(), GetSysScreen()),
			    	    	     valuemask,
			    	    	     &winattr);
	}
	
	if (MASTERWIN(data)) 
    #endif	

	DRAWABLE(data) = XCALL(XCreateWindow,  GetSysDisplay(),
    	    	    	    	    #if ADJUST_XWIN_SIZE
	    		    	    	MASTERWIN(data),
    	    	    	    	    #else
			    	    	rootwin,
    	    	    	    	    #endif
					0,	/* leftedge 	*/
					0,	/* topedge	*/
					width,
					height,
					0,	/* BorderWidth	*/
					depth,
					InputOutput,
					DefaultVisual (GetSysDisplay(), GetSysScreen()),
					valuemask,
					&winattr);
    	UNLOCK_GDI	    

	D(bug("Xwindow : %p\n", DRAWABLE(data)));

    #if ADJUST_XWIN_SIZE
	if (DRAWABLE(data) && MASTERWIN(data))
	{
    #else
	if (DRAWABLE(data))
	{
            XSizeHints 		 sizehint;
    #endif
	    struct MsgPort 	*port;	    
	    struct notify_msg 	*msg;
    	    Pixmap  	    	 icon;

    	    LOCK_GDI
	    
	    XCALL(XStoreName, GetSysDisplay(), MASTERWIN(data), "AROS");
	    XCALL(XSetIconName, GetSysDisplay(), MASTERWIN(data), "AROS Screen");
		    
    	#if !ADJUST_XWIN_SIZE
	    sizehint.flags      = PMinSize | PMaxSize;
	    sizehint.min_width  = width;
	    sizehint.min_height = height;
	    sizehint.max_width  = width;
	    sizehint.max_height = height;
	    
	    XCALL(XSetWMNormalHints, GetSysDisplay(), MASTERWIN(data), &sizehint);
    	#endif
	    
	    XCALL(XSetWMProtocols, GetSysDisplay(), MASTERWIN(data), &XSD(cl)->delete_win_atom, 1);

    	    icon = init_icon(GetSysDisplay(),
	    	    	     MASTERWIN(data),
			     DefaultColormap(GetSysDisplay(), GetSysScreen()),
			     depth,
			     XSD(cl));
	    if (icon)
	    {
		XWMHints hints;

		hints.icon_pixmap = icon;
		hints.flags = IconPixmapHint;

		XCALL(XSetWMHints, GetSysDisplay(), MASTERWIN(data), &hints);
	    }
	    

	    D(bug("Calling XMapRaised\n"));

/*
  stegerg: XMapRaised is now called inside the GDI task when getting
           the NOTY_MAPWINDOW message, otherwise the GDI task can
	   get a "dead" MapNotify event:
	   
	   XCreateWindow is called here on the app task context.
	   If we also call XMapRaised here then the GDI task might
	   get the MapNotify event before he got the NOTY_WINCREATE
	   message sent from here (see below). So the GDI task
	   would not know about our window and therefore ignore
	   the MapNotify event from X.
	   
	   This caused the freezes which sometimes happened during
	   startup when the Workbench screen was opened.
	   
	   //XCALL(XMapRaised, GetSysDisplay(), DRAWABLE(data));
*/

    	    UNLOCK_GDI	 
	       
	    /* Now we need to get some message from the GDI task about when
	       the window has been mapped (ie. MapWindow event).
	       This is because we cannot render into the window until the
	       it has been mapped.kfind &
	    */

	    /* Create GDI GC */
	    
	    port = CreateMsgPort();
	    msg = AllocMem(sizeof (*msg), MEMF_PUBLIC | MEMF_CLEAR);

	    if (NULL != port && NULL != msg)
	    {
	    	XGCValues gcval;

		/* Send a message to the gdi task that the window has been created */
		
		msg->notify_type = NOTY_WINCREATE;
		msg->xdisplay = GetSysDisplay();
		msg->xwindow = DRAWABLE(data);
		msg->masterxwindow = MASTERWIN(data);
		msg->bmobj = o;
		msg->execmsg.mn_ReplyPort = port;
		
    	    	LOCK_GDI
		XCALL(XSync, GetSysDisplay(), FALSE);
    	    	UNLOCK_GDI

		PutMsg(XSD(cl)->gditask_notify_port, (struct Message *)msg);
				
		/* Wait for the reply, so we are sure that the gdi task
		   has got it */

		WaitPort(port);
		GetMsg(port);
		
    	    #if !DELAY_XWIN_MAPPING		
		/* Send a message to the GDI task to ask when the window has been mapped */
		
   		msg->xdisplay = GetSysDisplay();
		msg->xwindow = DRAWABLE(data);
		msg->masterxwindow = MASTERWIN(data);
		msg->notify_type = NOTY_MAPWINDOW;
		msg->execmsg.mn_ReplyPort = port;

    	    	LOCK_GDI
		XCALL(XSync, GetSysDisplay(), FALSE);
    	    	UNLOCK_GDI
		
		PutMsg(XSD(cl)->gditask_notify_port, (struct Message *)msg);

		/* Wait for result */
		WaitPort(port);		
		GetMsg(port);

    	    	kprintf("NOTY_MAPWINDOW request returned\n");		
    	    #endif
	    		
	    	gcval.plane_mask = AllPlanes;
	    	gcval.graphics_exposures = False;
		
    	    	LOCK_GDI	 
	    	data->gc = XCALL(XCreateGC, data->display, DRAWABLE(data),
		    	    	     GCPlaneMask | GCGraphicsExposures, &gcval);
    	    	UNLOCK_GDI	
			
	    	if (data->gc)
		{
		    ok = TRUE;
		    
    	    	#if GDISOFTMOUSE
		    init_empty_cursor(DRAWABLE(data), data->gc, XSD(cl));
    	    	#endif	    
		}	    
		else
		{
		    ok = FALSE;
		}
		
	    }
	    else
	    {
	    	ok = FALSE;
	    } /* if (msgport created && msg allocated) */

	    if (NULL != msg)
	    	FreeMem(msg, sizeof (*msg));
		
	    if (NULL != port)
	    	DeleteMsgPort(port);
		
	    
	}
	else
	{
	    ok = FALSE;
	} /* if (Xwindow created) */
		
    	if (!ok)
    	{
    
            OOP_MethodID disp_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
	    
    	    OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);
	
	    o = NULL;
    	}

#endif
    } /* if (object allocated by superclass) */

    ReturnPtr("GDIGfx.OnBitMap::New()", OOP_Object *, o);
}

/****************************************************************************************/

VOID GDIOnBM__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    
    EnterFunc(bug("GDIGfx.BitMap::Dispose()\n"));
    
    /* Someone is trying to dispose the framefuffer. This should really
    never happen in AROS. */
    
    //CCALL(raise, 19);
    
/*  if (data->gc)
    {
    	LOCK_GDI
    	XCALL(XFreeGC, data->display, data->gc);
    	UNLOCK_GDI	
    }

    if (DRAWABLE(data))
    {
	struct MsgPort    *port;
	struct notify_msg *msg;

	port = CreateMsgPort();
	msg = AllocMem(sizeof (*msg), MEMF_PUBLIC | MEMF_CLEAR);

	if (NULL == port || NULL == msg)
	{
	    kprintf("COULD NOT CREATE PORT OR ALLOCATE MEM IN onbitmap_dispose()\n");
    	    //CCALL(raise, 19);
	}
	
	msg->notify_type = NOTY_WINDISPOSE;
   	msg->xdisplay = GetSysDisplay();
	msg->xwindow = DRAWABLE(data);
	msg->masterxwindow = MASTERWIN(data);
	msg->execmsg.mn_ReplyPort = port;
	
	PutMsg(XSD(cl)->gditask_notify_port, (struct Message *)msg);
	WaitPort(port);
	
	GetMsg(port);
	
	FreeMem(msg, sizeof (*msg));
	DeleteMsgPort(port);

    	LOCK_GDI	
    	XCALL(XDestroyWindow,  GetSysDisplay(), DRAWABLE(data));
	XCALL(XFlush,  GetSysDisplay() );
    	UNLOCK_GDI		
    }

#if ADJUST_XWIN_SIZE
    if (MASTERWIN(data))
    {
    	LOCK_GDI
        XCALL(XDestroyWindow,  GetSysDisplay(), MASTERWIN(data));
	XCALL(XFlush,  GetSysDisplay() );
    	UNLOCK_GDI
    }
#endif

    if (data->flags & BMDF_COLORMAP_ALLOCED)
    {
    	LOCK_GDI
	XCALL(XFreeColormap, GetSysDisplay(), data->colmap);
    	UNLOCK_GDI
    }
    */
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
