/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: Bitmap class for X11 hidd.
    Lang: English.
*/

#define AROS_ALMOST_COMPATIBLE 1

#include <stdlib.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

#include <string.h>

#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/dos.h>

#include <exec/memory.h>
#include <exec/lists.h>

#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <oop/oop.h>

#include <hidd/graphics.h>

#include "x11gfx_intern.h"
#include "x11.h"

#include "bitmap.h"

static AttrBase HiddBitMapAttrBase = 0;
static AttrBase HiddX11GfxAB = 0;
static AttrBase HiddX11BitMapAB = 0;

static struct ABDescr attrbases[] = 
{
    { IID_Hidd_BitMap,		&HiddBitMapAttrBase },
    /* Private bases */
    { IID_Hidd_X11Gfx,		&HiddX11GfxAB	},
    { IID_Hidd_X11BitMap,	&HiddX11BitMapAB },
    { NULL, NULL }
};


/* Macro trick to reuse code between offscreen and onscreen bitmap hidd
(bitmap_common.c) */
#define DRAWABLE(data) (data)->drawable.xwindow

#define MNAME(x) onbitmap_ ## x

/* !!! Include methods whose implementation is eqaul for windows and pixmaps
 (except the DRAWABLE) */

#include "bitmap_common.c"

#define DEBUG 0
#include <aros/debug.h>
 

/*********** BitMap::New() *************************************/

static Object *onbitmap_new(Class *cl, Object *o, struct pRoot_New *msg)
{
    BOOL ok = TRUE;
    
    EnterFunc(bug("X11Gfx.BitMap::New()\n"));
    
    o = (Object *)DoSuperMethod(cl, o, (Msg) msg);
    if (o)
    {
    	struct bitmap_data *data;
	struct TagItem depth_tags[] = {
	    { aHidd_BitMap_Depth, 0 },
	    { TAG_DONE, 0 }
	};
	Window rootwin;
	
        IPTR width, height, depth;
	XSetWindowAttributes winattr;
	int visualclass;
	unsigned long valuemask;
	
        data = INST_DATA(cl, o);
	
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
LX11
	    cm = XCreateColormap(GetSysDisplay(),
				 RootWindow(GetSysDisplay(), GetSysScreen()),
				 XSD(cl)->vi.visual,
				 AllocAll);				 
UX11
	    if (cm)
	    {
	        data->colmap = cm;
		data->flags |= BMDF_COLORMAP_ALLOCED;
	    }
	}
	
	/* end stegerg */
	
	/* Get attr values */
	GetAttr(o, aHidd_BitMap_Width,		&width);
	GetAttr(o, aHidd_BitMap_Height, 	&height);
	GetAttr(o, aHidd_BitMap_Depth,		&depth);
	
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
	
	winattr.event_mask |= FocusChangeMask;
	    
	/* Use backing store for now. (Uses lots of mem) */
	winattr.backing_store = Always;

LX11	
    
	winattr.cursor = GetSysCursor();
	winattr.save_under = True;
	
	winattr.background_pixel = WhitePixel(GetSysDisplay(), GetSysScreen());
	rootwin = DefaultRootWindow (GetSysDisplay());
	D(bug("Creating XWindow: root win=%p\n", rootwin));
	depth = DefaultDepth(GetSysDisplay(), GetSysScreen());
	
	/* Update the depth to the one we use */
	depth_tags[0].ti_Data = depth;
	SetAttrs(o, depth_tags);
	
/* stegerg */
	valuemask = CWBackingStore
		    	| CWCursor
		    	| CWSaveUnder
		   	| CWEventMask
		    	| CWBackPixel;
	
	if (data->flags & BMDF_COLORMAP_ALLOCED)
	{
	    winattr.colormap = data->colmap;
	    valuemask |= CWColormap;
	}
/* end stegerg */
	
	DRAWABLE(data) = XCreateWindow( GetSysDisplay()
	    		, rootwin
			, 0	/* leftedge 	*/
			, 0	/* topedge	*/
			, width
			, height
			, 0	/* BorderWidth	*/
			, depth
			, InputOutput
			, DefaultVisual (GetSysDisplay(), GetSysScreen())
			, valuemask
			, &winattr
	   	 );
UX11	    
	D(bug("Xwindow : %p\n", DRAWABLE(data)));
	if (DRAWABLE(data))
	{
            XSizeHints 		sizehint;
	    struct MsgPort 	*port;	    
	    struct notify_msg 	*msg;

LX11
	    XStoreName   (GetSysDisplay(), DRAWABLE(data), "AROS");
	    XSetIconName (GetSysDisplay(), DRAWABLE(data), "AROS Screen");
		    
	    sizehint.flags      = PMinSize | PMaxSize;
	    sizehint.min_width  = width;
	    sizehint.min_height = height;
	    sizehint.max_width  = width;
	    sizehint.max_height = height;
	    XSetWMNormalHints (GetSysDisplay(), DRAWABLE(data), &sizehint);
	    
	    XSetWMProtocols (GetSysDisplay(), DRAWABLE(data), &XSD(cl)->delete_win_atom, 1);

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

UX11	    
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
		msg->bmobj = o;
		msg->execmsg.mn_ReplyPort = port;
		
		PutMsg(XSD(cl)->x11task_notify_port, (struct Message *)msg);
		
		
		/* Wait for the reply, so we are sure that the x11 task
		   has got it */

		WaitPort(port);
		GetMsg(port);
		

		/* Send a message to the X11 task to ask when the window has been mapped */
		
   		msg->xdisplay = GetSysDisplay();
		msg->xwindow = DRAWABLE(data);
		msg->notify_type = NOTY_MAPWINDOW;
		msg->execmsg.mn_ReplyPort = port;
		
		PutMsg(XSD(cl)->x11task_notify_port, (struct Message *)msg);

		/* Wait for result */
		WaitPort(port);
		
		GetMsg(port);
		
	    	gcval.plane_mask = AllPlanes;
	    	gcval.graphics_exposures = False;
LX11	 
	    	data->gc = XCreateGC( data->display
	 		, DRAWABLE(data)
			, GCPlaneMask | GCGraphicsExposures
			, &gcval
		    );

UX11		
	    	if (data->gc)
	    	{
		    	/* Set the bitmap pixel format in the superclass */

		    	if (!set_pixelformat(o, XSD(cl), DRAWABLE(data))) {
			    ok = FALSE;
			}
		    
	    	} else {
		    ok = FALSE;
		} /* if (gc created) */
		
		
	    } else {
	    	ok = FALSE;
	    } /* if (msgport created && msg allocated) */

	    if (NULL != msg)
	    	FreeMem(msg, sizeof (*msg));
		
	    if (NULL != port)
	    	DeleteMsgPort(port);
		
	    
	} else {
	    ok = FALSE;
	} /* if (Xwindow created) */
		
    	if (!ok)
    	{
    
            MethodID disp_mid = GetMethodID(IID_Root, moRoot_Dispose);
    	    CoerceMethod(cl, o, (Msg) &disp_mid);
	
	    o = NULL;
    	}


    } /* if (object allocated by superclass) */

    ReturnPtr("X11Gfx.BitMap::New()", Object *, o);
}


