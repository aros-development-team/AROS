/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Bitmap class for X11 hidd.
    Lang: English.
*/


#include <stdlib.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

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

#define DEBUG 0
#include <aros/debug.h>

#include "x11gfx_intern.h"
#include "x11.h"

#include "bitmap.h"

/****************************************************************************************/

static OOP_AttrBase HiddBitMapAttrBase;
OOP_AttrBase HiddPixFmtAttrBase;
static OOP_AttrBase HiddX11GfxAB;
static OOP_AttrBase HiddX11BitMapAB;

/****************************************************************************************/

static struct OOP_ABDescr attrbases[] = 
{
    { IID_Hidd_BitMap	, &HiddBitMapAttrBase 	},
    { IID_Hidd_PixFmt	, &HiddPixFmtAttrBase 	},
    /* Private bases */
    { IID_Hidd_X11Gfx	, &HiddX11GfxAB	    	},
    { IID_Hidd_X11BitMap, &HiddX11BitMapAB  	},
    { NULL  	    	, NULL      	    	}
};

/****************************************************************************************/

/* Macro trick to reuse code between offscreen and onscreen bitmap hidd
(bitmap_common.c) */

#define DRAWABLE(data)  (data)->drawable.xwindow

#if ADJUST_XWIN_SIZE
#define MASTERWIN(data) (data)->masterxwindow
#else
#define MASTERWIN(data) DRAWABLE(data)
#endif

#define MNAME(x) onbitmap_ ## x

/****************************************************************************************/

/* !!! Include methods whose implementation is eqaul for windows and pixmaps
 (except the DRAWABLE) */

#include "bitmap_common.c"

#if X11SOFTMOUSE
static void init_empty_cursor(Window w, GC c, struct x11_staticdata *xsd);
#endif
static Pixmap init_icon(Display *d, Window w, Colormap cm, LONG depth,
    	    	    	struct x11_staticdata *xsd);

/****************************************************************************************/

