/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: nVidia bitmap class
    Lang: English.
*/

#define AROS_ALMOST_COMPATIBLE 1

#include <proto/oop.h>
#include <proto/utility.h>

#include <exec/memory.h>
#include <exec/lists.h>

#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <oop/oop.h>

#include <hidd/graphics.h>

#include "riva_hw.h"
#include "nv.h"

#include "bitmap.h"

/* Don't initialize static variables with "=0", otherwise they go into DATA segment */

static OOP_AttrBase HiddBitMapAttrBase;
static OOP_AttrBase HiddNVGfxAttrBase;
static OOP_AttrBase HiddNVBitMapAttrBase;

static struct OOP_ABDescr attrbases[] = 
{
    { IID_Hidd_BitMap,		&HiddBitMapAttrBase },
    /* Private bases */
    { IID_Hidd_NVgfx,		&HiddNVGfxAttrBase	},
    { IID_Hidd_NVBitMap,	&HiddNVBitMapAttrBase },
    { NULL, NULL }
};

#define MNAME(x) onbitmap_ ## x

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#define OnBitmap 1
#include "bitmap_common.c"

/*********** BitMap::New() *************************************/

static OOP_Object *onbitmap_new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    EnterFunc(bug("NVGfx.BitMap::New()\n"));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
    	struct bitmap_data *data;
		int multi = 1;
	
		OOP_Object *pf;

        IPTR width, height, depth;
	
        data = OOP_INST_DATA(cl, o);

		/* clear all data  */
        memset(data, 0, sizeof(struct bitmap_data));
	
		/* Get attr values */
		OOP_GetAttr(o, aHidd_BitMap_Width,		&width);
		OOP_GetAttr(o, aHidd_BitMap_Height, 	&height);
		OOP_GetAttr(o,  aHidd_BitMap_PixFmt,	(IPTR *)&pf);
		OOP_GetAttr(pf, aHidd_PixFmt_Depth,		&depth);
	
		assert (width != 0 && height != 0 && depth != 0);
	
		/* 
		   We must only create depths that are supported by the friend drawable
	  	 Currently we only support the default depth
		*/

		data->width = width;
		data->height = height;
		data->bpp = depth;
		data->disp = -1;
		width=(width+15) & ~15;
		
		if (depth > 16)
		{
			multi = 4;
		}
		else if (depth > 8)
		{
			multi = 2;
		}

		/*
			Here there is brand new method of getting pixelclock data.
			It was introduced here to make the code more portable. Besides
			it may now be used as a base for creating other low level
			video drivers
		*/


	    data->VideoData = vbuffer_alloc(NSD(cl), width*height*multi);
	    if ((APTR)data->VideoData >= (APTR)NSD(cl)->memory)
	    {
			HIDDT_ModeID modeid;
			OOP_Object *sync;
			OOP_Object *pf;
				
			/* We should be able to get modeID from the bitmap */
			OOP_GetAttr(o, aHidd_BitMap_ModeID, &modeid);
				
			if (modeid != vHidd_ModeID_Invalid)
			{
				ULONG pixel, base = (ULONG)data->VideoData - (ULONG)NSD(cl)->memory;
				ULONG hdisp, vdisp, hstart, hend, htotal, vstart, vend, vtotal;
	
				/* Get Sync and PixelFormat properties */
				HIDD_Gfx_GetMode(NSD(cl)->nvhidd, modeid, &sync, &pf);

				OOP_GetAttr(sync, aHidd_Sync_PixelClock, 	&pixel);
				OOP_GetAttr(sync, aHidd_Sync_HDisp, 		&hdisp);
				OOP_GetAttr(sync, aHidd_Sync_VDisp, 		&vdisp);
				OOP_GetAttr(sync, aHidd_Sync_HSyncStart, 	&hstart);
				OOP_GetAttr(sync, aHidd_Sync_VSyncStart, 	&vstart);
				OOP_GetAttr(sync, aHidd_Sync_HSyncEnd,		&hend);
				OOP_GetAttr(sync, aHidd_Sync_VSyncEnd,		&vend);
				OOP_GetAttr(sync, aHidd_Sync_HTotal,		&htotal);
				OOP_GetAttr(sync, aHidd_Sync_VTotal,		&vtotal);
				    
			    /* Now, when the best display mode is chosen, we can build it */
				load_mode(NSD(cl), width, height, depth, pixel, base,
					hdisp, vdisp,
					hstart, hend, htotal,
					vstart, vend, vtotal);

			    NSD(cl)->visible = data;	/* Set created object as visible */

				ReturnPtr("NVGfx.BitMap::New()", Object *, o);

		    } /* if got data->VideoData */
		}

		vbuffer_free(NSD(cl), data->VideoData, width*height*multi);

		{
		    OOP_MethodID disp_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
	    	    OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);
		}
	
		o = NULL;
    } /* if created object */

    ReturnPtr("NVGfx.BitMap::New()", OOP_Object *, o);
}

