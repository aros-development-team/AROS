/*
    (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc: Class for VGA and compatible cards.
    Lang: English.
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>

#include <exec/alerts.h>
#include <exec/memory.h>

#include <hidd/hidd.h>
#include <hidd/graphics.h>

#include <aros/system.h>
#include <aros/machine.h>
#include <aros/asmcall.h>

#include <hardware/custom.h>

#include <devices/inputevent.h>

#include "vga.h"
#include "vgaclass.h"

#define DEBUG 0
#include <aros/debug.h>

/* Some attrbases needed as global vars.
  These are write-once read-many */

static AttrBase HiddBitMapAttrBase 	= 0;  
static AttrBase HiddPixFmtAttrBase	= 0;
static AttrBase HiddSyncAttrBase	= 0;
static AttrBase HiddVGAAB 		= 0;

static struct ABDescr attrbases[] =
{
    { IID_Hidd_BitMap,	&HiddBitMapAttrBase	},
    { IID_Hidd_VGAgfx,	&HiddVGAAB		},
    { IID_Hidd_PixFmt,	&HiddPixFmtAttrBase	},
    { IID_Hidd_Sync,	&HiddSyncAttrBase	},
    { NULL, NULL }
};

struct vga_data
{
    int	i;	//dummy!!!!!!!!!
};

/* Default graphics modes */

struct vgaModeDesc
    vgaDefMode[NUM_MODES]={
		{"640x480x4 @ 60Hz",	// h: 31.5 kHz v: 60Hz
		640,480,4,0,
		0,
		640,664,760,800,0,
		480,491,493,525},
		{"768x576x4 @ 54Hz",	// h: 32.5 kHz v: 54Hz
		768,576,4,1,
		0,
		768,795,805,872,0,
		576,577,579,600},
		{"800x600x4 @ 52Hz",	// h: 31.5 kHz v: 52Hz
		800,600,4,1,
		0,
		800,826,838,900,0,
		600,601,603,617}
		};

/* Default mouse shape */

UBYTE shape[] = 
{
    06,02,00,00,00,00,00,00,00,00,00
,
    01,06,02,02,00,00,00,00,00,00,00
,
    00,01,06,06,02,02,00,00,00,00,00
,
    00,01,06,06,06,06,02,02,00,00,00
,
    00,00,01,06,06,06,06,06,02,02,00
,
    00,00,01,06,06,06,06,06,06,06,00
,
    00,00,00,01,06,06,06,02,00,00,00
,
    00,00,00,01,06,06,01,06,02,00,00
,
    00,00,00,00,01,06,00,01,06,02,00
,
    00,00,00,00,01,06,00,00,01,06,02
,
    00,00,00,00,00,00,00,00,00,01,06
};

/*********************
**  GfxHidd::New()  **
*********************/

#define NUM_SYNC_TAGS 10
#define SET_SYNC_TAG(taglist, idx, tag, val) 	\
    taglist[idx].ti_Tag  = aHidd_Sync_ ## tag;	\
    taglist[idx].ti_Data = val

VOID init_sync_tags(struct TagItem *tags, struct vgaModeDesc *md)
{
#warning There does not sem to be a clock value set in vgaDefMode
#warning Note that PixelClock is frequency in Hz. PixelTime is time to draw a pixel in picoseconds
#warning I do not know which of these vgaModeDesc->clock is.
    SET_SYNC_TAG(tags, 0, PixelClock, 	md->clock	);
    SET_SYNC_TAG(tags, 1, HDisp, 	md->HDisplay	);
    SET_SYNC_TAG(tags, 2, VDisp, 	md->VDisplay	);
    SET_SYNC_TAG(tags, 3, HSyncStart, 	md->HSyncStart	);
    SET_SYNC_TAG(tags, 4, HSyncEnd, 	md->HSyncEnd	);
    SET_SYNC_TAG(tags, 5, HTotal, 	md->HTotal	);
    SET_SYNC_TAG(tags, 6, VSyncStart,	md->VSyncStart	);
    SET_SYNC_TAG(tags, 7, VSyncEnd, 	md->VSyncEnd	);
    SET_SYNC_TAG(tags, 8, VTotal, 	md->VTotal	);
    tags[9].ti_Tag = TAG_DONE;
}

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
	{ aHidd_PixFmt_CLUTMask,	0x0f	}, /* 14 */
	{ aHidd_PixFmt_BitMapType,	0	}, /* 15 */
	{ TAG_DONE, 0UL }
    };

