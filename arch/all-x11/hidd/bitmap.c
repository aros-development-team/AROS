/*
    (C) 1997 AROS - The Amiga Research OS
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

#include <proto/oop.h>

#include <exec/memory.h>
#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <oop/oop.h>

#include <hidd/graphics.h>

#include "x11gfx_intern.h"

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>



#define IS_BM_ATTR(attr, idx) ( ( (idx) = (attr) - HiddBitMapAttrBase) < num_Hidd_BitMap_Attrs)

/* Class static data */
#undef X11GfxBase
#define X11GfxBase ((struct x11gfxbase *)cl->UserData)

#undef OOPBase
#define OOPBase (X11GfxBase->oopbase)

#undef SysBase
#define SysBase (X11GfxBase->sysbase)

#define PEN_BITS    4
#define NUM_COLORS  (1L << PEN_BITS)
#define PEN_MASK    (NUM_COLORS - 1)


#define GetSysDisplay() (data->sysdisplay)
#define GetSysScreen()  (data->sysscreen)
#define GetSysCursor()  (data->syscursor)

#define IS_BM_ATTR(attr, idx) ( ( (idx) = (attr) - HiddBitMapAttrBase) < num_Hidd_BitMap_Attrs)
static AttrBase HiddBitMapAttrBase = 0;

/*********** BitMap::New() *************************************/

/*
   Inits sysdisplay, sysscreen, colormap, etc.. */
static BOOL initx11stuff(struct bitmap_data *data)
{
/*    XColor fg, bg; */
    BOOL ok = TRUE;
    char *displayname;

    /* Try to get the display */
    if (!(displayname = getenv("DISPLAY")))
	displayname =":0.0";


    data->sysdisplay = XOpenDisplay(displayname);
    if (!data->sysdisplay)
    	ok = FALSE;
    else
    {
    	data->sysscreen = DefaultScreen( GetSysDisplay() );
	
	data->depth  = DisplayPlanes( GetSysDisplay(), GetSysScreen() );
	data->colmap = DefaultColormap( GetSysDisplay(), GetSysScreen() );
	
	
	data->maxpen = NUM_COLORS;
	
	/* Create cursor */
/*	data->syscursor = XCreateFontCursor( GetSysDisplay(), XC_top_left_arrow);
	XRecolorCursor( GetSysDisplay(), GetSysCursor(), &fg, &bg);
*/
    } /* if (Could get sysdisplay) */
    
    return ok;

}

static VOID cleanupx11stuff(struct bitmap_data *data)
{
    /* Do nothing for now */
    return;
}

