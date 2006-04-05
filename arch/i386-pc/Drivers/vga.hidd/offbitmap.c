/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Offscreen bitmap class for VGA hidd.
    Lang: English.
*/

#define __OOP_NOATTRBASES__

#include <proto/oop.h>
#include <proto/utility.h>

#include <exec/memory.h>
#include <exec/lists.h>

#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <oop/oop.h>
#include <exec/alerts.h>

#include <aros/symbolsets.h>

#include <hidd/graphics.h>

#include <assert.h>

#include "vga.h"
#include "vgaclass.h"

#include LC_LIBDEFS_FILE

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#include "bitmap.h"

/* Don't initialize them with "= 0", otherwise they end up in the DATA segment! */

static OOP_AttrBase HiddBitMapAttrBase;
static OOP_AttrBase HiddPixFmtAttrBase;
static OOP_AttrBase HiddGfxAttrBase;
static OOP_AttrBase HiddVGAGfxAB;
static OOP_AttrBase HiddVGABitMapAB;

static struct OOP_ABDescr attrbases[] = 
{
    { IID_Hidd_BitMap,		&HiddBitMapAttrBase },
    { IID_Hidd_PixFmt,		&HiddPixFmtAttrBase },
    { IID_Hidd_Gfx,		&HiddGfxAttrBase },
    /* Private bases */
    { IID_Hidd_VGAgfx,		&HiddVGAGfxAB	},
    { IID_Hidd_VGABitMap,	&HiddVGABitMapAB },
    { NULL, NULL }
};

void free_offbmclass(struct vga_staticdata *);
void vgaRefreshArea(struct bitmap_data *, int , struct Box *);

#define MNAME_ROOT(x) PCVGAOffBM__Root__ ## x
#define MNAME_BM(x) PCVGAOffBM__Hidd_BitMap__ ## x

#include "bitmap_common.c"

/*********** BitMap::New() *************************************/

OOP_Object *PCVGAOffBM__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    EnterFunc(bug("VGAGfx.BitMap::New()\n"));
    
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
    	struct bitmap_data *data;
        IPTR width, height, depth;
	
	OOP_Object *friend, *pf;
	
        data = OOP_INST_DATA(cl, o);
	
	/* clear all data  */
        memset(data, 0, sizeof(struct bitmap_data));
	
	/* Get attr values */
	OOP_GetAttr(o, aHidd_BitMap_Width,		&width);
	OOP_GetAttr(o, aHidd_BitMap_Height, 	&height);
#if 0
/* nlorentz: The aHidd_BitMap_Depth attribute no loner exist,, so we must
	get the depth in two steps: First get pixel format, then get depth */
	OOP_GetAttr(o, aHidd_BitMap_Depth,		&depth);
#else
	OOP_GetAttr(o,  aHidd_BitMap_PixFmt,	(IPTR *)&pf);
	OOP_GetAttr(pf, aHidd_PixFmt_Depth,		&depth);
#endif
	
	/* Get the friend bitmap. This should be a displayable bitmap */
	OOP_GetAttr(o, aHidd_BitMap_Friend,	(IPTR *)&friend);

	/* If you got a friend bitmap, copy its colormap */
	if (friend)
	{
	    struct bitmap_data *src = OOP_INST_DATA(cl, friend);
	    
	    CopyMem(&src->cmap, &data->cmap, 4*16);
	}
	

	ASSERT (width != 0 && height != 0 && depth != 0);
	
	/* 
	   We must only create depths that are supported by the friend drawable
	   Currently we only support the default depth
	*/
	
/* nlorentz: With the new HIDD design we decided in Gfx::NewBitMap()
    that we should only create bitmaps that are alike to the friend bitmap.
    Thus the test below is really not necessary, as we will allways
    get the same depth
*/
	if (depth != 4)
	{
//	    depth = 4;	/* Do anything... */
	}

#if 0
    /* nlorentz: Not necessary with the new design */	
	/* Update the depth to the one we use */
	depth_tags[0].ti_Data = depth;
	SetAttrs(o, depth_tags);
#endif
	data->width = width;
	data->height = height;
	data->bpp = depth;
	data->disp = 0;
	width=(width+15) & ~15;
	data->VideoData = AllocVec(width*height,MEMF_PUBLIC|MEMF_CLEAR);
	if (data->VideoData)
	{
	    data->Regs = AllocVec(sizeof(struct vgaHWRec),MEMF_PUBLIC|MEMF_CLEAR);
	    if (data->Regs)
	    {
#if 0
    /* nlorentz: Not necessary nor possible with the new design */
		set_pixelformat(o);
#endif
		if (XSD(cl)->activecallback)
		    XSD(cl)->activecallback(XSD(cl)->callbackdata, o, TRUE);

		ReturnPtr("VGAGfx.BitMap::New()", Object *, o);
	    }
	} /* if got data->VideoData */

	{
	    OOP_MethodID disp_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
    	    OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);
	}
	
	o = NULL;
    } /* if created object */

    ReturnPtr("VGAGfx.BitMap::New()", OOP_Object *, o);
}

/**********  Bitmap::Dispose()  ***********************************/

VOID PCVGAOffBM__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    EnterFunc(bug("VGAGfx.BitMap::Dispose()\n"));
    
    if (data->VideoData)
	FreeVec(data->VideoData);
    if (data->Regs)
	FreeVec(data->Regs);
	
    OOP_DoSuperMethod(cl, o, msg);
    
    ReturnVoid("VGAGfx.BitMap::Dispose");
}


#undef SDEBUG
#undef DEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>



/*** init_bmclass *********************************************************/

AROS_SET_LIBFUNC(PCVGAOffBM_Init, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    EnterFunc(bug("PCVGAOffBM_Init\n"));
    
    ReturnInt("PCVGAOffBM_Init", ULONG, OOP_ObtainAttrBases(attrbases));
    
    AROS_SET_LIBFUNC_EXIT
}

/*** expunge_onbmclass *******************************************************/

AROS_SET_LIBFUNC(PCVGAOffBM_Expunge, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    EnterFunc(bug("PCVGAOffBM_Expunge\n"));

    OOP_ReleaseAttrBases(attrbases);
    ReturnInt("PCVGAOffBM_Expunge", ULONG, TRUE);
    
    AROS_SET_LIBFUNC_EXIT
}

/*****************************************************************************/

ADD2INITLIB(PCVGAOffBM_Init, 0)
ADD2EXPUNGELIB(PCVGAOffBM_Expunge, 0)