#if 1 
    struct TagItem sync_640_480[NUM_SYNC_TAGS];
    struct TagItem sync_758_576[NUM_SYNC_TAGS];
    struct TagItem sync_800_600[NUM_SYNC_TAGS];
    
    struct TagItem modetags[] = {
	{ aHidd_Gfx_PixFmtTags,	(IPTR)pftags		},
	{ aHidd_Gfx_SyncTags,	(IPTR)sync_640_480	},
	{ aHidd_Gfx_SyncTags,	(IPTR)sync_758_576	},
	{ aHidd_Gfx_SyncTags,	(IPTR)sync_800_600	},
	{ TAG_DONE, 0UL }
    };
    
    struct TagItem mytags[] = {
	{ aHidd_Gfx_ModeTags,	(IPTR)modetags	},
	{ TAG_MORE, 0UL }
    };
    struct pRoot_New mymsg;

    /* Init the pixel format */
    pftags[0].ti_Data = 0;
    pftags[1].ti_Data = 0;
    pftags[2].ti_Data = 0;
    pftags[3].ti_Data = 0;

    pftags[4].ti_Data = 0x00003f;
    pftags[5].ti_Data = 0x003f00;
    pftags[6].ti_Data = 0x3f0000;
    pftags[7].ti_Data = 0;
	
    pftags[8].ti_Data = vHidd_ColorModel_Palette;
    pftags[9].ti_Data = 4;
    pftags[10].ti_Data = 1;
    pftags[11].ti_Data = 4;
    pftags[12].ti_Data = vHidd_StdPixFmt_Native;

#warning Is this true in all cases ?
    pftags[15].ti_Data = vHidd_BitMapType_Chunky;

    
    /* First init the sync tags */
    init_sync_tags(sync_640_480, &vgaDefMode[0]);
    init_sync_tags(sync_758_576, &vgaDefMode[1]);
    init_sync_tags(sync_800_600, &vgaDefMode[2]);
    
    /* init mytags. We use TAG_MORE to attach our own tags before we send them
    to the superclass */
    mytags[1].ti_Tag  = TAG_MORE;
    mytags[1].ti_Data = (IPTR)msg->attrList;
    
    /* Init mymsg. We have to use our own message struct because
       one should not alter the one passed to this method.
       message structs passed to a method are allways read-only.
       (The user who called us might want to reuse the same msg struct
       for several calls, but that will break if som method changes the
       msg struct contents)
    */
    mymsg.mID	= msg->mID;	/* We got New() method and we are sending 
				   the same method to the superclass	*/
    mymsg.attrList = mytags;
    
	
    msg = &mymsg;
#else

   
    struct TagItem tags_640_480[] = {
    	{ aHidd_GfxMode_Width,		640	},
	{ aHidd_GfxMode_Height,		480	},
	{ aHidd_GfxMode_PixFmtTags,	(IPTR)pftags	},
	{ TAG_DONE, 0UL }
    };

    struct TagItem tags_768_576[] = {
    	{ aHidd_GfxMode_Width,		768	},
	{ aHidd_GfxMode_Height,		576	},
	{ aHidd_GfxMode_PixFmtTags,	(IPTR)pftags	},
	{ TAG_DONE, 0UL }
    };

    struct TagItem tags_800_600[] = {
    	{ aHidd_GfxMode_Width,		800	},
	{ aHidd_GfxMode_Height,		600	},
	{ aHidd_GfxMode_PixFmtTags,	(IPTR)pftags	},
	{ TAG_DONE, 0UL }
    };
    
    struct TagItem *mode_tags[] = {
	tags_640_480, tags_768_576, tags_800_600, NULL
    };
#endif

    EnterFunc(bug("VGAGfx::New()\n"));
    


    o = (Object *)DoSuperMethod(cl, o, (Msg)msg);
    if (o)
    {
	D(bug("Got object from super\n"));

#if 0
       /* nlorentz: This is only needed for hidds that run on a window system */
       
          
	XSD(cl)->activecallback = (VOID (*)())GetTagData(aHidd_Gfx_ActiveBMCallBack, (IPTR)NULL, msg->attrList);
	XSD(cl)->callbackdata = (APTR)GetTagData(aHidd_Gfx_ActiveBMCallBackData, (IPTR)NULL, msg->attrList);

#endif

	XSD(cl)->mouseW = 11;
	XSD(cl)->mouseH = 11;
	XSD(cl)->mouseShape = shape;
	XSD(cl)->mouseVisible = 1;

	XSD(cl)->vgahidd = o;
	
	ReturnPtr("VGAGfx::New", Object *, o);
	
    }
    ReturnPtr("VGAGfx::New", Object *, NULL);
}

