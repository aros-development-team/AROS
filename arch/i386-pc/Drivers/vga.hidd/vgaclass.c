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
static AttrBase HiddGfxModeAttrBase	= 0;
static AttrBase HiddVGAAB 		= 0;

static struct ABDescr attrbases[] =
{
    { IID_Hidd_BitMap, &HiddBitMapAttrBase },
    { IID_Hidd_VGAgfx, &HiddVGAAB },
    { IID_Hidd_GfxMode, &HiddGfxModeAttrBase },
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
	{ aHidd_PixFmt_CLUTShift,	0	}, /* 13 */
	{ aHidd_PixFmt_CLUTMask,	0x0f	}, /* 14 */
	{ TAG_DONE, 0UL }
    };
        
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

    EnterFunc(bug("VGAGfx::New()\n"));

    o = (Object *)DoSuperMethod(cl, o, (Msg)msg);
    if (o)
    {
    
	MethodID dispose_mid;
	
	D(bug("Got object from super\n"));

	XSD(cl)->activecallback = (VOID (*)())GetTagData(aHidd_Gfx_ActiveBMCallBack, (IPTR)NULL, msg->attrList);
	XSD(cl)->callbackdata = (APTR)GetTagData(aHidd_Gfx_ActiveBMCallBackData, (IPTR)NULL, msg->attrList);

	pftags[0].ti_Data = 0;
	pftags[1].ti_Data = 0;
	pftags[2].ti_Data = 0;
	pftags[3].ti_Data = 0;

	pftags[4].ti_Data = 0x00003f;
	pftags[5].ti_Data = 0x003f00;
	pftags[6].ti_Data = 0x3f0000;
	pftags[7].ti_Data = 0;
	
	pftags[8].ti_Data = vHidd_GT_Palette;
	pftags[9].ti_Data = 4;
	pftags[10].ti_Data = 1;
	pftags[11].ti_Data = 4;
	pftags[12].ti_Data = vHidd_PixFmt_Native;

	XSD(cl)->mouseW = 11;
	XSD(cl)->mouseH = 11;
	XSD(cl)->mouseShape = shape;
	XSD(cl)->mouseVisible = 1;
	
	if(HIDD_Gfx_RegisterGfxModes(o, mode_tags))
	{
	    ReturnPtr("VGAGfx::New", Object *, o);
	}
	
	D(bug("Disposing obj\n"));
	
	dispose_mid = GetMethodID(IID_Root, moRoot_Dispose);
	CoerceMethod(cl, o, (Msg)&dispose_mid);
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
    struct pHidd_Gfx_NewBitMap p;
    struct vga_data *data;
    
    EnterFunc(bug("VGAGfx::NewBitMap()\n"));
    
    data = INST_DATA(cl, o);
    
    /* Displayeable bitmap ? */
    
    displayable = GetTagData(aHidd_BitMap_Displayable, FALSE, msg->attrList);
    if (displayable) {
    	p.classPtr = XSD(cl)->onbmclass;
    } else {
	p.classPtr = XSD(cl)->offbmclass;
    }

    /* !!! IMPORTANT !!! */
    p.classID = NULL;
    p.mID = msg->mID;
    p.attrList = msg->attrList;

    ReturnPtr("VGAGfx::NewBitMap", Object *, (Object *)DoSuperMethod(cl, o, (Msg)&p));
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

    XSD(cl)->mouseX = msg->x;
    XSD(cl)->mouseY = msg->y;

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

	    for (y_i = 0, y = xsd->mouseY ; y_i < xsd->mouseH; y_i++, y++)
    	    {
    		for (x_i = 0, x = xsd->mouseX; x_i < xsd->mouseW; x_i++, x++)
		{
		    ptr = (char *)(0xa0000 + (x + (y * width)) / 8);
		    pix = 0x8000 >> (x % 8);
    
		    fg = (char)*data++;

		    if (fg && (x < width))
		    {
    			outw(0x3c4,0x0f02);
			outw(0x3ce,pix | 8);
			outw(0x3ce,0x0005);
			outw(0x3ce,0x0003);
			outw(0x3ce,(fg << 8));
			outw(0x3ce,0x0f01);

			*ptr |= 1;		// This or'ed value isn't important
		    }
		}
	    }
	    
	    ReleaseSemaphore(&xsd->HW_acc);
	}
    }
}
