/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: X11 gfx HIDD for AROS.
    Lang: English.
*/


#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/Xutil.h>

#include <sys/signal.h>

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
#include "x11.h"
#include "bitmap.h"

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>


#define IS_X11GFX_ATTR(attr, idx) ( ( (idx) = (attr) - HiddX11GfxAB) < num_Hidd_X11Gfx_Attrs)


/* Some attrbases needed as global vars.
  These are write-once read-many */

static AttrBase HiddBitMapAttrBase	= 0;  
static AttrBase HiddX11GfxAB		= 0;
static AttrBase HiddX11BitMapAB		= 0;
static AttrBase HiddSyncAttrBase	= 0;
static AttrBase HiddPixFmtAttrBase	= 0;
static AttrBase HiddGfxAttrBase		= 0;

static struct ABDescr attrbases[] =
{
    { IID_Hidd_BitMap,  	&HiddBitMapAttrBase	},
    { IID_Hidd_X11Gfx,  	&HiddX11GfxAB		},
    { IID_Hidd_X11BitMap,	&HiddX11BitMapAB	},
    { IID_Hidd_Sync,		&HiddSyncAttrBase	},
    { IID_Hidd_PixFmt,		&HiddPixFmtAttrBase	},
    { IID_Hidd_Gfx,		&HiddGfxAttrBase	},
    { NULL, NULL }
};


/* Private instance data for Gfx hidd class */
struct gfx_data
{
    Display	*display;
    int		screen;
    int		depth;
    Colormap	colmap;
    Cursor	cursor;
   
};


static VOID cleanupx11stuff(struct x11_staticdata *xsd);
static BOOL initx11stuff(struct x11_staticdata *xsd);

/*********************
**  GfxHidd::New()  **
*********************/