/**********  Bitmap::Dispose()  ***********************************/

static VOID onbitmap_dispose(Class *cl, Object *o, Msg msg)
{
    struct bitmap_data *data = INST_DATA(cl, o);
    EnterFunc(bug("X11Gfx.BitMap::Dispose()\n"));
    
    if (data->gc)
    {
LX11
    	XFreeGC(data->display, data->gc);
UX11	
    }

    if (DRAWABLE(data))
    {


	struct MsgPort *port;
	struct notify_msg *msg;


	port = CreateMsgPort();
	msg = AllocMem(sizeof (*msg), MEMF_PUBLIC | MEMF_CLEAR);
	if (NULL == port || NULL == msg) {
	    kprintf("COULD NOT CREATE PORT OR ALLOCATE MEM IN onbitmap_dispose()\n");
//	    kill(getpid(), 19);
	}
	
	msg->notify_type = NOTY_WINDISPOSE;
   	msg->xdisplay = GetSysDisplay();
	msg->xwindow = DRAWABLE(data);
	msg->execmsg.mn_ReplyPort = port;
	
	PutMsg(XSD(cl)->x11task_notify_port, (struct Message *)msg);
	WaitPort(port);
	
	GetMsg(port);
	
	FreeMem(msg, sizeof (*msg));
	DeleteMsgPort(port);

LX11	
	
    	XDestroyWindow( GetSysDisplay(), DRAWABLE(data));
	XFlush( GetSysDisplay() );
UX11	
	
    }

    if (data->flags & BMDF_COLORMAP_ALLOCED)
    {
LX11
	XFreeColormap(GetSysDisplay(), data->colmap);
UX11
    }
    
    DoSuperMethod(cl, o, msg);
    
    ReturnVoid("X11Gfx.BitMap::Dispose");
}

/*********  BitMap::Clear()  *************************************/
static VOID onbitmap_clear(Class *cl, Object *o, struct pHidd_BitMap_Clear *msg)
{
    ULONG width, height;
    struct bitmap_data *data = INST_DATA(cl, o);
    
    XSetWindowAttributes winattr;
    
    
    /* Get width & height from bitmap */
  
    GetAttr(o, aHidd_BitMap_Width,  &width);
    GetAttr(o, aHidd_BitMap_Height, &height);
    
    winattr.background_pixel = GC_BG(msg->gc);
LX11    
    XChangeWindowAttributes(data->display
    		, DRAWABLE(data)
		, CWBackPixel
		, &winattr);
    
    XClearArea (data->display, DRAWABLE(data),
	    0, 0,
	    width, height,
	    FALSE);
    
    XFlush(data->display);
UX11    
    return;
    
}