static VOID gfx_dispose(Class *cl, Object *o, Msg msg)
{
    DoSuperMethod(cl, o, (Msg)msg);
    return;
}

static VOID gfx_get(Class *cl, Object *o, struct pRoot_Get *msg)
{
    DoSuperMethod(cl, o, (Msg)msg);
    return;
}

/********** GfxHidd::NewBitMap()  ****************************/
static Object *gfxhidd_newbitmap(Class *cl, Object *o, struct pHidd_Gfx_NewBitMap *msg)
{

    BOOL displayable;
    struct vga_data *data;
    Class *classptr = NULL;
    struct TagItem mytags[2];
    struct pHidd_Gfx_NewBitMap mymsg;
    
    EnterFunc(bug("VGAGfx::NewBitMap()\n"));
    
    
    
    data = INST_DATA(cl, o);
    
    
    
    /* Displayable bitmap ? */
    displayable = GetTagData(aHidd_BitMap_Displayable, FALSE, msg->attrList);
    if (displayable) {
	/* If the user asks for a displayable bitmap we must ALLWAYS supply a class */
    	classptr = XSD(cl)->onbmclass;
    } else {
	HIDDT_ModeID modeid;
	/* 
	    For the non-displayable case we can either supply a class ourselves
	    if we can optimize a certain type of non-displayable bitmaps. Or we
	    can let the superclass create on for us.
	   
	    The attributes that might come from the user deciding the bitmap
	    pixel format are:
		- aHidd_BitMap_ModeID:	a modeid. create a nondisplayable
			bitmap with the size  and pixelformat of a gfxmode.
		- aHidd_BitMap_StdPixFmt: a standard pixelformat as described in
			hidd/graphics.h
		- aHidd_BitMap_Friend: if this is supplied and none of the two above
		    are supplied, then the pixel format of the created bitmap
		    will be the same as the one of the friend bitmap.
		    
	    These tags are listed in prioritized order, so if
	    the user supplied a ModeID tag, then you should not care about StdPixFmt
	    or Friend. If there is no ModeID, but a StdPixFmt tag supplied,
	    then you should not care about Friend because you have to
	    create the correct pixelformat. And as said above, if only Friend
	    is supplied, you can create a bitmap with same pixelformat as Frien
	*/
	
	
	modeid = (HIDDT_ModeID)GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);
	if (vHidd_ModeID_Invalid != modeid) {
	    /* User supplied a valid modeid. We can use our offscreen class */
	    classptr = XSD(cl)->offbmclass;
	} else {
	    /* We may create an offscreen bitmap if the user supplied a friend
	       bitmap. But we need to check that he did not supplied a StdPixFmt
	    */
	    HIDDT_StdPixFmt stdpf;
	    stdpf = (HIDDT_StdPixFmt)GetTagData(aHidd_BitMap_StdPixFmt, vHidd_StdPixFmt_Unknown, msg->attrList);
	    if (vHidd_StdPixFmt_Unknown == stdpf) {
		/* No std pixfmt supplied */
		Object *friend;
	    
		/* Did the user supply a friend bitmap ? */
		friend = (Object *)GetTagData(aHidd_BitMap_Friend, 0, msg->attrList);
		if (NULL != friend) {
		    Object * gfxhidd;
		    /* User supplied friend bitmap. Is the friend bitmap a
		    VGA Gfx hidd bitmap ? */
		    GetAttr(friend, aHidd_BitMap_GfxHidd, (IPTR *)&gfxhidd);
		    if (gfxhidd == o) {
			/* Friend was VGA hidd bitmap. Now we can supply our own class */
			classptr = XSD(cl)->offbmclass;		    
		    }
		}
	    }
	}
    }
    
    /* Do we supply our own class ? */
    if (NULL != classptr) {
	/* Yes. We must let the superclass not that we do this. This is
	   done through adding a tag in the frot of the taglist */
	mytags[0].ti_Tag	= aHidd_BitMap_ClassPtr;
	mytags[0].ti_Data	= (IPTR)classptr;
	mytags[1].ti_Tag	= TAG_MORE;
	mytags[1].ti_Data	= (IPTR)msg->attrList;
	
	/* Like in Gfx::New() we init a new message struct */
	mymsg.mID	= msg->mID;
	mymsg.attrList	= mytags;
	
	/* Pass the new message to the superclass */
	msg = &mymsg;
    }

    ReturnPtr("VGAGfx::NewBitMap", Object *, (Object *)DoSuperMethod(cl, o, (Msg)msg));
}