static Object *gfx_new(Class *cl, Object *o, struct pRoot_New *msg)
{


    struct TagItem pftags[] = {
    	{ aHidd_PixFmt_RedShift,	0	}, /* 0 */
	{ aHidd_PixFmt_GreenShift,	0	}, /* 1 */
	{ aHidd_PixFmt_BlueShift,  	0	}, /* 2 */
	{ aHidd_PixFmt_AlphaShift,	0	}, /* 3 */
	{ aHidd_PixFmt_RedMask,		0	}, /* 4 */
	{ aHidd_PixFmt_GreenMask,	0	}, /* 5 */
	{ aHidd_PixFmt_BlueMask,	0	}, /* 6 */
	{ aHidd_PixFmt_AlphaMask,	0	}, /* 7 */
	{ aHidd_PixFmt_ColorModel,	0	}, /* 8 */
	{ aHidd_PixFmt_Depth,		0	}, /* 9 */
	{ aHidd_PixFmt_BytesPerPixel,	0	}, /* 10 */
	{ aHidd_PixFmt_BitsPerPixel,	0	}, /* 11 */
	{ aHidd_PixFmt_StdPixFmt,	0	}, /* 12 */
	{ aHidd_PixFmt_CLUTShift,	0	}, /* 13 */
	{ aHidd_PixFmt_CLUTMask,	0	}, /* 14 */ 
	{ aHidd_PixFmt_BitMapType,	0	}, /* 15 */   
	{ TAG_DONE, 0UL }
    };
        
    struct TagItem tags_320_240[] = {
    	{ aHidd_Sync_HDisp,		320	},
	{ aHidd_Sync_VDisp,		240	},
	{ TAG_DONE, 0UL }
    };

    struct TagItem tags_512_384[] = {
    	{ aHidd_Sync_HDisp,		512	},
	{ aHidd_Sync_VDisp,		384	},
	{ TAG_DONE, 0UL }
    };

    struct TagItem tags_640_480[] = {
    	{ aHidd_Sync_HDisp,		640	},
	{ aHidd_Sync_VDisp,		480	},
	{ TAG_DONE, 0UL }
    };

    struct TagItem tags_800_600[] = {
    	{ aHidd_Sync_HDisp,		800	},
	{ aHidd_Sync_VDisp,		600	},
	{ TAG_DONE, 0UL }
    };

    struct TagItem tags_1024_768[] = {
    	{ aHidd_Sync_HDisp,		1024	},
	{ aHidd_Sync_VDisp,		768	},
	{ TAG_DONE, 0UL }
    };
    
    struct TagItem mode_tags[] = {
	{ aHidd_Gfx_PixFmtTags, (IPTR)pftags		},
	
	/* Default values for the sync attributes */
	{ aHidd_Sync_PixelClock, 	100000000	}, /* Oh boy,  this X11 pixelclock is fast ;-) */
	{ aHidd_Sync_LeftMargin,	0		},
	{ aHidd_Sync_RightMargin,	0		},
	{ aHidd_Sync_HSyncLength,	0		},
	{ aHidd_Sync_UpperMargin,	0		},
	{ aHidd_Sync_LowerMargin,	0		},
	{ aHidd_Sync_VSyncLength,	0		},
	
	/* The different syncmodes. The default attribute values above 
	    will be applied to each of these. Note that
	    you can alter the defaults between the tags bewlow 
	*/
	{ aHidd_Gfx_SyncTags,	(IPTR)tags_320_240	},
	{ aHidd_Gfx_SyncTags,	(IPTR)tags_512_384	},
	{ aHidd_Gfx_SyncTags,	(IPTR)tags_640_480	},
	{ aHidd_Gfx_SyncTags,	(IPTR)tags_800_600	},
	{ aHidd_Gfx_SyncTags,	(IPTR)tags_1024_768	},
	{ TAG_DONE, 0UL }
    };
    
    struct TagItem mytags[] = {
	{ aHidd_Gfx_ModeTags,	(IPTR)mode_tags		},
	{ TAG_MORE, (IPTR)msg->attrList }
    };
    struct pRoot_New mymsg = { msg->mID, mytags };
    struct x11_staticdata *xsd = NULL;

    EnterFunc(bug("X11Gfx::New()\n"));

	/* Do GfxHidd initalization here */
    if (!initx11stuff(XSD(cl))) {
	kprintf("!!! initx11stuff() FAILED IN X11Gfx::New() !!!\n");
	ReturnPtr("X11Gfx::New()", Object *, NULL);
    }
	
    /* Register gfxmodes */
    pftags[0].ti_Data = XSD(cl)->red_shift;
    pftags[1].ti_Data = XSD(cl)->green_shift;
    pftags[2].ti_Data = XSD(cl)->blue_shift;
    pftags[3].ti_Data = 0;
	    
    pftags[4].ti_Data = XSD(cl)->vi.red_mask;
    pftags[5].ti_Data = XSD(cl)->vi.green_mask;
    pftags[6].ti_Data = XSD(cl)->vi.blue_mask;
    pftags[7].ti_Data = 0x00000000;
	    
    if (XSD(cl)->vi.class == TrueColor) {
        pftags[8].ti_Data = vHidd_ColorModel_TrueColor;
    } else if (XSD(cl)->vi.class == PseudoColor) {
	pftags[8].ti_Data = vHidd_ColorModel_Palette;
        pftags[13].ti_Data = XSD(cl)->clut_shift;
	pftags[14].ti_Data = XSD(cl)->clut_mask;		
    } else {
	kprintf("!!! UNHANDLED COLOR MODEL IN X11Gfx:New(): %d !!!\n", XSD(cl)->vi.class);
	cleanupx11stuff(xsd);
	ReturnPtr("X11Gfx::New", Object *, NULL);
    }
	    
    pftags[9].ti_Data = XSD(cl)->size;
    pftags[10].ti_Data = XSD(cl)->bytes_per_pixel;
    pftags[11].ti_Data = XSD(cl)->size;
    pftags[12].ti_Data = vHidd_StdPixFmt_Native;
    
#warning Do better than this
    /* We assume chunky */
    pftags[15].ti_Data = vHidd_BitMapType_Chunky;
    
    o = (Object *)DoSuperMethod(cl, o, (Msg)&mymsg);
    if (NULL != o) {
	XColor bg, fg;
	struct gfx_data *data = INST_DATA(cl, o);
LX11
	data->display	= XSD(cl)->display;
	data->screen	= DefaultScreen( data->display );
	data->depth	= DisplayPlanes( data->display, data->screen );
	data->colmap	= DefaultColormap( data->display, data->screen );
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
UX11
	
	D(bug("X11Gfx::New(): Got object from super\n"));
ObtainSemaphore(&XSD(cl)->sema);
	XSD(cl)->activecallback = (VOID (*)())GetTagData(aHidd_Gfx_ActiveBMCallBack, (IPTR)NULL, msg->attrList);
	XSD(cl)->callbackdata = (APTR)GetTagData(aHidd_Gfx_ActiveBMCallBackData, (IPTR)NULL, msg->attrList);
ReleaseSemaphore(&XSD(cl)->sema);
	
	data->display = XSD(cl)->display;
	
    }
    ReturnPtr("X11Gfx::New", Object *, o);
}