static OOP_Object *onbitmap_new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    BOOL ok = TRUE;
    
    EnterFunc(bug("X11Gfx.OnBitMap::New()\n"));
    
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
    	struct bitmap_data  	*data;
	Window      	    	 rootwin;	
        IPTR 	    	    	 width, height, depth;
	XSetWindowAttributes 	 winattr;
	int 	    	    	 visualclass;
	unsigned long 	    	 valuemask;
	
        data = OOP_INST_DATA(cl, o);
	
	/* clear all data  */
        memset(data, 0, sizeof(struct bitmap_data));
	
	/* Get some info passed to us by the x11gfxhidd class */
	data->display = (Display *)GetTagData(aHidd_X11Gfx_SysDisplay, 0, msg->attrList);
	data->screen  =            GetTagData(aHidd_X11Gfx_SysScreen,  0, msg->attrList);
	data->cursor  = (Cursor)   GetTagData(aHidd_X11Gfx_SysCursor,  0, msg->attrList);
	data->colmap  = (Colormap) GetTagData(aHidd_X11Gfx_ColorMap,   0, msg->attrList);

	/* stegerg*/

	visualclass   =		   GetTagData(aHidd_X11Gfx_VisualClass, TrueColor, msg->attrList);
		
        if ( visualclass == PseudoColor)
	{
	    Colormap cm;
	    
    	    LOCK_X11
	    cm = XCreateColormap(GetSysDisplay(),
				 RootWindow(GetSysDisplay(), GetSysScreen()),
				 XSD(cl)->vi.visual,
				 AllocAll);				 
    	    UNLOCK_X11
	    
	    if (cm)
	    {
	        data->colmap = cm;
		data->flags |= BMDF_COLORMAP_ALLOCED;
	    }
	}
	
	/* end stegerg */
	
	/* Get attr values */
	OOP_GetAttr(o, aHidd_BitMap_Width,  &width);
	OOP_GetAttr(o, aHidd_BitMap_Height, &height);

	/* Open an X window to be used for viewing */
	    
	D(bug("Displayable bitmap\n"));
	    
	/* Listen for all sorts of events */
	winattr.event_mask = 0;
	/* Mouse buttons .. */
	winattr.event_mask |= ButtonPressMask | ButtonReleaseMask;
	/* Mouse movement .. */
	winattr.event_mask |= PointerMotionMask;
	/* Key press & release .. */
	winattr.event_mask |= KeyPressMask | KeyReleaseMask;
	    
	/* We must allways have this one */
	winattr.event_mask |= StructureNotifyMask;
	winattr.event_mask |= SubstructureNotifyMask;
	
	winattr.event_mask |= FocusChangeMask;
	    
	/* Use backing store for now. (Uses lots of mem) */
	winattr.backing_store = Always;

    	LOCK_X11	
    
	winattr.cursor = GetSysCursor();
	winattr.save_under = True;
	
	winattr.background_pixel = WhitePixel(GetSysDisplay(), GetSysScreen());
	rootwin = DefaultRootWindow (GetSysDisplay());
	D(bug("Creating XWindow: root win=%p\n", rootwin));
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

	    if (data->flags & BMDF_COLORMAP_ALLOCED)
	    {
		winattr.colormap = data->colmap;
		valuemask |= CWColormap;
	    }
	    
	    MASTERWIN(data) = XCreateWindow( GetSysDisplay(),
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

	DRAWABLE(data) = XCreateWindow( GetSysDisplay(),
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
    	UNLOCK_X11	    

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

    	    LOCK_X11
	    
	    XStoreName   (GetSysDisplay(), MASTERWIN(data), "AROS");
	    XSetIconName (GetSysDisplay(), MASTERWIN(data), "AROS Screen");
		    
    	#if !ADJUST_XWIN_SIZE
	    sizehint.flags      = PMinSize | PMaxSize;
	    sizehint.min_width  = width;
	    sizehint.min_height = height;
	    sizehint.max_width  = width;
	    sizehint.max_height = height;
	    
	    XSetWMNormalHints (GetSysDisplay(), MASTERWIN(data), &sizehint);
    	#endif
	    
	    XSetWMProtocols (GetSysDisplay(), MASTERWIN(data), &XSD(cl)->delete_win_atom, 1);

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

		XSetWMHints(GetSysDisplay(), MASTERWIN(data), &hints);
	    }
	    

	    D(bug("Calling XMapRaised\n"));

/*
  stegerg: XMapRaised is now called inside the X11 task when getting
           the NOTY_MAPWINDOW message, otherwise the X11 task can
	   get a "dead" MapNotify event:
	   
	   XCreateWindow is called here on the app task context.
	   If we also call XMapRaised here then the X11 task might
	   get the MapNotify event before he got the NOTY_WINCREATE
	   message sent from here (see below). So the X11 task
	   would not know about our window and therefore ignore
	   the MapNotify event from X.
	   
	   This caused the freezes which sometimes happened during
	   startup when the Workbench screen was opened.
	   
	   //XMapRaised (GetSysDisplay(), DRAWABLE(data));
*/

    	    UNLOCK_X11	 
	       
	    /* Now we need to get some message from the X11 task about when
	       the window has been mapped (ie. MapWindow event).
	       This is because we cannot render into the window until the
	       it has been mapped.kfind &
	    */

	    /* Create X11 GC */
	    
	    port = CreateMsgPort();
	    msg = AllocMem(sizeof (*msg), MEMF_PUBLIC | MEMF_CLEAR);

	    if (NULL != port && NULL != msg)
	    {
	    	XGCValues gcval;

		/* Send a message to the x11 task that the window has been created */
		
		msg->notify_type = NOTY_WINCREATE;
		msg->xdisplay = GetSysDisplay();
		msg->xwindow = DRAWABLE(data);
		msg->masterxwindow = MASTERWIN(data);
		msg->bmobj = o;
		msg->execmsg.mn_ReplyPort = port;
		
    	    	LOCK_X11
		XSync(GetSysDisplay(), FALSE);
    	    	UNLOCK_X11

		PutMsg(XSD(cl)->x11task_notify_port, (struct Message *)msg);
				
		/* Wait for the reply, so we are sure that the x11 task
		   has got it */

		WaitPort(port);
		GetMsg(port);
		
		/* Send a message to the X11 task to ask when the window has been mapped */
		
   		msg->xdisplay = GetSysDisplay();
		msg->xwindow = DRAWABLE(data);
		msg->masterxwindow = MASTERWIN(data);
		msg->notify_type = NOTY_MAPWINDOW;
		msg->execmsg.mn_ReplyPort = port;

    	    	LOCK_X11
		XSync(GetSysDisplay(), FALSE);
    	    	UNLOCK_X11
		
		PutMsg(XSD(cl)->x11task_notify_port, (struct Message *)msg);

		/* Wait for result */
		WaitPort(port);		
		GetMsg(port);

    	    	kprintf("NOTY_MAPWINDOW request returned\n");		
		
	    	gcval.plane_mask = AllPlanes;
	    	gcval.graphics_exposures = False;
		
    	    	LOCK_X11	 
	    	data->gc = XCreateGC(data->display, DRAWABLE(data),
		    	    	     GCPlaneMask | GCGraphicsExposures, &gcval);
    	    	UNLOCK_X11	
			
	    	if (data->gc)
		{
		    ok = TRUE;
		    
    	    	#if X11SOFTMOUSE
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


    } /* if (object allocated by superclass) */

    ReturnPtr("X11Gfx.OnBitMap::New()", OOP_Object *, o);
}

/****************************************************************************************/

static VOID onbitmap_dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    
    EnterFunc(bug("X11Gfx.BitMap::Dispose()\n"));
    
    /* Someone is trying to dispose the framefuffer. This should really
    never happen in AROS. */
    
    //kill(getpid(), 19);
    
    if (data->gc)
    {
    	LOCK_X11
    	XFreeGC(data->display, data->gc);
    	UNLOCK_X11	
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
    	    //kill(getpid(), 19);
	}
	
	msg->notify_type = NOTY_WINDISPOSE;
   	msg->xdisplay = GetSysDisplay();
	msg->xwindow = DRAWABLE(data);
	msg->masterxwindow = MASTERWIN(data);
	msg->execmsg.mn_ReplyPort = port;
	
	PutMsg(XSD(cl)->x11task_notify_port, (struct Message *)msg);
	WaitPort(port);
	
	GetMsg(port);
	
	FreeMem(msg, sizeof (*msg));
	DeleteMsgPort(port);

    	LOCK_X11	
    	XDestroyWindow( GetSysDisplay(), DRAWABLE(data));
	XFlush( GetSysDisplay() );
    	UNLOCK_X11		
    }