static Object *bitmap_new(Class *cl, Object *o, struct pRoot_New *msg)
{
    BOOL ok = TRUE;
    
    EnterFunc(bug("X11Gfx.BitMap::New()\n"));
    
    o = (Object *)DoSuperMethod(cl, o, (Msg) msg);
    if (o)
    {
    	struct bitmap_data *data;
	
        IPTR width, height, depth, displayable;
	
        data = INST_DATA(cl, o);
	
	/* clear all data  */
        memset(data, 0, sizeof(struct bitmap_data));

	if (displayable)
	{
	    if (!initx11stuff(data))
	    	ok = FALSE;
	    else
	    {
	    	XSetWindowAttributes winattr;
		
		D(bug("Got sysdisplay\n"));
		
	
	    	/* Get attr values */
	    	GetAttr(o, aHidd_BitMap_Width,		&width);
	    	GetAttr(o, aHidd_BitMap_Height, 	&height);
	    	GetAttr(o, aHidd_BitMap_Depth,		&depth);
	    	GetAttr(o, aHidd_BitMap_Displayable,	&displayable);
	
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
	    
	    	/* Use backing store for now. (Uses lots of mem) */
	    	winattr.backing_store = Always;
	    
	    	winattr.cursor = GetSysCursor();
	    	winattr.save_under = True;
	    	winattr.background_pixel = WhitePixel(GetSysDisplay(), GetSysScreen());
	    
	    	D(bug("Creating XWindow\n"));
	    	data->xwindow = XCreateWindow( GetSysDisplay()
	    		, DefaultRootWindow (GetSysDisplay())
			, 0	/* leftedge 	*/
			, 0	/* topedge	*/
			, width
			, height
			, 0	/* BorderWidth	*/
			, DefaultDepth (GetSysDisplay(), GetSysScreen())
			, InputOutput
			, DefaultVisual (GetSysDisplay(), GetSysScreen())
			, CWBackingStore
		    		| CWCursor
		    		| CWSaveUnder
		   		| CWEventMask
		    		| CWBackPixel
			, &winattr
	   	 );
	    
	    	D(bug("Xwindow : %p\n", data->xwindow));
	   	if (data->xwindow)
	    	{
	            D(bug("Calling XMapRaised\n"));
	    	    XMapRaised (GetSysDisplay(), data->xwindow);

		    /* Wait for MapNotify (ie. for window to be displayed) */
		    for (;;)
		    {
		        XEvent e;
			XNextEvent(data->sysdisplay, &e);
			if (e.type == MapNotify)
			    break;
		    }
		    
		
	
	    	}
	    	else
	    	{
	             ok = FALSE;
	    	} /* if (Xwindow created) */
		
	    
	    } /* if (Could get sysdisplay) */
	}
	else
	{
	    /* Do nothing, as this is an offscreen bitmap handled by superclass */
	}

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

static VOID bitmap_dispose(Class *cl, Object *o, Msg msg)
{
    struct bitmap_data *data = INST_DATA(cl, o);
    EnterFunc(bug("X11Gfx.BitMap::Dispose()\n"));
    if (data->xwindow)
    {
    	XDestroyWindow( GetSysDisplay(), data->xwindow);
	XFlush( GetSysDisplay() );
    }
    
    cleanupx11stuff(data);
    
    
    DoSuperMethod(cl, o, msg);
    
    ReturnVoid("X11Gfx.BitMap::Dispose");
}

static BOOL bitmap_setcolors(Class *cl, Object *o, struct pHidd_BitMap_SetColors *msg)
{
    /* FixMe: Doesn't deallocate previously set colors */
    
    
    struct bitmap_data *data = INST_DATA(cl, o);
    ULONG xc_i, col_i;
    
    XColor xc;
    
    EnterFunc(bug("X11Gfx.BitMap::SetColors(num=%d, first=%d)\n",
    		msg->numColors, msg->firstColor));
    
    for ( xc_i = msg->firstColor, col_i = 0;
    		col_i < msg ->numColors; 
		xc_i ++, col_i ++ )
    {
	xc.red   = msg->colors[col_i].red;
	xc.green = msg->colors[col_i].green;
	xc.blue	 = msg->colors[col_i].blue;
	
	if (XAllocColor(data->sysdisplay, data->colmap, &xc))
	{
		D(bug("Successfully allocated color (%x, %x, %x)\n",
			xc.red, xc.green, xc.blue));
			
	    /* Remember the color */
	    data->hidd2x11cmap[xc_i] =xc.pixel;
        			
	}		
    }
    
    ReturnBool("X11Gfx.BitMap::SetColors",  TRUE);

}




/*** init_bitmapclass *********************************************************/

#undef X11GfxBase

#define NUM_ROOT_METHODS   3
#define NUM_BITMAP_METHODS 1


Class *init_bitmapclass(struct x11gfxbase *X11GfxBase)
{
    struct MethodDescr root_descr[NUM_ROOT_METHODS + 1] =
    {
        {(IPTR (*)())bitmap_new    , moRoot_New    },
        {(IPTR (*)())bitmap_dispose, moRoot_Dispose},
        {NULL, 0UL}
    };

    struct MethodDescr bitMap_descr[NUM_BITMAP_METHODS + 1] =
    {
        {(IPTR (*)())bitmap_setcolors, moHidd_BitMap_SetColors},
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

    EnterFunc(bug("init_bitmapclass(X11GfxBase=%p)\n", X11GfxBase));

    if(MetaAttrBase)
    {
        cl = NewObject(NULL, CLID_HiddMeta, tags);
        if(cl)
        {
            D(bug("BitMap class ok\n"));
            X11GfxBase->bitmapclass = cl;
            cl->UserData     = (APTR) X11GfxBase;
           
            /* Get attrbase for the BitMap interface */
            HiddBitMapAttrBase = ObtainAttrBase(IID_Hidd_BitMap);
            if(HiddBitMapAttrBase)
            {
                AddClass(cl);
            }
            else
            {
                free_bitmapclass(X11GfxBase);
                cl = NULL;
            }
        }
	
	/* We don't need this anymore */
	ReleaseAttrBase(IID_Meta);
    } /* if(MetaAttrBase) */

    ReturnPtr("init_bitmapclass", Class *,  cl);
}


/*** free_bitmapclass *********************************************************/

void free_bitmapclass(struct x11gfxbase *X11GfxBase)
{
    EnterFunc(bug("free_bitmapclass(X11GfxBase=%p)\n", X11GfxBase));

    if(X11GfxBase)
    {
        RemoveClass(X11GfxBase->bitmapclass);
        if(X11GfxBase->bitmapclass) DisposeObject((Object *) X11GfxBase->bitmapclass);
        X11GfxBase->bitmapclass = NULL;
	
        if(HiddBitMapAttrBase) ReleaseAttrBase(IID_Hidd_BitMap);
	HiddBitMapAttrBase = 0;
	
    }

    ReturnVoid("free_bitmapclass");
}