/********** GfxHidd::Dispose()  ******************************/
static VOID gfx_dispose(Class *cl, Object *o, Msg msg)
{
    struct gfx_data *data;
    EnterFunc(bug("X11Gfx::Dispose(o=%p)\n", o));
    data = INST_DATA(cl, o);
    
    cleanupx11stuff(XSD(cl));
    D(bug("X11Gfx::Dispose: calling super\n"));
    
    DoSuperMethod(cl, o, msg);
    
    ReturnVoid("X11Gfx::Dispose");
}

/********** GfxHidd::NewBitMap()  ****************************/
static Object *gfxhidd_newbitmap(Class *cl, Object *o, struct pHidd_Gfx_NewBitMap *msg)
{

    BOOL displayable;
    
    struct pHidd_Gfx_NewBitMap p;
    
    struct gfx_data *data;
    struct TagItem tags[] =
    {
    	{ aHidd_X11Gfx_SysDisplay,	(IPTR) NULL	},	/* 0 */
	{ aHidd_X11Gfx_SysScreen,	0UL 		},	/* 1 */	
	{ aHidd_X11Gfx_SysCursor,	0UL		},	/* 2 */
	{ aHidd_X11Gfx_ColorMap,	0UL		},	/* 3 */
	{ aHidd_X11Gfx_VisualClass,	0UL		},	/* 4 */
	{ TAG_IGNORE,			0UL		},	/* 5 */
	{ TAG_MORE, (IPTR) NULL }				/* 6 */
    };
    
    EnterFunc(bug("X11Gfx::NewBitMap()\n"));
    
    data = INST_DATA(cl, o);
    
    tags[0].ti_Data = (IPTR)data->display;
    tags[1].ti_Data = data->screen;
    tags[2].ti_Data = (IPTR)data->cursor;
    tags[3].ti_Data = data->colmap;
    tags[4].ti_Data = XSD(cl)->vi.class;
    tags[6].ti_Data = (IPTR)msg->attrList;
    
    /* Displayable bitmap ? */
    displayable = GetTagData(aHidd_BitMap_Displayable, FALSE, msg->attrList);
    if (displayable) {
    	tags[5].ti_Tag	= aHidd_BitMap_ClassPtr;
	tags[5].ti_Data	= (IPTR)XSD(cl)->onbmclass;
    } else {
    	/* When do we create an x11 offscreen bitmap ?
	    - For 1-plane bitmaps.
	    - Bitmaps that have a friend that is an X11 bitmap
	      and there is no standard pixfmt supplied
	    - If the user supplied a modeid.
	*/
	Object *friend;
	BOOL usex11 = FALSE;
    	HIDDT_StdPixFmt stdpf;

	friend = (Object *)GetTagData(aHidd_BitMap_Friend, 0, msg->attrList);
	stdpf = (HIDDT_StdPixFmt)GetTagData(aHidd_BitMap_StdPixFmt, vHidd_StdPixFmt_Unknown, msg->attrList);
	if (NULL != friend) {
	    if (vHidd_StdPixFmt_Unknown == stdpf) {
	    	Drawable d;
	    	/* Is the friend ann X11 bitmap ? */
	    	d = (Drawable)GetAttr(friend, aHidd_X11BitMap_Drawable, (IPTR *)&d);
	    	if (0 != d) {
	    	    usex11 = TRUE;
		}
	    }
	}
	if (!usex11) {
	    if (vHidd_StdPixFmt_Plane == stdpf) {
	    	usex11 = TRUE;
	    } else {
	    	HIDDT_ModeID modeid;
	    	modeid = (HIDDT_ModeID)GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);
		if (vHidd_ModeID_Invalid != modeid) {
		    usex11 = TRUE;
		}
	    }
	}
	
	if (usex11) {
	    tags[5].ti_Tag  = aHidd_BitMap_ClassPtr;
	    tags[5].ti_Data = (IPTR)XSD(cl)->offbmclass;
	    
	} else {
	    /* Let the superclass allocate if it is a standard pixelformat thus do nothing */
	    kprintf("!!! COULD NOT CREATE OFSCREEN X11 BITMAP FOR SUPPLIED ATTRS !!!\n");
	    *((ULONG *)0) = 0;
	}
    }
    
    /* !!! IMPORTANT !!! */
    
    p.mID = msg->mID;
    p.attrList = tags;
    
    ReturnPtr("X11Gfx::NewBitMap", Object *, (Object *)DoSuperMethod(cl, o, (Msg)&p));
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
    } else if (IS_GFX_ATTR(msg->attrID, idx)) {
    	switch (idx) {
	    case aoHidd_Gfx_IsWindowed:
	    	*msg->storage = (IPTR)TRUE;
		break;
		
	    default:
	    	DoSuperMethod(cl, o, (Msg)msg);
		break;
	}
    } else {
    	DoSuperMethod(cl, o, (Msg)msg);
    }
    
    return;
}