/********** GfxHidd::SetMouseShape()  ****************************/

static VOID gfxhidd_setmouseshape(Class *cl, Object *o, struct pHidd_Gfx_SetMouseShape *msg)
{
    if (msg->shape)
    {
	XSD(cl)->mouseW = msg->width;
	XSD(cl)->mouseH = msg->height;
	XSD(cl)->mouseShape = msg->shape;
    }
    else
    {
	XSD(cl)->mouseW = 11;
	XSD(cl)->mouseH = 11;
	XSD(cl)->mouseShape = shape;
    }
    
    draw_mouse(XSD(cl));
}

/********** GfxHidd::SetMouseXY()  ****************************/

static VOID gfxhidd_setmousexy(Class *cl, Object *o, struct pHidd_Gfx_SetMouseXY *msg)
{
    struct Box box = {0, 0, 0, 0};

    box.x1 = XSD(cl)->mouseX;
    box.y1 = XSD(cl)->mouseY;
    box.x2 = box.x1 + XSD(cl)->mouseW;
    box.y2 = box.y1 + XSD(cl)->mouseH;

    if (XSD(cl)->visible)
	vgaRefreshArea(XSD(cl)->visible, 1, &box);

    XSD(cl)->mouseX += msg->dx;
    XSD(cl)->mouseY += msg->dy;

    if (XSD(cl)->visible)
    {
        if (XSD(cl)->mouseX < 0) XSD(cl)->mouseX = 0;
	if (XSD(cl)->mouseY < 0) XSD(cl)->mouseY = 0;
	if (XSD(cl)->mouseX >= XSD(cl)->visible->width) XSD(cl)->mouseX = XSD(cl)->visible->width - 1;
	if (XSD(cl)->mouseY >= XSD(cl)->visible->height) XSD(cl)->mouseY = XSD(cl)->visible->height - 1;
    }
    
    draw_mouse(XSD(cl));
}

/********** GfxHidd::SetMouseVisible()  ****************************/

static VOID gfxhidd_showhide(Class *cl, Object *o, struct pHidd_Gfx_ShowHide *msg)
{
    XSD(cl)->mouseVisible = msg->visible;
    
    draw_mouse(XSD(cl));
}


#undef XSD
#define XSD(cl) xsd

/********************  init_vgaclass()  *********************************/

#define NUM_ROOT_METHODS 3
#define NUM_VGA_METHODS 4

Class *init_vgaclass (struct vga_staticdata *xsd)
{
    Class *cl = NULL;

    struct MethodDescr root_descr[NUM_ROOT_METHODS + 1] = 
    {
    	{(IPTR (*)())gfx_new,		moRoot_New},
    	{(IPTR (*)())gfx_dispose,	moRoot_Dispose},
    	{(IPTR (*)())gfx_get,		moRoot_Get},
	{NULL, 0UL}
    };
    
    struct MethodDescr vgahidd_descr[NUM_VGA_METHODS + 1] = 
    {
    	{(IPTR (*)())gfxhidd_newbitmap,		moHidd_Gfx_NewBitMap},
	{(IPTR (*)())gfxhidd_setmouseshape,	moHidd_Gfx_SetMouseShape},
	{(IPTR (*)())gfxhidd_setmousexy,	moHidd_Gfx_SetMouseXY},
	{(IPTR (*)())gfxhidd_showhide,		moHidd_Gfx_ShowHide},
	{NULL, 0UL}
    };
    
    struct InterfaceDescr ifdescr[] =
    {
    	{root_descr, 	IID_Root, 		NUM_ROOT_METHODS},
    	{vgahidd_descr, IID_Hidd_Gfx,	 	NUM_VGA_METHODS},
	{NULL, NULL, 0}
    };
    
    AttrBase MetaAttrBase = ObtainAttrBase(IID_Meta);
	
    struct TagItem tags[] =
    {
	{ aMeta_SuperID,		(IPTR)CLID_Hidd_Gfx},
	{ aMeta_InterfaceDescr,		(IPTR)ifdescr},
	{ aMeta_InstSize,		(IPTR)sizeof (struct vga_data) },
	{ aMeta_ID,			(IPTR)CLID_Hidd_VGAgfx },
	{TAG_DONE, 0UL}
    };

    EnterFunc(bug("VgaHiddClass init\n"));
    
    if (MetaAttrBase)
    {
    	cl = NewObject(NULL, CLID_HiddMeta, tags);
    	if(cl)
    	{
	    cl->UserData = (APTR)xsd;
	    xsd->vgaclass = cl;
	    
	    if (ObtainAttrBases(attrbases))
	    {
		D(bug("VgaHiddClass ok\n"));
		
	    	AddClass(cl);
	    }
	    else
	    {
	    	free_vgaclass(xsd);
		cl = NULL;
	    }
	}
	/* Don't need this anymore */
	ReleaseAttrBase(IID_Meta);
    }
    ReturnPtr("init_vgaclass", Class *, cl);
}