#if ADJUST_XWIN_SIZE
    if (MASTERWIN(data))
    {
    	LOCK_X11
        XDestroyWindow( GetSysDisplay(), MASTERWIN(data));
	XFlush( GetSysDisplay() );
    	UNLOCK_X11
    }
#endif

    if (data->flags & BMDF_COLORMAP_ALLOCED)
    {
    	LOCK_X11
	XFreeColormap(GetSysDisplay(), data->colmap);
    	UNLOCK_X11
    }
    
    OOP_DoSuperMethod(cl, o, msg);
    
    ReturnVoid("X11Gfx.BitMap::Dispose");
}

/****************************************************************************************/

static VOID onbitmap_clear(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_Clear *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    ULONG   	    	width, height;
    
    XSetWindowAttributes winattr;
        
    /* Get width & height from bitmap */
  
    OOP_GetAttr(o, aHidd_BitMap_Width,  &width);
    OOP_GetAttr(o, aHidd_BitMap_Height, &height);
    
    winattr.background_pixel = GC_BG(msg->gc);

    LOCK_X11    
    XChangeWindowAttributes(data->display, DRAWABLE(data),
    	    	    	    CWBackPixel, &winattr);
    
    XClearArea (data->display, DRAWABLE(data),
	    0, 0,
	    width, height,
	    FALSE);
    
    XFlush(data->display);
    UNLOCK_X11            
}

/****************************************************************************************/

#undef XSD
#define XSD(cl) xsd