#undef XSD
#define XSD(cl) xsd

/********************  init_gfxclass()  *********************************/

#define NUM_ROOT_METHODS 3
#define NUM_GFXHIDD_METHODS 1

Class *init_gfxclass (struct x11_staticdata *xsd)
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
	{ aMeta_ID,		(IPTR)CLID_Hidd_X11Gfx },
	{TAG_DONE, 0UL}
    };

    EnterFunc(bug("GfxHiddClass init\n"));
    
    if (MetaAttrBase)
    {

    	cl = NewObject(NULL, CLID_HiddMeta, tags);
    
    	if(cl)
    	{
	    cl->UserData = (APTR)xsd;
	    xsd->gfxclass = cl;
	    
	    if (ObtainAttrBases(attrbases))
	    {
		D(bug("GfxHiddClass ok\n"));
	    	AddClass(cl);
	    }
	    else
	    {
	    	free_gfxclass( xsd );
		cl = NULL;
	    }
	}
	
	/* Don't need this anymore */
	ReleaseAttrBase(IID_Meta);
    }
    ReturnPtr("init_gfxclass", Class *, cl);
}




/*************** free_gfxclass()  **********************************/
VOID free_gfxclass(struct x11_staticdata *xsd)
{
    EnterFunc(bug("free_gfxclass(xsd=%p)\n", xsd));

    if(xsd)
    {

        RemoveClass(xsd->gfxclass);
	
        if(xsd->gfxclass) DisposeObject((Object *) xsd->gfxclass);
        xsd->gfxclass = NULL;
	
	ReleaseAttrBases(attrbases);

    }

    ReturnVoid("free_gfxclass");
}



static ULONG mask_to_shift(ULONG mask)
{
    ULONG i;
    
    for (i = 32; mask; i --) {
	mask >>= 1;
    }
	
    if (mask == 32) {
   	i = 0;
    }
	
    return i;
}

/*
   Inits sysdisplay, sysscreen, colormap, etc.. */
