/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: X11 gfx HIDD for AROS.
    Lang: English.
*/


#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/cursorfont.h>

#include <exec/libraries.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <aros/libcall.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <oop/oop.h>

#include <hidd/hidd.h>
#include <hidd/graphics.h>

#include "x11gfx_intern.h"

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>


#undef SysBase
#define SysBase (X11GfxBase->sysbase)

#undef OOPBase
#define OOPBase (X11GfxBase->oopbase)

#undef UtilityBase
#define UtilityBase (X11GfxBase->utilitybase)

#define X11GfxBase ((struct x11gfxbase *)cl->UserData)

#define IS_X11GFX_ATTR(attr, idx) ( ( (idx) = (attr) - HiddX11GfxAB) < num_Hidd_X11Gfx_Attrs)


/* Some attrbases needed as global vars.
  These are write-once read-many */

static AttrBase HiddBitMapAttrBase = 0;  
static AttrBase HiddX11GfxAB = 0;

struct abdescr attrbases[] =
{
    { IID_Hidd_BitMap, &HiddBitMapAttrBase },
    { IID_Hidd_X11Gfx, &HiddX11GfxAB },
    { NULL, NULL }
};


/* Private instance data for Gfx hidd class */
struct gfx_data
{
    Display	*display;
    int		screen;
    int		depth;
    long hidd2x11cmap[256];
    Colormap	colmap;
    Cursor	cursor;
};

/*********************
**  GfxHidd::New()  **
*********************/


/*
   Inits sysdisplay, sysscreen, colormap, etc.. */
static BOOL initx11stuff(struct gfx_data *data)
{
/*    XColor fg, bg; */
    BOOL ok = TRUE;
    char *displayname;

    /* Try to get the display */
    if (!(displayname = getenv("DISPLAY")))
	displayname =":0.0";


    data->display = XOpenDisplay(displayname);
    if (!data->display)
    	ok = FALSE;
    else
    {
        XColor bg, fg;
    	data->screen = DefaultScreen( data->display );
	
	data->depth  = DisplayPlanes( data->display, data->screen );
	data->colmap = DefaultColormap( data->display, data->screen );
	
	
/*	data->maxpen = NUM_COLORS; */
	
	/* Create cursor */
	
	data->cursor = XCreateFontCursor( data->display, XC_top_left_arrow);
	fg.pixel = BlackPixel(data->display, data->screen);
	fg.red = 0x0000; fg.green = 0x0000; fg.blue = 0x0000;
	fg.flags = (DoRed | DoGreen | DoBlue);
	bg.pixel = WhitePixel(data->display, data->screen);
	bg.red = 0xFFFF; bg.green = 0xFFFF; bg.blue = 0xFFFF;
	bg.flags = (DoRed | DoGreen | DoBlue);
	
	
	XRecolorCursor( data->display
		, data->cursor
		, &fg, &bg
	);

    } /* if (Could get sysdisplay) */
    
    return ok;

}

static VOID cleanupx11stuff(struct gfx_data *data)
{
    /* Do nothing for now */
    return;
}

static int MyErrorHandler (Display * display, XErrorEvent * errevent)
{
    char buffer[256];

    XGetErrorText (display, errevent->error_code, buffer, sizeof (buffer));
    fprintf (stderr
	, "XError %d (Major=%d, Minor=%d)\n%s\n"
	, errevent->error_code
	, errevent->request_code
	, errevent->minor_code
	, buffer
    );
    fflush (stderr);

    return 0;
}

static int MySysErrorHandler (Display * display)
{
    perror ("X11-Error");
    fflush (stderr);

    return 0;
}


static Object *gfx_new(Class *cl, Object *o, struct pRoot_New *msg)
{

    EnterFunc(bug("X11Gfx::New()\n"));

    o = (Object *)DoSuperMethod(cl, o, (Msg)msg);
    if (o)
    {
    
	MethodID dispose_mid;
	struct gfx_data *data = INST_DATA(cl, o);
	
	/* Do GfxHidd initalization here */
	if (initx11stuff(data));
	{
	    XSetErrorHandler (MyErrorHandler);
	    XSetIOErrorHandler (MySysErrorHandler);

    	    ReturnPtr("X11Gfx::New", Object *, o);
	}
	
	dispose_mid = GetMethodID(IID_Root, moRoot_Dispose);
	CoerceMethod(cl, o, (Msg)&dispose_mid);
    }
    ReturnPtr("X11Gfx::New", Object *, NULL);
}

/********** GfxHidd::Dispose()  ******************************/
static VOID gfx_dispose(Class *cl, Object *o, Msg msg)
{
    struct gfx_data *data = INST_DATA(cl, o);
    
    cleanupx11stuff(data);
    
    DoSuperMethod(cl, o, msg);
}