/*** init_onbmclass *********************************************************/

#undef XSD
#define XSD(cl) xsd

#define NUM_ROOT_METHODS   4

#if USE_X11_DRAWFUNCS
#   define NUM_BITMAP_METHODS 14
#else
#   define NUM_BITMAP_METHODS 12
#endif


Class *init_onbmclass(struct x11_staticdata *xsd)
{
    struct MethodDescr root_descr[NUM_ROOT_METHODS + 1] =
    {
        {(IPTR (*)())MNAME(new)    , moRoot_New    },
        {(IPTR (*)())MNAME(dispose), moRoot_Dispose},

#if 0
        {(IPTR (*)())MNAME(set)	   , moRoot_Set},
#endif
        {(IPTR (*)())MNAME(get)	   , moRoot_Get},
        {NULL, 0UL}
    };

    struct MethodDescr bitMap_descr[NUM_BITMAP_METHODS + 1] =
    {
        {(IPTR (*)())MNAME(setcolors),		moHidd_BitMap_SetColors},
    	{(IPTR (*)())MNAME(putpixel),		moHidd_BitMap_PutPixel},
    	{(IPTR (*)())MNAME(clear),		moHidd_BitMap_Clear},
    	{(IPTR (*)())MNAME(getpixel),		moHidd_BitMap_GetPixel},
    	{(IPTR (*)())MNAME(drawpixel),		moHidd_BitMap_DrawPixel},
    	{(IPTR (*)())MNAME(fillrect),		moHidd_BitMap_FillRect},
    	{(IPTR (*)())MNAME(copybox),		moHidd_BitMap_CopyBox},
    	{(IPTR (*)())MNAME(getimage),		moHidd_BitMap_GetImage},
    	{(IPTR (*)())MNAME(putimage),		moHidd_BitMap_PutImage},
    	{(IPTR (*)())MNAME(blitcolorexpansion),	moHidd_BitMap_BlitColorExpansion},
    	{(IPTR (*)())MNAME(putimagelut),	moHidd_BitMap_PutImageLUT},
    	{(IPTR (*)())MNAME(getimagelut),	moHidd_BitMap_GetImageLUT},
#if USE_X11_DRAWFUNCS
    	{(IPTR (*)())MNAME(drawline),		moHidd_BitMap_DrawLine},
    	{(IPTR (*)())MNAME(drawellipse),	moHidd_BitMap_DrawEllipse},
#endif


        {NULL, 0UL}
    };
    
    struct InterfaceDescr ifdescr[] =
    {
        {root_descr,    IID_Root       , NUM_ROOT_METHODS},
        {bitMap_descr,  IID_Hidd_BitMap, NUM_BITMAP_METHODS},
        {NULL, NULL, 0}
    };

    AttrBase MetaAttrBase = ObtainAttrBase(IID_Meta);

    struct TagItem tags[] =
    {
        {aMeta_SuperID,        (IPTR) CLID_Hidd_BitMap},
        {aMeta_InterfaceDescr, (IPTR) ifdescr},
        {aMeta_InstSize,       (IPTR) sizeof(struct bitmap_data)},
        {TAG_DONE, 0UL}
    };
    
    Class *cl = NULL;

    EnterFunc(bug("init_bitmapclass(xsd=%p)\n", xsd));
    
    
    D(bug("Metattrbase: %x\n", MetaAttrBase));


    if(MetaAttrBase)
    {
       D(bug("Got attrbase\n"));
       
/*    for (;;) {cl = cl; } */
        cl = NewObject(NULL, CLID_HiddMeta, tags);
        if(cl)
        {
            D(bug("BitMap class ok\n"));
            xsd->onbmclass = cl;
            cl->UserData     = (APTR) xsd;
           
            /* Get attrbase for the BitMap interface */
	    if (ObtainAttrBases(attrbases))
            {
	    
                AddClass(cl);
            }
            else
            {
#warning The failure handlilg code is buggy. How do we know if the class was successfully added before removing it in free_onbcmlass ?
                free_onbmclass( xsd );
                cl = NULL;
            }
        }
	
	/* We don't need this anymore */
	ReleaseAttrBase(IID_Meta);
    } /* if(MetaAttrBase) */

    ReturnPtr("init_onbmclass", Class *,  cl);
}


/*** free_bitmapclass *********************************************************/

void free_onbmclass(struct x11_staticdata *xsd)
{
    EnterFunc(bug("free_onbmclass(xsd=%p)\n", xsd));

    if(xsd)
    {
    
        RemoveClass(xsd->onbmclass);
        if(xsd->onbmclass) DisposeObject((Object *) xsd->onbmclass);
	
        xsd->onbmclass = NULL;
	
	ReleaseAttrBases(attrbases);
	
    }

    ReturnVoid("free_onbmclass");
}
