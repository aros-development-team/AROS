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
#include <proto/utility.h>

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

#undef UtilityBase
#define UtilityBase (X11GfxBase->utilitybase)



#define GetSysDisplay() (data->display)
#define GetSysScreen()  (data->screen)
#define GetSysCursor()  (data->cursor)

#define IS_BM_ATTR(attr, idx) ( ( (idx) = (attr) - HiddBitMapAttrBase) < num_Hidd_BitMap_Attrs)
static AttrBase HiddBitMapAttrBase = 0;
static AttrBase HiddX11GfxAB = 0;
static AttrBase HiddX11OsbmAB = 0;

static struct abdescr attrbases[] = 
{
    { IID_Hidd_BitMap,	&HiddBitMapAttrBase },
    /* Private bases */
    { IID_Hidd_X11Gfx,	&HiddX11GfxAB	},
    { IID_Hidd_X11Osbm,	&HiddX11OsbmAB },
    { NULL, NULL }
};

/*********** BitMap::New() *************************************/
struct bitmap_data
{
    Window 	xwindow;
    Cursor	cursor;
    long 	maxpen;
    unsigned long sysplanemask;
    Colormap	colmap;
    int		depth;
    long	*hidd2x11cmap;
    GC 		gc;	/* !!! This is an X11 GC, NOT a HIDD gc */
    Display	*display;
    int		screen;
    
};





