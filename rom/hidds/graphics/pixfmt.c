/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Pixelformat class
    Lang: English.
*/

/****************************************************************************************/

#include <proto/oop.h>
#include <proto/utility.h>
#include <oop/oop.h>
#include <utility/tagitem.h>
#include <hidd/graphics.h>

#define DEBUG 0
#include <aros/debug.h>

#include "graphics_intern.h"

/****************************************************************************************/

OOP_Object *PF__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    DECLARE_ATTRCHECK(pixfmt);
    
    HIDDT_PixelFormat 	pf;
    BOOL    	    	ok = FALSE;
    
    /* If no attrs are supplied, just create an empty pixfmt object */
    
    EnterFunc(bug("PixFmt::New()\n"));
    
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (NULL == o)
	ReturnPtr("PixFmt::New(Failed from superclass", OOP_Object *, NULL);
    
    if (NULL == msg->attrList)
    	ReturnPtr("PixFmt::New(empty)", OOP_Object *, o);
	
    if (!parse_pixfmt_tags(msg->attrList, &pf, ATTRCHECK(pixfmt), CSD(cl) ))
    {
    	D(bug("!!! ERROR PARSINF ATTRS IN PixFmt::New() !!!\n"));
    }
    else
    {
	ok = TRUE;
    }
    
    if (!ok)
    {
	OOP_MethodID dispose_mid;
	
	dispose_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
	OOP_CoerceMethod(cl, o, (OOP_Msg)&dispose_mid);
	
	o = NULL;
    }
    
    ReturnPtr("PixFmt::New(Success)", OOP_Object *, o);
     
}     

/****************************************************************************************/

VOID PF__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    HIDDT_PixelFormat 	*pf;
    struct pixfmt_data  *data;
    ULONG   	    	idx;

    data = OOP_INST_DATA(cl, o);
    pf = &data->pf;
    
    if (IS_PIXFMT_ATTR(msg->attrID, idx))
    {
    	switch (idx)
	{
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
	    
	    case aoHidd_PixFmt_SwapPixelBytes:
	    	*msg->storage = HIDD_PF_SWAPPIXELBYTES(pf);
		break;
		
	    default:
	    	D(bug("TRYING TO GET UNKNOWN PIXFMT ATTR\n"));
    		OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
		break;
	}
    
    }
    else
    {
    	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
    
    return;    
}
     
/****************************************************************************************/