/**********  Bitmap::Dispose()  ***********************************/

static VOID onbitmap_dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
	int multi = 1;

    EnterFunc(bug("NVGfx.BitMap::Dispose()\n"));
    
	if (data->bpp > 16)
	{
		multi = 4;
	}
	else if (data->bpp > 8)
	{
		multi = 2;
	}

    if (data->VideoData)
		vbuffer_free(NSD(cl), data->VideoData, data->width * data->height * multi);
   
    OOP_DoSuperMethod(cl, o, msg);
    
    ReturnVoid("NVGfx.BitMap::Dispose");
}

/*** init_onbmclass *********************************************************/

#undef NSD
#define NSD(cl) nsd

#define NUM_ROOT_METHODS   3
#define NUM_BITMAP_METHODS 10

OOP_Class *nv_init_onbmclass(struct nv_staticdata *nsd)
{
    struct OOP_MethodDescr root_descr[NUM_ROOT_METHODS + 1] =
    {
        {(IPTR (*)())MNAME(new)    , moRoot_New    },
        {(IPTR (*)())MNAME(dispose), moRoot_Dispose},
        {(IPTR (*)())MNAME(get)	   , moRoot_Get},
        {NULL, 0UL}
    };

    struct OOP_MethodDescr bitMap_descr[NUM_BITMAP_METHODS + 1] =
    {
    	{(IPTR (*)())MNAME(setcolors),		moHidd_BitMap_SetColors},
    	{(IPTR (*)())MNAME(putpixel),		moHidd_BitMap_PutPixel},
    	{(IPTR (*)())MNAME(clear),			moHidd_BitMap_Clear},
    	{(IPTR (*)())MNAME(getpixel),		moHidd_BitMap_GetPixel},
    	{(IPTR (*)())MNAME(fillrect),		moHidd_BitMap_FillRect},
    	{(IPTR (*)())MNAME(getimage),		moHidd_BitMap_GetImage},
    	{(IPTR (*)())MNAME(putimage),		moHidd_BitMap_PutImage},
    	{(IPTR (*)())MNAME(blitcolorexpansion),	moHidd_BitMap_BlitColorExpansion},
    	{(IPTR (*)())MNAME(putimagelut),	moHidd_BitMap_PutImageLUT},
    	{(IPTR (*)())MNAME(getimagelut),	moHidd_BitMap_GetImageLUT},
        {NULL, 0UL}
    };
    
    struct OOP_InterfaceDescr ifdescr[] =
    {
        {root_descr,    IID_Root       , NUM_ROOT_METHODS},
        {bitMap_descr,  IID_Hidd_BitMap, NUM_BITMAP_METHODS},
        {NULL, NULL, 0}
    };

    OOP_AttrBase MetaAttrBase = OOP_ObtainAttrBase(IID_Meta);

    struct TagItem tags[] =
    {
        {aMeta_SuperID,        (IPTR) CLID_Hidd_BitMap},
        {aMeta_InterfaceDescr, (IPTR) ifdescr},
        {aMeta_InstSize,       (IPTR) sizeof(struct bitmap_data)},
        {TAG_DONE, 0UL}
    };
    
    OOP_Class *cl = NULL;

    EnterFunc(bug("init_bitmapclass(nsd=%p)\n", nsd));
    
    D(bug("Metattrbase: %x\n", MetaAttrBase));

    if(MetaAttrBase)
    {
		D(bug("Got attrbase\n"));
       
        cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
        if(cl)
        {
            D(bug("BitMap class ok\n"));
            nsd->onbmclass = cl;
            cl->UserData     = (APTR) nsd;
           
            /* Get attrbase for the BitMap interface */
		    if (OOP_ObtainAttrBases(attrbases))
            {
                OOP_AddClass(cl);
            }
            else
            {
                nv_free_onbmclass( nsd );
                cl = NULL;
            }
        }
	
	/* We don't need this anymore */
	OOP_ReleaseAttrBase(IID_Meta);
    } /* if(MetaAttrBase) */

    ReturnPtr("init_onbmclass", OOP_Class *,  cl);
}

/*** free_bitmapclass *********************************************************/

void nv_free_onbmclass(struct nv_staticdata *nsd)
{
    EnterFunc(bug("free_onbmclass(nsd=%p)\n", nsd));

    if(nsd)
    {
        OOP_RemoveClass(nsd->onbmclass);
        if(nsd->onbmclass) OOP_DisposeObject((OOP_Object *) nsd->onbmclass);
        nsd->onbmclass = NULL;
	
		OOP_ReleaseAttrBases(attrbases);
    }

    ReturnVoid("free_onbmclass");
}