static Object *bitmap_new(Class *cl, Object *o, struct pRoot_New *msg)
{
    BOOL ok = TRUE;
    
    EnterFunc(bug("X11Gfx.BitMap::New()\n"));
    
    o = (Object *)DoSuperMethod(cl, o, (Msg) msg);
    if (o)
    {
    	struct bitmap_data *data;
	
        IPTR width, height, depth;
	XSetWindowAttributes winattr;
	
        data = INST_DATA(cl, o);
	
	/* clear all data  */
        memset(data, 0, sizeof(struct bitmap_data));
	
	/* Get some info passed to us by the x11gfxhidd class */
	data->display = (Display *)GetTagData(aHidd_X11Gfx_SysDisplay, 0, msg->attrList);
	data->screen  = GetTagData(aHidd_X11Gfx_SysScreen, 0, msg->attrList);
	data->hidd2x11cmap = (long *)GetTagData(aHidd_X11Gfx_Hidd2X11CMap, 0, msg->attrList);
	data->cursor = (Cursor)GetTagData(aHidd_X11Gfx_SysCursor, 0, msg->attrList);
	data->colmap = (Colormap)GetTagData(aHidd_X11Gfx_ColorMap, 0, msg->attrList);
		
	
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
	    XGCValues gcval;
		    
	    D(bug("Calling XMapRaised\n"));
	    XMapRaised (GetSysDisplay(), data->xwindow);

	    /* Wait for MapNotify (ie. for window to be displayed) */
	    for (;;)
	    {
		XEvent e;
		XNextEvent(data->display, &e);
		if (e.type == MapNotify)
		    break;
	    }
		
		
	    /* Create X11 GC */
	 
	    gcval.plane_mask = 0xFFFFFFFF; /*BlackPixel(data->display, data->screen); */ /* bm_data->sysplanemask; */
	    gcval.graphics_exposures = True;
	 
	    data->gc = XCreateGC( data->display
	 		, DefaultRootWindow( data->display )
			, GCPlaneMask | GCGraphicsExposures
			, &gcval
		    );
	    if (data->gc)
	    {
	    }
	    else
	    {
		ok = FALSE;
	    }
	
	}
	else
	{
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

static VOID bitmap_dispose(Class *cl, Object *o, Msg msg)
{
    struct bitmap_data *data = INST_DATA(cl, o);
    EnterFunc(bug("X11Gfx.BitMap::Dispose()\n"));
    if (data->gc)
    {
    	XFreeGC(data->display, data->gc);
    }
    if (data->xwindow)
    {
    	XDestroyWindow( GetSysDisplay(), data->xwindow);
	XFlush( GetSysDisplay() );
    }
    
    
    
    DoSuperMethod(cl, o, msg);
    
    ReturnVoid("X11Gfx.BitMap::Dispose");
}
/**************  BitMap::Set()  *********************************/
static VOID bitmap_set(Class *cl, Object *o, struct pRoot_Set *msg)
{
    struct bitmap_data *data = INST_DATA(cl, o);
    struct TagItem *tag, *tstate;
    ULONG idx;
    
    tstate = msg->attrList;
    while((tag = NextTagItem(&tstate)))
    {
        if(IS_BM_ATTR(tag->ti_Tag, idx))
        {
            switch(idx)
            {
                case aoHidd_BitMap_Foreground :
		    /* Set X GC color */
		    XSetForeground(data->display, data->gc, data->hidd2x11cmap[tag->ti_Data]);
		    break;
		    
                case aoHidd_BitMap_Background :
		    XSetBackground(data->display, data->gc, data->hidd2x11cmap[tag->ti_Data]);
		    break;
            }
        }
    }
    
    /* Let supermethod take care of other attrs */
    DoSuperMethod(cl, o, (Msg)msg);
    
    return;
}
static BOOL bitmap_setcolors(Class *cl, Object *o, struct pHidd_BitMap_SetColors *msg)
{
#warning Does not deallocate previously allocated colors
    
    
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
	
	if (XAllocColor(data->display, data->colmap, &xc))
	{
		D(bug("Successfully allocated color (%x, %x, %x)\n",
			xc.red, xc.green, xc.blue));
			
	    /* Remember the color */
	    data->hidd2x11cmap[xc_i] =xc.pixel;
        			
	}		
    }
    
    
    ReturnBool("X11Gfx.BitMap::SetColors",  TRUE);

}
/*********  BitMap::PutPixel()  ***************************/

static VOID bitmap_putpixel(Class *cl, Object *o, struct pHidd_BitMap_PutPixel *msg)
{
     struct bitmap_data *data = INST_DATA(cl, o);
     ULONG old_fg;
     
     GetAttr(o, aHidd_BitMap_Foreground, &old_fg);
     
     XSetForeground(data->display, data->gc, data->hidd2x11cmap[msg->val]);
     XDrawPoint(data->display, data->xwindow, data->gc, msg->x, msg->y);
     
     /* Reset GC to old value */
     XSetForeground(data->display, data->gc, data->hidd2x11cmap[old_fg]);
     
     XFlush(data->display);
     return;
}

/*********  BitMap::GetPixel()  *********************************/
static ULONG bitmap_getpixel(Class *cl, Object *o, struct pHidd_BitMap_GetPixel *msg)
{
    ULONG pixel, i;
    struct bitmap_data *data = INST_DATA(cl, o);
    
    XImage *image;

    
    image = XGetImage(data->display
    	, data->xwindow
	, msg->x, msg->y
	, 1, 1
	, AllPlanes
	, ZPixmap);
    
    if (!image)
    	return -1L;
	
    pixel = XGetPixel(image, 0, 0);
    XFree(image);
    
    /* Get pen number from colortab */

    for (i = 0; i < 256; i ++)
    {
        if (pixel == data->hidd2x11cmap[i])
    	    return i;
    }
    
    return -1L;
    
    
}

/*********  BitMap::DrawPixel() ************************************/
static ULONG bitmap_drawpixel(Class *cl, Object *o, struct pHidd_BitMap_DrawPixel *msg)
{

    struct bitmap_data *data = INST_DATA(cl, o);
    

    /* Foreground pen allready set in X GC. Note, though, that a
       call to WritePixelDirect may owerwrite th GC's pen  */
    XDrawPoint(data->display, data->xwindow, data->gc, msg->x, msg->y);
    XFlush(data->display);
    
    return 0;

    
}
/*********  BitMap::FillRect()  *************************************/
static VOID bitmap_fillrect(Class *cl, Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    struct bitmap_data *data = INST_DATA(cl, o);
    ULONG mode;
    EnterFunc(bug("X11Gfx.BitMap::FillRect(%d,%d,%d,%d)\n",
    	msg->minX, msg->minY, msg->maxX, msg->maxY));
	
    GetAttr(o, aHidd_BitMap_DrawMode, &mode);
    
    D(bug("Drawmode: %d\n", mode));
    
    
    if (mode == vHIDD_GC_DrawMode_Copy)
    {
    	
	XFillRectangle(data->display
		, data->xwindow
		, data->gc
		, msg->minX
		, msg->minY
		, msg->maxX - msg->minX + 1
		, msg->maxY - msg->minY + 1
	);
    }
    else
    {
    	XImage *image;
	WORD x, y, width, height;
	ULONG src;
	GetAttr(o, aHidd_BitMap_Foreground, &src);
	
	width  = msg->maxX - msg->minX + 1;
	height = msg->maxY - msg->minY + 1;
    	/* Special drawmode */
	image = XGetImage(data->display
		, data->xwindow
		, msg->minX, msg->minY
		, width, height
		, AllPlanes
		, ZPixmap);
		
	if (!image)
	    ReturnVoid("X11Gfx.BitMap::FillRect(Couldn't get XImage)");
	    
	for (y = 0; y < width; y ++)
	{
	    for (x = 0; x < height; x ++)
	    {
	        ULONG dest;
		ULONG val = 0UL;
		
		dest = map_x11_to_hidd(data->hidd2x11cmap, XGetPixel(image, x, y));
		    
		/* Apply drawmodes to pixel */
	   	if(mode & 1) val = ( src &  dest);
	   	if(mode & 2) val = ( src & ~dest) | val;
	   	if(mode & 4) val = (~src &  dest) | val;
	   	if(mode & 8) val = (~src & ~dest) | val;
		
		
		XPutPixel(image, x, y, data->hidd2x11cmap[val]);

	    }
	    
	}  
	D(bug("Putting image at (%d, %d), w=%d, h=%d\n",
		msg->minX, msg->minY, width, height ));
		
	/* Put image back into display */
	XPutImage(data->display
    		, data->xwindow
		, data->gc
		, image
		, 0, 0
		, msg->minX, msg->minY
		, width, height);
	    
	D(bug("image put\n"));

	XFree(image);
	D(bug("image destroyed\n"));
    }
   
    D(bug("Flushing\n"));

    XFlush(data->display);
    ReturnVoid("X11Gfx.BitMap::FillRect");
    

}

/*********  BitMap::CopyBox()  *************************************/
static VOID bitmap_copybox(Class *cl, Object *o, struct pHidd_BitMap_CopyBox *msg)
{
    ULONG mode;
    struct bitmap_data *data = INST_DATA(cl, o);
    GetAttr(o, aHidd_BitMap_DrawMode, &mode);
    EnterFunc(bug("X11Gfx.BitMap::CopyBox( %d,%d to %d,%d of dim %d,%d\n",
    	msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height));

#warning Does not handle copying between different windows.

    if (mode == vHIDD_GC_DrawMode_Copy) /* Optimize this drawmode */
    {
    	XCopyArea(data->display
    		, data->xwindow	/* src	*/
		, data->xwindow /* dest */
		, data->gc
		, msg->srcX
		, msg->srcY
		, msg->width
		, msg->height
		, msg->destX
		, msg->destY
    	);
    
    }
    else
    {
	XImage *src_image, *dst_image;
	WORD x, y;
	
	src_image = XGetImage(data->display
		, data->xwindow
		, msg->srcX, msg->srcY
		, msg->width, msg->height
		, AllPlanes
		, ZPixmap);
		
	if (!src_image)
	    ReturnVoid("X11Gfx.BitMap::CopyBox(Couldn't get source XImage)");
	    
	dst_image = XGetImage(data->display
		, data->xwindow
		, msg->destX, msg->destY
		, msg->width, msg->height
		, AllPlanes
		, ZPixmap);
		
	if (!dst_image)
	{
	    XDestroyImage(src_image);
	    ReturnVoid("X11Gfx.BitMap::CopyBox(Couldn't get destination XImage)");
	}
     	   
	for (y = 0; y < msg->height; y ++)
	{
	    for (x = 0; x < msg->width; x ++)
	    {
    		/* Drawmodes make things more complicated */
		ULONG src;
		ULONG dest;
		ULONG val = 0;
		
		src  = map_x11_to_hidd(data->hidd2x11cmap, XGetPixel(src_image, x, y));
		dest = map_x11_to_hidd(data->hidd2x11cmap, XGetPixel(dst_image, x, y));
		    
		/* Apply drawmodes to pixel */
	   	if(mode & 1) val = ( src &  dest);
	   	if(mode & 2) val = ( src & ~dest) | val;
	   	if(mode & 4) val = (~src &  dest) | val;
	   	if(mode & 8) val = (~src & ~dest) | val;
		
		XPutPixel(dst_image, x, y, data->hidd2x11cmap[val]);
	    }
	}
	/* Put image back into display */
	XPutImage(data->display
    		, data->xwindow
		, data->gc
		, dst_image
		, 0, 0
		, msg->destX, msg->destY
		, msg->width, msg->height);
	

	XFree(src_image);
	XFree(dst_image);
	
    }
    
    
    XFlush(data->display);
    ReturnVoid("X11Gfx.BitMap::CopyBox");
}
/*********  BitMap::Clear()  *************************************/
static VOID bitmap_clear(Class *cl, Object *o, struct pHidd_BitMap_Clear *msg)
{
    ULONG width, height, bg;
    struct bitmap_data *data = INST_DATA(cl, o);
    
    XSetWindowAttributes winattr;
    
    GetAttr(o, aHidd_BitMap_Background, &bg);
    
    
    /* Get width & height from bitmap */
  
    GetAttr(o, aHidd_BitMap_Width,  &width);
    GetAttr(o, aHidd_BitMap_Height, &height);
    
    /* Change background color of X window to bg color of HIDD bitmap  */
    winattr.background_pixel = data->hidd2x11cmap[bg];
    
    XChangeWindowAttributes(data->display
    		, data->xwindow
		, CWBackPixel
		, &winattr);
    
    XClearArea (data->display, data->xwindow,
	    0, 0,
	    width, height,
	    FALSE);
    
    XFlush(data->display);
    return;
    
}

static VOID bitmap_getbox(Class *cl, Object *o, struct pHidd_BitMap_GetBox *msg)
{
    /* Read an X image from which we can faster get the pixels, since the
       X image resides in the client.
    */
    WORD x, y;
    ULONG *pixarray = msg->pixels;
    struct bitmap_data *data;
    XImage *image;
    
    EnterFunc(bug("X11Gfx.BitMap::GetBox(pa=%p, x=%d, y=%d, w=%d, h=%d)\n",
    	msg->pixels, msg->x, msg->y, msg->width, msg->height));
	
    data = INST_DATA(cl, o);

    image = XGetImage(data->display
    	, data->xwindow
	, msg->x, msg->y
	, msg->width, msg->height
	, AllPlanes
	, ZPixmap);
	
    if (!image)
    	ReturnVoid("X11Gfx.BitMap::GetBox(couldn't get XImage)");
	
    for (y = 0; y < msg->height; y ++)
    {
	for (x = 0; x < msg->width; x ++)
	{
	    *pixarray ++ = map_x11_to_hidd(data->hidd2x11cmap, XGetPixel(image, x, y));
	}
	
    }
    XFree(image);
    
    ReturnVoid("X11Gfx.BitMap::GetBox");
    
}

static VOID bitmap_putbox(Class *cl, Object *o, struct pHidd_BitMap_PutBox *msg)
{
    ULONG mode;
    WORD x, y;
    ULONG *pixarray = msg->pixels;
    struct bitmap_data *data;
    XImage *image;

    EnterFunc(bug("X11Gfx.BitMap::PutBox(pa=%p, x=%d, y=%d, w=%d, h=%d)\n",
    	msg->pixels, msg->x, msg->y, msg->width, msg->height));
	
    data = INST_DATA(cl, o);
    GetAttr(o, aHidd_BitMap_DrawMode, &mode);
    
    image = XGetImage(data->display
    	, data->xwindow
	, msg->x, msg->y
	, msg->width, msg->height
	, AllPlanes
	, ZPixmap
    );
    if (!image)
    	ReturnVoid("X11Gfx.BitMap::PutBox(couldn't get XImage)");
    	
    D(bug("drawmode: %d\n", mode));
    if (mode == vHIDD_GC_DrawMode_Copy)
    {
        D(bug("Drawmode COPY\n"));
    	/* Do plain copy, optimized */
	for (y = 0; y < msg->height; y ++)
	{
	    for (x = 0; x < msg->width; x ++)
	    {
		
		XPutPixel(image, x, y, data->hidd2x11cmap[*pixarray ++]);
		
	    }
	    
	}
	
    }
    else
    {
     	   
	for (y = 0; y < msg->height; y ++)
	{
	    for (x = 0; x < msg->width; x ++)
	    {
    		/* Drawmodes make things more complicated */
		ULONG src;
		ULONG dest;
		ULONG val = 0;

		src  = *pixarray ++;
		dest = map_x11_to_hidd(data->hidd2x11cmap, XGetPixel(image, x, y));
		    
		/* Apply drawmodes to hidd pen */
	   	if(mode & 1) val = ( src &  dest);
	   	if(mode & 2) val = ( src & ~dest) | val;
	   	if(mode & 4) val = (~src &  dest) | val;
	   	if(mode & 8) val = (~src & ~dest) | val;
		
		XPutPixel(image, x, y, data->hidd2x11cmap[val]);
	    }
	}
    }
    /* Put image back into display */
    XPutImage(data->display
    	, data->xwindow
	, data->gc
	, image
	, 0, 0
	, msg->x, msg->y
	, msg->width, msg->height);
	
	
   XFree(image);

   XFlush(data->display);
   ReturnVoid("X11Gfx.BitMap::PutBox");

   
}



/*** BitMap::BlitColExp() **********************************************/
static VOID bitmap_blitcolexp(Class *cl, Object *o, struct pHidd_BitMap_BlitColExp *msg)
{
    ULONG cemd;
    XImage *src_im, *dest_im;
    struct bitmap_data *data = INST_DATA(cl, o);
    ULONG fg, bg, fg_pixel, bg_pixel;
    LONG x, y;
    
    
    /* Get the color expansion mode */
    GetAttr(o, aHidd_BitMap_ColExpMode, &cemd);
    
    dest_im = XGetImage(data->display
    	, data->xwindow
	, msg->destX, msg->destY
	, msg->width, msg->height
	, AllPlanes
	, ZPixmap);
    	
    
    if (!dest_im)
    	return;

    GetAttr(o, aHidd_BitMap_Foreground, &fg);
    GetAttr(o, aHidd_BitMap_Background, &bg);
    
    fg_pixel = data->hidd2x11cmap[fg];
    bg_pixel = data->hidd2x11cmap[bg];
    

    /* Get XImage from offscreen HIDD bitmap */
    GetAttr(msg->srcBitMap, aHidd_X11Osbm_XImage, (IPTR *)&src_im);

    
    for (y = 0; y < msg->height; y ++)
    {
    	for (x = 0; x < msg->width; x ++)
	{
	    ULONG is_set;
	    
	    is_set = XGetPixel(src_im, x + msg->srcX, y + msg->srcY);
	    
	    if (is_set)
	    {
	    	XPutPixel(dest_im, x + msg->destX, y + msg->destY, fg_pixel);
	    }
	    else
	    {
		if (cemd & vHidd_GC_ColExp_Opaque)
		{
		    XPutPixel(dest_im, x + msg->destX, y + msg->destY, bg_pixel);
		}
	    }
	} /* for (each x) */
	    
    } /* for (each y) */
    
    
    /* Put image back into display */
    XPutImage(data->display
    	, data->xwindow
	, data->gc
	, dest_im
	, 0, 0
	, msg->destX, msg->destY
	, msg->width, msg->height
    );
    
    XDestroyImage(dest_im);
}

/*** init_bitmapclass *********************************************************/

#undef X11GfxBase

#define NUM_ROOT_METHODS   3
#define NUM_BITMAP_METHODS 10


Class *init_bitmapclass(struct x11gfxbase *X11GfxBase)
{
    struct MethodDescr root_descr[NUM_ROOT_METHODS + 1] =
    {
        {(IPTR (*)())bitmap_new    , moRoot_New    },
        {(IPTR (*)())bitmap_dispose, moRoot_Dispose},
        {(IPTR (*)())bitmap_set	   , moRoot_Set},
        {NULL, 0UL}
    };

    struct MethodDescr bitMap_descr[NUM_BITMAP_METHODS + 1] =
    {
        {(IPTR (*)())bitmap_setcolors,		moHidd_BitMap_SetColors},
    	{(IPTR (*)())bitmap_putpixel,		moHidd_BitMap_PutPixel},
    	{(IPTR (*)())bitmap_clear,		moHidd_BitMap_Clear},
    	{(IPTR (*)())bitmap_getpixel,		moHidd_BitMap_GetPixel},
    	{(IPTR (*)())bitmap_drawpixel,		moHidd_BitMap_DrawPixel},
    	{(IPTR (*)())bitmap_fillrect,		moHidd_BitMap_FillRect},
    	{(IPTR (*)())bitmap_copybox,		moHidd_BitMap_CopyBox},
    	{(IPTR (*)())bitmap_getbox,		moHidd_BitMap_GetBox},
    	{(IPTR (*)())bitmap_putbox,		moHidd_BitMap_PutBox},
    	{(IPTR (*)())bitmap_blitcolexp,		moHidd_BitMap_BlitColExp},
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
	    if (obtainattrbases(attrbases, OOPBase))
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
	
	releaseattrbases(attrbases, OOPBase);
	
    }

    ReturnVoid("free_bitmapclass");
}
