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


#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>


#define IS_X11GFX_ATTR(attr, idx) ( ( (idx) = (attr) - HiddX11GfxAB) < num_Hidd_X11Gfx_Attrs)


/* Some attrbases needed as global vars.
  These are write-once read-many */

static AttrBase HiddBitMapAttrBase	= 0;  
static AttrBase HiddX11GfxAB		= 0;
static AttrBase HiddGfxModeAttrBase	= 0;
static AttrBase HiddGfxAttrBase		= 0;

static struct ABDescr attrbases[] =
{
    { IID_Hidd_BitMap,  &HiddBitMapAttrBase	},
    { IID_Hidd_X11Gfx,  &HiddX11GfxAB		},
    { IID_Hidd_GfxMode, &HiddGfxModeAttrBase	},
    { IID_Hidd_Gfx,	&HiddGfxAttrBase	},
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


static VOID cleanupx11stuff(struct gfx_data *data, struct x11_staticdata *xsd);
static BOOL initx11stuff(struct gfx_data *data, struct x11_staticdata *xsd);

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
	{ aHidd_PixFmt_GraphType,	0	}, /* 8 */
	{ aHidd_PixFmt_Depth,		0	}, /* 9 */
	{ aHidd_PixFmt_BytesPerPixel,	0	}, /* 10 */
	{ aHidd_PixFmt_BitsPerPixel,	0	}, /* 11 */
	{ aHidd_PixFmt_StdPixFmt,	0	}, /* 12 */
	{ TAG_DONE, 0UL }
    };
        
    struct TagItem tags_320_240[] = {
    	{ aHidd_GfxMode_Width,		320	},
	{ aHidd_GfxMode_Height,		240	},
	{ aHidd_GfxMode_PixFmtTags,	(IPTR)pftags	},
	{ TAG_DONE, 0UL }
    };

    struct TagItem tags_512_384[] = {
    	{ aHidd_GfxMode_Width,		512	},
	{ aHidd_GfxMode_Height,		384	},
	{ aHidd_GfxMode_PixFmtTags,	(IPTR)pftags	},
	{ TAG_DONE, 0UL }
    };

    struct TagItem tags_640_480[] = {
    	{ aHidd_GfxMode_Width,		640	},
	{ aHidd_GfxMode_Height,		480	},
	{ aHidd_GfxMode_PixFmtTags,	(IPTR)pftags	},
	{ TAG_DONE, 0UL }
    };

    struct TagItem tags_800_600[] = {
    	{ aHidd_GfxMode_Width,		800	},
	{ aHidd_GfxMode_Height,		600	},
	{ aHidd_GfxMode_PixFmtTags,	(IPTR)pftags	},
	{ TAG_DONE, 0UL }
    };

    struct TagItem tags_1024_768[] = {
    	{ aHidd_GfxMode_Width,		1024	},
	{ aHidd_GfxMode_Height,		768	},
	{ aHidd_GfxMode_PixFmtTags,	(IPTR)pftags	},
	{ TAG_DONE, 0UL }
    };
    
    struct TagItem *mode_tags[] = {
	tags_320_240, tags_512_384, tags_640_480, tags_800_600, tags_1024_768, NULL
    };

    EnterFunc(bug("X11Gfx::New()\n"));
    
    o = (Object *)DoSuperMethod(cl, o, (Msg)msg);
    if (o)
    {
    
	MethodID dispose_mid;
	struct gfx_data *data = INST_DATA(cl, o);
	
	D(bug("Got object from super\n"));
ObtainSemaphore(&XSD(cl)->sema);
	XSD(cl)->activecallback = (VOID (*)())GetTagData(aHidd_Gfx_ActiveBMCallBack, (IPTR)NULL, msg->attrList);
	XSD(cl)->callbackdata = (APTR)GetTagData(aHidd_Gfx_ActiveBMCallBackData, (IPTR)NULL, msg->attrList);
ReleaseSemaphore(&XSD(cl)->sema);
	
	data->display = XSD(cl)->display;
	D(bug("Initing misc X11 stuff\n"));
	
	/* Do GfxHidd initalization here */
	if (initx11stuff(data, XSD(cl)));
	{
	
	    /* Register gfxmodes */
	    pftags[0].ti_Data = XSD(cl)->red_shift;
	    pftags[1].ti_Data = XSD(cl)->green_shift;
	    pftags[2].ti_Data = XSD(cl)->blue_shift;
	    pftags[3].ti_Data = 0;
	    
	    pftags[4].ti_Data = XSD(cl)->vi.red_mask;
	    pftags[5].ti_Data = XSD(cl)->vi.green_mask;
	    pftags[6].ti_Data = XSD(cl)->vi.blue_mask;
	    pftags[7].ti_Data = 0x00000000;
	    
	    pftags[8].ti_Data = vHidd_GT_TrueColor;
	    pftags[9].ti_Data = XSD(cl)->size;
	    pftags[10].ti_Data = XSD(cl)->bytes_per_pixel;
	    pftags[11].ti_Data = XSD(cl)->size;
	    pftags[12].ti_Data = vHidd_PixFmt_Native;
	    
	    if (HIDD_Gfx_RegisterGfxModes(o, mode_tags)) {

		ReturnPtr("X11Gfx::New", Object *, o);
	    }
	}
	
	D(bug("Disposing obj\n"));
	
	dispose_mid = GetMethodID(IID_Root, moRoot_Dispose);
	CoerceMethod(cl, o, (Msg)&dispose_mid);
    }
    ReturnPtr("X11Gfx::New", Object *, NULL);
}