/*************** free_vgaclass()  **********************************/
VOID free_vgaclass(struct vga_staticdata *xsd)
{
    EnterFunc(bug("free_vgaclass(xsd=%p)\n", xsd));

    if(xsd)
    {
        RemoveClass(xsd->vgaclass);
	
        if(xsd->vgaclass) DisposeObject((Object *) xsd->vgaclass);
        xsd->vgaclass = NULL;
	
	ReleaseAttrBases(attrbases);
    }
    ReturnVoid("free_vgaclass");
}

#if 0
    /* nlorentz: This function is no longer necessary */
BOOL set_pixelformat(Object *bm)
{
    
    struct TagItem pf_tags[] = {
    	{ aHidd_PixFmt_RedShift,	0	}, /* 0 */
	{ aHidd_PixFmt_GreenShift,	0	}, /* 1 */
	{ aHidd_PixFmt_BlueShift,  	0	}, /* 2 */
	{ aHidd_PixFmt_AlphaShift,	0	}, /* 3 */
	{ aHidd_PixFmt_RedMask,		0x00003f}, /* 4 */
	{ aHidd_PixFmt_GreenMask,	0x003f00}, /* 5 */
	{ aHidd_PixFmt_BlueMask,	0x3f0000}, /* 6 */
	{ aHidd_PixFmt_AlphaMask,	0	}, /* 7 */
	{ aHidd_PixFmt_GraphType,	vHidd_GT_Palette	}, /* 8 */
	{ aHidd_PixFmt_Depth,		4			}, /* 9 */
	{ aHidd_PixFmt_BytesPerPixel,	1			}, /* 10 */
	{ aHidd_PixFmt_BitsPerPixel,	4			}, /* 11 */
	{ aHidd_PixFmt_StdPixFmt,	vHidd_PixFmt_Native	}, /* 12 */
	{ aHidd_PixFmt_CLUTShift,	0x00	}, /* 13 */
	{ aHidd_PixFmt_CLUTMask,	0x0f	}, /* 13 */
	{ TAG_DONE, 0UL }
    };
    
    Object *pf;
    
    pf = HIDD_BM_SetPixelFormat(bm, pf_tags);
    if (NULL == pf) {
	return FALSE;
    }
    
    return TRUE;
}

#endif

void draw_mouse(struct vga_staticdata *xsd)
{
    int pix;
    unsigned char *ptr, *data;
    int x, y, width, fg, x_i, y_i;
    
    if (xsd->mouseVisible)
    {

        if (xsd->visible)
	{
	    /* Get display width */
	    width = xsd->visible->width;

    	    /* And pointer data */
    	    data = xsd->mouseShape;
    
    	    ObtainSemaphore(&xsd->HW_acc);

    	    outw(0x3c4,0x0f02);
	    outw(0x3ce,0x0005);
	    outw(0x3ce,0x0003);
	    outw(0x3ce,0x0f01);

	    for (y_i = 0, y = xsd->mouseY ; y_i < xsd->mouseH; y_i++, y++)
    	    {
    		for (x_i = 0, x = xsd->mouseX; x_i < xsd->mouseW; x_i++, x++)
		{
		    ptr = (char *)(0xa0000 + (x + (y * width)) / 8);
		    pix = 0x8000 >> (x % 8);
    
		    fg = (char)*data++;

		    if (fg && (x < width))
		    {
			outw(0x3ce,pix | 8);
			outw(0x3ce,(fg << 8));

			*ptr |= 1;		// This or'ed value isn't important
		    }
		}
	    }
	    
	    ReleaseSemaphore(&xsd->HW_acc);
	}
    }
}