#define NUM_ROOT_METHODS 4

#if USE_X11_DRAWFUNCS
#   define NUM_BITMAP_METHODS 13
#else
#   define NUM_BITMAP_METHODS 11
#endif

/****************************************************************************************/

OOP_Class *init_onbmclass(struct x11_staticdata *xsd)
{
    struct OOP_MethodDescr root_descr[NUM_ROOT_METHODS + 1] =
    {
        {(IPTR (*)())MNAME(new)    , moRoot_New     },
        {(IPTR (*)())MNAME(dispose), moRoot_Dispose },

#if 0
        {(IPTR (*)())MNAME(set)	   , moRoot_Set     },
#endif
        {(IPTR (*)())MNAME(get)	   , moRoot_Get     },
        {NULL	    	    	   , 0UL    	    }
    };

    struct OOP_MethodDescr bitMap_descr[NUM_BITMAP_METHODS + 1] =
    {
        {(IPTR (*)())MNAME(setcolors)	    	, moHidd_BitMap_SetColors   	    },
    	{(IPTR (*)())MNAME(putpixel)	    	, moHidd_BitMap_PutPixel    	    },
    	{(IPTR (*)())MNAME(clear)   	    	, moHidd_BitMap_Clear	    	    },
    	{(IPTR (*)())MNAME(getpixel)	    	, moHidd_BitMap_GetPixel    	    },
    	{(IPTR (*)())MNAME(drawpixel)	    	, moHidd_BitMap_DrawPixel   	    },
    	{(IPTR (*)())MNAME(fillrect)	    	, moHidd_BitMap_FillRect    	    },
    	{(IPTR (*)())MNAME(getimage)	    	, moHidd_BitMap_GetImage    	    },
    	{(IPTR (*)())MNAME(putimage)	    	, moHidd_BitMap_PutImage    	    },
    	{(IPTR (*)())MNAME(blitcolorexpansion)	, moHidd_BitMap_BlitColorExpansion  },
    	{(IPTR (*)())MNAME(putimagelut)     	, moHidd_BitMap_PutImageLUT 	    },
    	{(IPTR (*)())MNAME(getimagelut)     	, moHidd_BitMap_GetImageLUT 	    },
#if USE_X11_DRAWFUNCS
    	{(IPTR (*)())MNAME(drawline)	    	, moHidd_BitMap_DrawLine    	    },
    	{(IPTR (*)())MNAME(drawellipse)     	, moHidd_BitMap_DrawEllipse 	    },
#endif
        {NULL	    	    	    	    	, 0UL	    	    	    	    }
    };
    
    struct OOP_InterfaceDescr ifdescr[] =
    {
        {root_descr 	, IID_Root          , NUM_ROOT_METHODS	},
        {bitMap_descr	, IID_Hidd_BitMap   , NUM_BITMAP_METHODS},
        {NULL	    	, NULL	    	    , 0     	    	}
    };

    OOP_AttrBase MetaAttrBase = OOP_ObtainAttrBase(IID_Meta);

    struct TagItem tags[] =
    {
        {aMeta_SuperID	    	, (IPTR) CLID_Hidd_BitMap   	    },
        {aMeta_InterfaceDescr	, (IPTR) ifdescr    	    	    },
        {aMeta_InstSize     	, (IPTR) sizeof(struct bitmap_data) },
        {TAG_DONE   	    	, 0UL	    	    	    	    }	
    };
    
    OOP_Class *cl = NULL;

    EnterFunc(bug("init_bitmapclass(xsd=%p)\n", xsd));
        
    D(bug("Metattrbase: %x\n", MetaAttrBase));

    if(MetaAttrBase)
    {
        D(bug("Got attrbase\n"));
       
        cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
        if(cl)
        {
            D(bug("BitMap class ok\n"));
	    
            xsd->onbmclass = cl;
            cl->UserData     = (APTR) xsd;
           
            /* Get attrbase for the BitMap interface */
	    if (OOP_ObtainAttrBases(attrbases))
            {	    
                OOP_AddClass(cl);
            }
            else
            {
    	    	#warning "The failure handlilg code is buggy. How do we know if the class was successfully added before removing it in free_onbcmlass ?"
                free_onbmclass( xsd );
                cl = NULL;
            }
        }
	
	/* We don't need this anymore */
	OOP_ReleaseAttrBase(IID_Meta);
	
    } /* if(MetaAttrBase) */

    ReturnPtr("init_onbmclass", OOP_Class *,  cl);
}