/********** GfxHidd::Dispose()  ******************************/
static VOID gfx_dispose(Class *cl, Object *o, Msg msg)
{
    struct gfx_data *data;
    EnterFunc(bug("X11Gfx::Dispose(o=%p)\n", o));
    data = INST_DATA(cl, o);
    
    cleanupx11stuff(data, XSD(cl));
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
    HIDDT_StdPixFmt stdpf;
    struct TagItem tags[] =
    {
    	{ aHidd_X11Gfx_SysDisplay,	(IPTR) NULL},
	{ aHidd_X11Gfx_SysScreen,	0UL },
	{ aHidd_X11Gfx_SysCursor,	0UL},
	{ aHidd_X11Gfx_ColorMap,	0UL},
	{ TAG_MORE, (IPTR) NULL }
    };
    
    EnterFunc(bug("X11Gfx::NewBitMap()\n"));
    
    data = INST_DATA(cl, o);
    
    tags[0].ti_Data = (IPTR)data->display;
    tags[1].ti_Data = data->screen;
    tags[2].ti_Data = (IPTR)data->cursor;
    tags[3].ti_Data = data->colmap;
    tags[4].ti_Data = (IPTR)msg->attrList;
    
    /* Displayeable bitmap ? */
    
    
    stdpf = (HIDDT_StdPixFmt)GetTagData(aHidd_BitMap_StdPixFmt, vHidd_PixFmt_Unknown, msg->attrList);
    
    displayable = GetTagData(aHidd_BitMap_Displayable, FALSE, msg->attrList);
    if (displayable) {
    	p.classPtr = XSD(cl)->onbmclass;
    } else {
    
	if (vHidd_PixFmt_Unknown == stdpf) {
	    p.classPtr = XSD(cl)->offbmclass;
	} else {
	    /* Let the superclass allocate if it is a standard pixelformat */
	    p.classPtr = NULL;
	}
    }
    
    /* !!! IMPORTANT !!! */
    p.classID = NULL;
    
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
static BOOL initx11stuff(struct gfx_data *data, struct x11_staticdata *xsd)
{
/*    XColor fg, bg; */
    BOOL ok = TRUE;

    XVisualInfo template;
    XVisualInfo *visinfo;
    int template_mask;
    int numvisuals;


    XColor bg, fg;
    
    XImage *testimage;


    EnterFunc(bug("initx11stuff()\n"));


LX11	

    data->screen = DefaultScreen( data->display );

    data->depth  = DisplayPlanes( data->display, data->screen );
    data->colmap = DefaultColormap( data->display, data->screen );


    /* Get some info on the display */
    template.visualid = XVisualIDFromVisual(DefaultVisual(data->display, data->screen));
    template_mask = VisualIDMask;

    visinfo = XGetVisualInfo(data->display, template_mask, &template, &numvisuals);

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
#if 0
	kprintf("Using visualid %d:\n", (int)(visinfo->visualid));
	kprintf("	screen %d\n", visinfo->screen);
	kprintf("	red_mask 0x%x\n", (int)(visinfo->red_mask));
	kprintf("	green_mask 0x%x\n", (int)(visinfo->green_mask));
	kprintf("	blue_mask 0x%x\n", (int)(visinfo->blue_mask));
	kprintf("	colormap_size %d\n", visinfo->colormap_size);
	kprintf("	bits_per_rgb %d\n", visinfo->bits_per_rgb);
	kill(getpid(), SIGSTOP);
#endif	

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
	    	/* Do nothing for now */
	    	break;
		
	    default:
	    	kprintf("GFX HIDD only supports truecolor and pseudocolor diplays for now\n");
	    	kill(getpid(), SIGSTOP);
	}

	xsd->size = 0;

	/* Create a dummy X image to get bits per pixel */
	testimage = XGetImage(xsd->display
		, RootWindow(xsd->display, data->screen)
		, 0, 0, 1, 1
		, AllPlanes, ZPixmap
	);
	
	if (NULL != testimage)
	{
	    xsd->size = testimage->bits_per_pixel;
	    XDestroyImage(testimage);
	}
	else
	{
	    kprintf("!!! X11gfx could not get bits per pixel\n");
	    kill(getpid(), SIGSTOP);
	}
	
	xsd->bytes_per_pixel = (xsd->size + 7) >> 3;
	
	if (PseudoColor == xsd->vi.class) {
	    xsd->clut_mask  = (1L << xsd->size) - 1;
	    xsd->clut_shift = 0;
	}

#if 0	
kprintf("Red:   mask: %p, shift: %d\n", xsd->vi.red_mask,   xsd->red_shift);
kprintf("Green: mask: %p, shift: %d\n", xsd->vi.green_mask, xsd->green_shift);
kprintf("Blue:  mask: %p, shift: %d\n", xsd->vi.blue_mask,  xsd->blue_shift);

kill(getpid(), SIGSTOP);
#endif
	
	
    }




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

 
    /* Create a dummy window for pixmaps */

    xsd->dummy_window_for_creating_pixmaps = XCreateSimpleWindow(
   	     data->display
   	   , DefaultRootWindow(data->display)
   	   , 0, 0
   	   , 100, 100
   	   , 0
   	   , BlackPixel(data->display, data->screen)
   	   , BlackPixel(data->display, data->screen)
    );


    if (0 == xsd->dummy_window_for_creating_pixmaps)
    {
	ok = FALSE;
    }
#if USE_XSHM
    	    
    /* Do we have Xshm support ? */
    xsd->xshm_info = init_shared_mem(data->display);
    if (NULL == xsd->xshm_info)
    {
    	ok = FALSE;
kprintf("INITIALIZATION OF XSHM FAILED !!\n");	    
    }
    else
    {

    	InitSemaphore(&xsd->shm_sema);
    	xsd->use_xshm = TRUE;
    }    	
#endif

	
UX11
    
    ReturnBool("initx11stuff", ok);

}

static VOID cleanupx11stuff(struct gfx_data *data, struct x11_staticdata *xsd)
{
LX11
    /* Do nothing for now */
    if (0 != xsd->dummy_window_for_creating_pixmaps)
    {
    	XDestroyWindow(data->display, xsd->dummy_window_for_creating_pixmaps);
    }
    
#if USE_XSHM
	cleanup_shared_mem(data->display, xsd->xshm_info);
#endif    
UX11
    return;
}