static BOOL initx11stuff(struct x11_staticdata *xsd)
{
/*    XColor fg, bg; */
    BOOL ok = TRUE;

    XVisualInfo template;
    XVisualInfo *visinfo;
    int template_mask;
    int numvisuals;


    
    XImage *testimage;


    EnterFunc(bug("initx11stuff()\n"));


LX11	

    /* Get some info on the display */
    template.visualid = XVisualIDFromVisual(DefaultVisual(xsd->display, DefaultScreen(xsd->display)));
    template_mask = VisualIDMask;

    visinfo = XGetVisualInfo(xsd->display, template_mask, &template, &numvisuals);

    if (numvisuals > 1)
    {

    	    kprintf("GOT MORE THAN ONE VISUAL FROM X\n");
//    	    kill(getpid(), SIGSTOP);
    }

    if (NULL == visinfo)
    {
    	    kprintf("COULD NOT GET X VISUAL INFO\n");
    	    kill(getpid(), SIGSTOP);
    	    
    	    ok = FALSE;
    }
    else
    {
	/* Store the visual info structure */

	memcpy(&xsd->vi, visinfo, sizeof (XVisualInfo));
	
	XFree(visinfo);
	
	visinfo = &xsd->vi;
	
	/* We only support TrueColor for now */
	
	switch (visinfo->class) {
	    case TrueColor:
		/* Get the pixel masks */
		xsd->red_shift	 = mask_to_shift(xsd->vi.red_mask);
		xsd->green_shift = mask_to_shift(xsd->vi.green_mask);
		xsd->blue_shift	 = mask_to_shift(xsd->vi.blue_mask);
	        break;
		
	    case PseudoColor:
	        /* stegerg */
	    	xsd->vi.red_mask   = ((1 << xsd->vi.bits_per_rgb) - 1) << (xsd->vi.bits_per_rgb * 2);
		xsd->vi.green_mask = ((1 << xsd->vi.bits_per_rgb) - 1) << (xsd->vi.bits_per_rgb * 1);
		xsd->vi.blue_mask  = ((1 << xsd->vi.bits_per_rgb) - 1);
		xsd->red_shift	 = mask_to_shift(xsd->vi.red_mask);
		xsd->green_shift = mask_to_shift(xsd->vi.green_mask);
		xsd->blue_shift	 = mask_to_shift(xsd->vi.blue_mask);
		/* end stegerg */
	    	break;
		
	    default:
	    	kprintf("GFX HIDD only supports truecolor and pseudocolor diplays for now\n");
	    	kill(getpid(), SIGSTOP);
	}

	xsd->size = 0;

	/* Create a dummy X image to get bits per pixel */
	testimage = XGetImage(xsd->display
		, RootWindow(xsd->display, DefaultScreen(xsd->display))
		, 0, 0, 1, 1
		, AllPlanes, ZPixmap
	);
	
	if (NULL != testimage)	{
	    xsd->size = testimage->bits_per_pixel;
	    XDestroyImage(testimage);
	} else {
	    kprintf("!!! X11gfx could not get bits per pixel\n");
	    kill(getpid(), SIGSTOP);
	}
	
	xsd->bytes_per_pixel = (xsd->size + 7) >> 3;
	
	if (PseudoColor == xsd->vi.class) {
	    xsd->clut_mask  = (1L << xsd->size) - 1;
	    xsd->clut_shift = 0;
	}
    }

    /* Create a dummy window for pixmaps */

    xsd->dummy_window_for_creating_pixmaps = XCreateSimpleWindow(
   	     xsd->display
   	   , DefaultRootWindow(xsd->display)
   	   , 0, 0
   	   , 100, 100
   	   , 0
   	   , BlackPixel(xsd->display, DefaultScreen(xsd->display))
   	   , BlackPixel(xsd->display, DefaultScreen(xsd->display))
    );

kprintf("\n1\n");
    if (0 == xsd->dummy_window_for_creating_pixmaps)
    {
kprintf("\n2 ----\n");
	ok = FALSE;
    }
#if USE_XSHM
    	    
    /* Do we have Xshm support ? */
    xsd->xshm_info = init_shared_mem(xsd->display);
    if (NULL == xsd->xshm_info)
    {
kprintf("\n3\n");
    	ok = FALSE;
kprintf("INITIALIZATION OF XSHM FAILED !!\n");	    
    }
    else
    {

    	InitSemaphore(&xsd->shm_sema);
    	xsd->use_xshm = TRUE;
    }    	
#endif

	kprintf("\n3 %d \n",ok);

UX11
    
    ReturnBool("initx11stuff", ok);

}

static VOID cleanupx11stuff(struct x11_staticdata *xsd)
{
LX11
    /* Do nothing for now */
    if (0 != xsd->dummy_window_for_creating_pixmaps)
    {
    	XDestroyWindow(xsd->display, xsd->dummy_window_for_creating_pixmaps);
    }
    
#if USE_XSHM
	cleanup_shared_mem(xsd->display, xsd->xshm_info);
#endif    
UX11
    return;
}