/****************************************************************************************/

void free_onbmclass(struct x11_staticdata *xsd)
{
    EnterFunc(bug("free_onbmclass(xsd=%p)\n", xsd));

    if(xsd)
    {    
        OOP_RemoveClass(xsd->onbmclass);
	
        if(xsd->onbmclass) OOP_DisposeObject((OOP_Object *) xsd->onbmclass);	
        xsd->onbmclass = NULL;
	
	OOP_ReleaseAttrBases(attrbases);
	
    }

    ReturnVoid("free_onbmclass");
}

/****************************************************************************************/

#if X11SOFTMOUSE

/****************************************************************************************/

static void init_empty_cursor(Window w, GC gc, struct x11_staticdata *xsd)
{
    Pixmap  p, mask; 
    int     width, height;
    
    width = height = 1;

    LOCK_X11    
    p = XCreatePixmap( xsd->display, w, width, height, 1);
    UNLOCK_X11    


    if (0 != p)
    {
    	LOCK_X11    
	mask = XCreatePixmap( xsd->display
		, w
		, width
		, height
		, 1
    	);
	XFlush(xsd->display);	
    	UNLOCK_X11    
	
	if (0 != mask)
	{
	    /* Define cursor for window */
	    XColor fg, bg;
	    Cursor c;
	    int    x, y;

    	    LOCK_X11
	    XSetForeground(xsd->display, gc, 0);
	    XSetFunction(xsd->display, gc, GXcopy);	    
    	#if 0	    
	    XFillRectangle(xsd->display, p, gc, 1, 1, 1, 1);
	    for (y = 0; y < height; y ++)
	    {
	    	for (x = 0; x < width; x ++)
		{
		    XDrawPoint(xsd->display, mask, gc, x, y);
		}
	    }
    	#endif	    
    	    UNLOCK_X11	
	        
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

    	    LOCK_X11
	    c = XCreatePixmapCursor(xsd->display, p, mask, &fg, &bg, 0, 0);
    	    UNLOCK_X11
	    	    
	    if (0 != c)
	    {
    	    	LOCK_X11	    
	    	XDefineCursor(xsd->display, w, c);
    	    	UNLOCK_X11		
	    }
	    
    	    LOCK_X11	    
	    XFreePixmap(xsd->display, mask);
    	    UNLOCK_X11	    
	}

    	LOCK_X11	
	XFreePixmap(xsd->display, p);
    	UNLOCK_X11
    }
  	
}

/****************************************************************************************/

#endif

/****************************************************************************************/

static Pixmap init_icon(Display *d, Window w, Colormap cm, LONG depth, struct x11_staticdata *xsd)
{
    #include "icon.h"

    #define SHIFT_PIX(pix, shift)	\
	(( (shift) < 0) ? (pix) >> (-shift) : (pix) << (shift) )
    
    Pixmap   icon = XCreatePixmap(d, w, width, height, depth);
    char    *data = header_data;
    LONG     red_shift, green_shift, blue_shift;
    GC       gc;
    
    red_shift   = 24 - xsd->red_shift;
    green_shift = 24 - xsd->green_shift;
    blue_shift  = 24 - xsd->blue_shift;
    
    if (icon)
    {
    	gc = XCreateGC(d, icon, 0, 0);
	
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
			
			if (XAllocColor(d, cm, &xcol))
			{
			    pixel = xcol.pixel;
			}
		    }
		    
    	    	    XSetForeground(d, gc, pixel);
		    XDrawPoint(d, icon, gc, x, y);
		}
	    }
	    
	    XFreeGC(d, gc);
	    
	} /* if (gc) */
	
    } /* if (icon) */
    
    return icon;
}

/****************************************************************************************/
