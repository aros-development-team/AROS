/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Pixelformat class
    Lang: English.
*/

#include <proto/oop.h>
#include <proto/utility.h>
#include <oop/oop.h>
#include <utility/tagitem.h>
#include <hidd/graphics.h>

#define DEBUG 0
#include <aros/debug.h>

#include "graphics_intern.h"

/* Don't initialize them with "= 0", otherwise they end up in the DATA segment! */

static OOP_AttrBase HiddPixFmtAttrBase;

struct pixfmt_data {
     HIDDT_PixelFormat pf;
    
};


OOP_Object *pixfmt_new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    DECLARE_ATTRCHECK(pixfmt);
    
    HIDDT_PixelFormat pf;
    BOOL ok = FALSE;
    
    /* If no attrs are supplied, just create an empty pixfmt object */
    
    EnterFunc(bug("PixFmt::New()\n"));
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (NULL == o)
	ReturnPtr("PixFmt::New(Failed from superclass", OOP_Object *, NULL);
    
    if (NULL == msg->attrList)
    	ReturnPtr("PixFmt::New(empty)", OOP_Object *, o);
	
    if (!parse_pixfmt_tags(msg->attrList, &pf, ATTRCHECK(pixfmt), CSD(cl) )) {
    	D(bug("!!! ERROR PARSINF ATTRS IN PixFmt::New() !!!\n"));
    } else {
	ok = TRUE;
    }
    
    if (!ok) {
	OOP_MethodID dispose_mid;
	dispose_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
	OOP_CoerceMethod(cl, o, (OOP_Msg)&dispose_mid);
	o = NULL;
    }
    
    ReturnPtr("PixFmt::New(Success)", OOP_Object *, o);
     
}     

static VOID pixfmt_get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    HIDDT_PixelFormat *pf;
    struct pixfmt_data *data;
    ULONG idx;

    data = OOP_INST_DATA(cl, o);
    pf = &data->pf;
    
    if (IS_PIXFMT_ATTR(msg->attrID, idx)) {
    	switch (idx) {
	    case aoHidd_PixFmt_RedShift:
	    	*msg->storage = pf->red_shift;
	    	break;
	
	    case aoHidd_PixFmt_GreenShift:
	    	*msg->storage = pf->green_shift;
	    	break;
	
	    case aoHidd_PixFmt_BlueShift:
	    	*msg->storage = pf->blue_shift;
	    	break;
	
	    case aoHidd_PixFmt_AlphaShift:
	    	*msg->storage = pf->alpha_shift;
	    	break;
	
	    case aoHidd_PixFmt_RedMask:
	    	*msg->storage = pf->red_mask;
	    	break;
	
	    case aoHidd_PixFmt_GreenMask:
	    	*msg->storage = pf->green_mask;
	    	break;
	
	    case aoHidd_PixFmt_BlueMask:
	    	*msg->storage = pf->blue_mask;
	    	break;
	
	    case aoHidd_PixFmt_AlphaMask:
	    	*msg->storage = pf->alpha_mask;
	    	break;
	
	    case aoHidd_PixFmt_CLUTShift:
	    	*msg->storage = pf->clut_shift;
	    	break;
	
	    case aoHidd_PixFmt_CLUTMask:
	    	*msg->storage = pf->clut_mask;
	    	break;
	
	    case aoHidd_PixFmt_Depth:
	    	*msg->storage = pf->depth;
	    	break;
	
	    case aoHidd_PixFmt_BitsPerPixel:
	    	*msg->storage = pf->size;
	    	break;
	
	    case aoHidd_PixFmt_BytesPerPixel:
	    	*msg->storage = pf->bytes_per_pixel;
	    	break;
	    
	    case aoHidd_PixFmt_StdPixFmt:
	    	*msg->storage = pf->stdpixfmt;
	    	break;
	    
	    case aoHidd_PixFmt_ColorModel:
	    	*msg->storage = HIDD_PF_COLMODEL(pf);
	    	break;
		
	    case aoHidd_PixFmt_BitMapType:
	    	*msg->storage = HIDD_PF_BITMAPTYPE(pf);
		break;
	    
	    default:
	    	D(bug("TRYING TO GET UNKNOWN PIXFMT ATTR\n"));
    		OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
		break;
	}
    
    } else {
    	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
    
    return;    
}
     

/*** init_pixfmtclass *********************************************************/

#undef OOPBase
#undef SysBase

#define OOPBase (csd->oopbase)
#define SysBase (csd->sysbase)

#define NUM_ROOT_METHODS   2
#define NUM_PIXFMT_METHODS 0

OOP_Class *init_pixfmtclass(struct class_static_data *csd)
{
    struct OOP_MethodDescr root_descr[NUM_ROOT_METHODS + 1] =
    {
        {(IPTR (*)())pixfmt_new    , moRoot_New	},
        {(IPTR (*)())pixfmt_get    , moRoot_Get	},
	{ NULL, 0UL }
    };
    
    struct OOP_MethodDescr pixfmt_descr[NUM_PIXFMT_METHODS + 1] = {
	{ NULL, 0UL }
    };
        
    struct OOP_InterfaceDescr ifdescr[] =
    {
        {root_descr,    IID_Root       , NUM_ROOT_METHODS},
        {pixfmt_descr,  IID_Hidd_PixFmt, NUM_PIXFMT_METHODS},
        {NULL, NULL, 0}
    };

    OOP_AttrBase MetaAttrBase = OOP_GetAttrBase(IID_Meta);

    struct TagItem tags[] =
    {
        {aMeta_SuperID,        (IPTR) CLID_Root},
        {aMeta_InterfaceDescr, (IPTR) ifdescr},
        {aMeta_InstSize,       (IPTR) sizeof (struct pixfmt_data)},
        {TAG_DONE, 0UL}
    };
    
    OOP_Class *cl = NULL;

    EnterFunc(bug("init_pixfmtclass(csd=%p)\n", csd));

    if(MetaAttrBase)  {
        /* Get attrbase for the PixFmt interface */
#ifndef AROS_CREATE_ROM_BUG
        HiddPixFmtAttrBase = OOP_ObtainAttrBase(IID_Hidd_PixFmt);
	if (HiddPixFmtAttrBase) 
#endif
	{
    	    cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
    	    if(NULL != cl)  {
        	D(bug("PixFmt class ok\n"));
        	csd->pixfmtclass = cl;
D(bug("init_pixfmtclass: csd=%p\n", csd));
        	cl->UserData     = (APTR) csd;
		OOP_AddClass(cl);
            
            }
        }
    } /* if(MetaAttrBase) */
    
    if (NULL == cl)
	free_pixfmtclass(csd);

    ReturnPtr("init_pixfmtclass", OOP_Class *,  cl);
}


/*** free_pixfmtclass *********************************************************/

void free_pixfmtclass(struct class_static_data *csd)
{
    EnterFunc(bug("free_pixfmtclass(csd=%p)\n", csd));

    if(NULL != csd)    {
        if (NULL !=csd->pixfmtclass) {
    	    OOP_RemoveClass(csd->pixfmtclass);
	    OOP_DisposeObject((OOP_Object *) csd->pixfmtclass);
            csd->pixfmtclass = NULL;
	}
	
#ifndef AROS_CREATE_ROM_BUG
        if(HiddPixFmtAttrBase)
	    OOP_ReleaseAttrBase(IID_Hidd_PixFmt);
#endif
    }

    ReturnVoid("free_pixfmtclass");
}
