/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Offscreen bitmap class for Vesa hidd.
    Lang: English.
*/

#define __OOP_NOATTRBASES__

#include <proto/oop.h>
#include <proto/utility.h>
#include <assert.h>
#include <exec/alerts.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <graphics/gfx.h>
#include <graphics/rastport.h>
#include <hidd/graphics.h>
#include <oop/oop.h>
#include <aros/symbolsets.h>
#define DEBUG 0
#include <aros/debug.h>

#include "bitmap.h"
#include "offbitmap.h"
#include "vesagfxclass.h"

#include LC_LIBDEFS_FILE

/* Don't initialize them with "= 0", otherwise they end up in the DATA segment! */

static OOP_AttrBase HiddBitMapAttrBase;
static OOP_AttrBase HiddPixFmtAttrBase;
static OOP_AttrBase HiddGfxAttrBase;
static OOP_AttrBase HiddVesaGfxAttrBase;
static OOP_AttrBase HiddVesaGfxBitMapAttrBase;

static struct OOP_ABDescr attrbases[] = 
{
    {IID_Hidd_BitMap	    , &HiddBitMapAttrBase   	    },
    {IID_Hidd_PixFmt	    , &HiddPixFmtAttrBase   	    },
    {IID_Hidd_Gfx   	    , &HiddGfxAttrBase	    	    },
    /* Private bases */
    {IID_Hidd_VesaGfx	    , &HiddVesaGfxAttrBase  	    },
    {IID_Hidd_VesaGfxBitMap , &HiddVesaGfxBitMapAttrBase    },
    {NULL   	    	    , NULL  	    	    	    }
};

#define MNAME_ROOT(x) PCVesaOffBM__Root__ ## x
#define MNAME_BM(x) PCVesaOffBM__Hidd_BitMap__ ## x

#include "bitmap_common.c"

/*********** BitMap::New() *************************************/
OOP_Object *MNAME_ROOT(New)(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{

    EnterFunc(bug("VesaGfx.BitMap::New()\n"));
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
	struct BitmapData   *data;
	IPTR 	    	     width, height, depth, multi;
	OOP_Object  	    *friend, *pf;
	
	data = OOP_INST_DATA(cl, o);
	
	/* clear all data  */
	memset(data, 0, sizeof(struct BitmapData));
	
	/* Get attr values */
	OOP_GetAttr(o, aHidd_BitMap_Width, &width);
	OOP_GetAttr(o, aHidd_BitMap_Height, &height);
	OOP_GetAttr(o, aHidd_BitMap_PixFmt, &pf);
	OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);
	OOP_GetAttr(pf, aHidd_PixFmt_BytesPerPixel, &multi);
	
	/* Get the friend bitmap. This should be a displayable bitmap */
	OOP_GetAttr(o, aHidd_BitMap_Friend, &friend);
	
	/* If you got a friend bitmap, copy its colormap */
	if (friend)
	{
	    struct BitmapData *src = OOP_INST_DATA(cl, friend);
	    CopyMem(&src->cmap, &data->cmap, 4*16);
	}
	
	ASSERT (width != 0 && height != 0 && depth != 0);
	
	width=(width+15) & ~15;
	data->width = width;
	data->height = height;
	data->bpp = depth;
	data->disp = 0;

	data->bytesperpix = multi;
	data->bytesperline = width * multi;
	
	data->VideoData = AllocVec(width*height*multi, MEMF_PUBLIC | MEMF_CLEAR);
	if (data->VideoData)
	{
	    data->data = &XSD(cl)->data;
	    
	    if (XSD(cl)->activecallback)
		XSD(cl)->activecallback(XSD(cl)->callbackdata, o, TRUE);
		
	    ReturnPtr("VesaGfx.BitMap::New()", OOP_Object *, o);
	} /* if got data->VideoData */
	
	{
	    OOP_MethodID disp_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
	    OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);
	}
	
	o = NULL;
	
    } /* if created object */
    
    ReturnPtr("VesaGfx.BitMap::New()", OOP_Object *, o);
}

/**********  Bitmap::Dispose()  ***********************************/
VOID MNAME_ROOT(Dispose)(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);

    EnterFunc(bug("VesaGfx.BitMap::Dispose()\n"));
    
    if (data->VideoData)
	FreeVec(data->VideoData);
	
    OOP_DoSuperMethod(cl, o, msg);
    
    ReturnVoid("VesaGfx.BitMap::Dispose");
}

#undef SDEBUG
#undef DEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

/*** init_bmclass *********************************************************/

static int PCVesaOffBM_Init(LIBBASETYPEPTR LIBBASE)
{
    EnterFunc(bug("PCVesaOffBM_Init\n"));
    
    ReturnInt("PCVesaOffBM_Init", ULONG, OOP_ObtainAttrBases(attrbases));
}

/*** free_bitmapclass *********************************************************/

static int PCVesaOffBM_Expunge(LIBBASETYPEPTR LIBBASE)
{
    OOP_ReleaseAttrBases(attrbases);
    ReturnInt("PCVesaOffBM_Expunge", int, TRUE);
}

ADD2INITLIB(PCVesaOffBM_Init, 0)
ADD2EXPUNGELIB(PCVesaOffBM_Expunge, 0)
