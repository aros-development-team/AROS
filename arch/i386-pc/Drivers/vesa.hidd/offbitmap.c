/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Offscreen bitmap class for Vesa hidd.
    Lang: English.
*/


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
#define DEBUG 0
#include <aros/debug.h>

#include "bitmap.h"
#include "offbitmap.h"
#include "vesagfxclass.h"

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

#define MNAME(x) vesagfxoffbitmap_ ## x

#include "bitmap_common.c"

/*********** BitMap::New() *************************************/
static OOP_Object *MNAME(new)(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
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
	OOP_GetAttr(o, aHidd_BitMap_PixFmt, (IPTR *)&pf);
	OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);
	OOP_GetAttr(pf, aHidd_PixFmt_BytesPerPixel, &multi);
	
	/* Get the friend bitmap. This should be a displayable bitmap */
	OOP_GetAttr(o, aHidd_BitMap_Friend, (IPTR *)&friend);
	
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
static VOID MNAME(dispose)(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
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

#undef XSD
#define XSD(cl) xsd

#define NUM_ROOT_METHODS   3
#define NUM_BITMAP_METHODS 8

OOP_Class *init_vesagfxoffbmclass(struct VesaGfx_staticdata *xsd)
{
    struct OOP_MethodDescr root_descr[NUM_ROOT_METHODS + 1] =
    {
	{(IPTR (*)())MNAME(new)     , moRoot_New    },
	{(IPTR (*)())MNAME(dispose) , moRoot_Dispose},
	{(IPTR (*)())MNAME(get)     , moRoot_Get    },
	{NULL	    	    	    , 0UL   	    }
    };
    struct OOP_MethodDescr bitMap_descr[NUM_BITMAP_METHODS + 1] =
    {
	{(IPTR (*)())MNAME(putpixel)	    	, moHidd_BitMap_PutPixel    	    },
	{(IPTR (*)())MNAME(getpixel)	    	, moHidd_BitMap_GetPixel    	    },
	{(IPTR (*)())MNAME(fillrect)	    	, moHidd_BitMap_FillRect    	    },
	{(IPTR (*)())MNAME(putimage)	    	, moHidd_BitMap_PutImage    	    },
	{(IPTR (*)())MNAME(getimage)	    	, moHidd_BitMap_GetImage    	    },
    	{(IPTR (*)())MNAME(putimagelut)     	, moHidd_BitMap_PutImageLUT 	    },
    	{(IPTR (*)())MNAME(blitcolorexpansion)	, moHidd_BitMap_BlitColorExpansion  },
    	{(IPTR (*)())MNAME(puttemplate)	    	, moHidd_BitMap_PutTemplate 	    },
	{NULL	    	    	    	    	, 0UL	    	    	    	    }
    };
    struct OOP_InterfaceDescr ifdescr[] =
    {
	{root_descr 	, IID_Root          , NUM_ROOT_METHODS	},
	{bitMap_descr	, IID_Hidd_BitMap   , NUM_BITMAP_METHODS},
	{NULL	    	, NULL	    	    , 0     	    	}
    };
    OOP_AttrBase MetaAttrBase = OOP_ObtainAttrBase(IID_Meta);
    struct TagItem tags[] =
    {
	{aMeta_SuperID	    	, (IPTR) CLID_Hidd_BitMap   	    },
	{aMeta_InterfaceDescr	, (IPTR) ifdescr    	    	    },
	{aMeta_InstSize     	, (IPTR) sizeof(struct BitmapData)  },
	{TAG_DONE   	    	, 0UL	    	    	    	    }
    };
    OOP_Class *cl = NULL;

    EnterFunc(bug("init_vesagfxoffbmclass(xsd=%p)\n", xsd));
    D(bug("Metattrbase: %x\n", MetaAttrBase));
    
    if(MetaAttrBase)
    {
	D(bug("Got attrbase\n"));
	
	cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
	if(cl)
	{
	    D(bug("BitMap class ok\n"));
	    
	    xsd->offbmclass = cl;
	    cl->UserData     = (APTR) xsd;
	    
	    /* Get attrbase for the BitMap interface */
	    if (OOP_ObtainAttrBases(attrbases))
	    {
		OOP_AddClass(cl);
	    }
	    else
	    {
		free_vesagfxoffbmclass( xsd );
		cl = NULL;
	    }
	}
	
	/* We don't need this anymore */
	OOP_ReleaseAttrBase(IID_Meta);
	
    } /* if(MetaAttrBase) */
    
    ReturnPtr("init_vesagfxoffbmclass", OOP_Class *,  cl);
}

/*** free_offbitmapclass *********************************************************/
void free_vesagfxoffbmclass(struct VesaGfx_staticdata *xsd)
{
    EnterFunc(bug("free_vesagfxoffbmclass(xsd=%p)\n", xsd));

    if(xsd)
    {
	OOP_RemoveClass(xsd->offbmclass);
	if(xsd->offbmclass)
	    OOP_DisposeObject((OOP_Object *) xsd->offbmclass);	    
	xsd->offbmclass = NULL;
	
	OOP_ReleaseAttrBases(attrbases);
    }
    ReturnVoid("free_vesagfxoffbmclass");
}