/********** GfxHidd::NewBitMap()  ****************************/
static Object *gfxhidd_newbitmap(Class *cl, Object *o, struct pHidd_Gfx_NewBitMap *msg)
{

    BOOL displayable;
    Class *bm_cl;
    Object *bm;
    
    struct gfx_data *data;
    struct TagItem tags[] =
    {
    	{ aHidd_X11Gfx_SysDisplay,	(IPTR) NULL},
	{ aHidd_X11Gfx_SysScreen,	0UL },
	{ aHidd_X11Gfx_Hidd2X11CMap,	(IPTR) NULL},
	{ aHidd_X11Gfx_SysCursor,	0UL},
	{ aHidd_X11Gfx_ColorMap,	0UL},
	{ TAG_MORE, (IPTR) NULL }
    };
    
    EnterFunc(bug("X11Gfx::NewBitMap()\n"));
    
    data = INST_DATA(cl, o);
    
    tags[0].ti_Data = (IPTR)data->display;
    tags[1].ti_Data = data->screen;
    tags[2].ti_Data = (IPTR)data->hidd2x11cmap;
    tags[3].ti_Data = (IPTR)data->cursor;
    tags[4].ti_Data = data->colmap;
    tags[5].ti_Data = (IPTR)msg->attrList;
    

    /* Displayeable bitmap ? */
    
    displayable = GetTagData(aHidd_BitMap_Displayable, FALSE, msg->attrList);
    if (displayable)
    {
    	bm = NewObject(X11GfxBase->bitmapclass, NULL, tags);
    }
    else
    {
	bm = NewObject(NULL, CLID_Hidd_BitMap, tags);
    }
    
    
    ReturnPtr("X11Gfx::NewBitMap", Object *, bm);
}

/******* X11Gfx::Set()  ********************************************/
static VOID gfx_get(Class *cl, Object *o, struct pRoot_Get *msg)
{
    struct gfx_data *data = INST_DATA(cl, o);
    ULONG idx;
    if (IS_X11GFX_ATTR(msg->attrID, idx))
    {
	switch (idx)
	{
	    case aoHidd_X11Gfx_SysDisplay:
	    	*msg->storage = (IPTR)data->display;
		break;
		
	    case aoHidd_X11Gfx_SysScreen:
	    	*msg->storage = (IPTR)data->screen;
		break;
	    
	    default:
	    	DoSuperMethod(cl, o, (Msg)msg);
		break;
	}
    }
    
    return;
}

#undef X11GfxBase

/********************  init_gfxclass()  *********************************/

#define NUM_ROOT_METHODS 3
#define NUM_GFXHIDD_METHODS 1

Class *init_gfxclass (struct x11gfxbase *X11GfxBase)
{
    Class *cl = NULL;

    struct MethodDescr root_descr[NUM_ROOT_METHODS + 1] = 
    {
    	{(IPTR (*)())gfx_new,		moRoot_New},
    	{(IPTR (*)())gfx_dispose,	moRoot_Dispose},
    	{(IPTR (*)())gfx_get,		moRoot_Get},
	{NULL, 0UL}
    };
    
    struct MethodDescr gfxhidd_descr[NUM_GFXHIDD_METHODS + 1] = 
    {
    	{(IPTR (*)())gfxhidd_newbitmap,	moHidd_Gfx_NewBitMap},
	{NULL, 0UL}
    };
    
    
    struct InterfaceDescr ifdescr[] =
    {
    	{root_descr, 	IID_Root, 	NUM_ROOT_METHODS},
    	{gfxhidd_descr, IID_Hidd_Gfx, 	NUM_GFXHIDD_METHODS},
	{NULL, NULL, 0}
    };
    
    AttrBase MetaAttrBase = ObtainAttrBase(IID_Meta);
	
    struct TagItem tags[] =
    {
	{ aMeta_SuperID,		(IPTR)CLID_Hidd_Gfx},
	{ aMeta_InterfaceDescr,		(IPTR)ifdescr},
	{ aMeta_InstSize,		(IPTR)sizeof (struct gfx_data) },
	{TAG_DONE, 0UL}
    };

    EnterFunc(bug("GfxHiddClass init\n"));
    
    if (MetaAttrBase)
    {

    	cl = NewObject(NULL, CLID_HiddMeta, tags);
    
    	if(cl)
    	{
	    if (obtainattrbases(attrbases, OOPBase))
	    {
		D(bug("GfxHiddClass ok\n"));
		cl->UserData = (APTR)X11GfxBase;
	    	AddClass(cl);
	    }
	    else
	    {
	    	free_gfxclass(X11GfxBase);
		cl = NULL;
	    }
	}
	
	/* Don't need this anymore */
	ReleaseAttrBase(IID_Meta);
    }
    return cl;
}




/*************** free_gfxclass()  **********************************/
VOID free_gfxclass(struct x11gfxbase *X11GfxBase)
{
    EnterFunc(bug("free_gfxclass(X11GfxBase=%p)\n", X11GfxBase));

    if(X11GfxBase)
    {

        RemoveClass(X11GfxBase->gfxclass);
	
        if(X11GfxBase->gfxclass) DisposeObject((Object *) X11GfxBase->gfxclass);
        X11GfxBase->gfxclass = NULL;
	
	releaseattrbases(attrbases, OOPBase);

    }

    ReturnVoid("free_gfxclass");
}



