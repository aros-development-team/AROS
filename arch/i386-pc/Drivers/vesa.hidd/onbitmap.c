/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Bitmap class for Vesa hidd.
    Lang: English.
*/


#include <proto/oop.h>
#include <proto/utility.h>
#include <assert.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <hidd/graphics.h>
#include <oop/oop.h>
#define DEBUG 0
#include <aros/debug.h>

#include "onbitmap.h"
#include "bitmap.h"
#include "vesagfxclass.h"

/* Don't initialize static variables with "=0", otherwise they go into DATA segment */

static OOP_AttrBase HiddBitMapAttrBase;
static OOP_AttrBase HiddPixFmtAttrBase;
static OOP_AttrBase HiddGfxAttrBase;
static OOP_AttrBase HiddSyncAttrBase;
static OOP_AttrBase HiddVesaGfxAttrBase;
static OOP_AttrBase HiddVesaGfxBitMapAttrBase;

static struct OOP_ABDescr attrbases[] = 
{
    { IID_Hidd_BitMap	    , &HiddBitMapAttrBase   	},
    { IID_Hidd_PixFmt	    , &HiddPixFmtAttrBase   	},
    { IID_Hidd_Gfx  	    , &HiddGfxAttrBase      	},
    { IID_Hidd_Sync 	    , &HiddSyncAttrBase     	},
    /* Private bases */
    { IID_Hidd_VesaGfx	    , &HiddVesaGfxAttrBase  	},
    { IID_Hidd_VesaGfxBitMap, &HiddVesaGfxBitMapAttrBase},
    { NULL  	    	    , NULL  	    	    	}
};

#define MNAME(x) vesagfxonbitmap_ ## x

#define OnBitmap 1
#include "bitmap_common.c"

/*********** BitMap::New() *************************************/
static OOP_Object *MNAME(new)(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    EnterFunc(bug("VesaGfx.BitMap::New()\n"));
    
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
	struct BitmapData   *data;
	OOP_Object  	    *pf;
	IPTR 	    	     width, height, depth, multi;
	HIDDT_ModeID 	     modeid;
	OOP_Object  	    *sync;
	ULONG 	    	     pixelc;

	data = OOP_INST_DATA(cl, o);
	
	/* clear all data  */
	memset(data, 0, sizeof(struct BitmapData));
	
	/* Get attr values */
	OOP_GetAttr(o, aHidd_BitMap_Width, &width);
	OOP_GetAttr(o, aHidd_BitMap_Height, &height);
	OOP_GetAttr(o,  aHidd_BitMap_PixFmt, (IPTR *)&pf);
	OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);
	OOP_GetAttr(pf, aHidd_PixFmt_BytesPerPixel, &multi);
	
	ASSERT (width != 0 && height != 0 && depth != 0);
	/* 
	   We must only create depths that are supported by the friend drawable
	   Currently we only support the default depth
	   */

	width=(width+15) & ~15;
	data->width = width;
	data->height = height;
	data->bpp = depth;
	data->disp = -1;
	    
	data->bytesperpix = multi;
	data->data = &XSD(cl)->data;
	data->mouse = &XSD(cl)->mouse;

    #if BUFFERED_VRAM
	data->bytesperline = width * multi;
    	data->VideoData = AllocVec(width * height * multi, MEMF_PUBLIC | MEMF_CLEAR);
    #else
	data->bytesperline = data->data->bytesperline;
	data->VideoData = data->data->framebuffer;
    #endif
    
	/* We should be able to get modeID from the bitmap */
	OOP_GetAttr(o, aHidd_BitMap_ModeID, &modeid);
	
	if ((modeid != vHidd_ModeID_Invalid) && (data->VideoData))
	{
	    /*
	       Because of not defined BitMap_Show method show 
	       bitmap immediately
	       */
	    XSD(cl)->visible = data;	/* Set created object as visible */
	    
	    ReturnPtr("VesaGfx.BitMap::New()", OOP_Object *, o);
	}
	
	{
	    OOP_MethodID disp_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
	    OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);
	}
	
	o = NULL;
	
    } /* if created object */
    
    ReturnPtr("VesaGfx.BitMap::New()", OOP_Object *, o);
}

/**********  Bitmap::Dispose()  ***********************************/
STATIC VOID MNAME(dispose)(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);

    EnterFunc(bug("VesaGfx.BitMap::Dispose()\n")); 

#if BUFFERED_VRAM
    if (data->VideoData)
    	FreeVec(data->VideoData);
#endif
    OOP_DoSuperMethod(cl, o, msg);

    ReturnVoid("VesaGfx.BitMap::Dispose");
}

/*** init_onbmclass *********************************************************/

#undef XSD
#define XSD(cl) xsd

#define NUM_ROOT_METHODS   3
#define NUM_BITMAP_METHODS 8

OOP_Class *init_vesagfxonbmclass(struct VesaGfx_staticdata *xsd)
{
    struct OOP_MethodDescr root_descr[NUM_ROOT_METHODS + 1] =
    {
	{(IPTR (*)())MNAME(new)     , moRoot_New    },
	{(IPTR (*)())MNAME(dispose) , moRoot_Dispose},
	{(IPTR (*)())MNAME(get)	    , moRoot_Get    },
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
	{root_descr 	, IID_Root       , NUM_ROOT_METHODS   	},
	{bitMap_descr	, IID_Hidd_BitMap, NUM_BITMAP_METHODS 	},
	{NULL	    	, NULL 	    	 , 0   	    	    	}
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

    EnterFunc(bug("init_vesagfxonbmclass(xsd=%p)\n", xsd));
    
    D(bug("Metattrbase: %x\n", MetaAttrBase));
    
    if(MetaAttrBase)
    {
	D(bug("Got attrbase\n"));
	
	/*		for (;;) {cl = cl; } */
	
	cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
	if(cl)
	{
	    D(bug("BitMap class ok\n"));
	    xsd->onbmclass = cl;
	    cl->UserData     = (APTR) xsd;
	    /* Get attrbase for the BitMap interface */
	    if (OOP_ObtainAttrBases(attrbases))
	    {
		OOP_AddClass(cl);
	    }
	    else
	    {
		free_vesagfxonbmclass( xsd );
		cl = NULL;
	    }
	}
	
	/* We don't need this anymore */
	OOP_ReleaseAttrBase(IID_Meta);
	
    } /* if(MetaAttrBase) */
    
    ReturnPtr("init_vesagfxonbmclass", OOP_Class *,  cl);
}

/*** free_bitmapclass *********************************************************/
void free_vesagfxonbmclass(struct VesaGfx_staticdata *xsd)
{
    EnterFunc(bug("free_vesagfxonbmclass(xsd=%p)\n", xsd));
    
    if(xsd)
    {
	OOP_RemoveClass(xsd->onbmclass);
	
	if(xsd->onbmclass)
	    OOP_DisposeObject((OOP_Object *) xsd->onbmclass);
	    
	xsd->onbmclass = NULL;
	
	OOP_ReleaseAttrBases(attrbases);
    }
    
    ReturnVoid("free_vesagfxonbmclass");
}
