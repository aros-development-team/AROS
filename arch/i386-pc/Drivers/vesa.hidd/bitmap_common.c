/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English.
*/

#include <exec/alerts.h>
#include <string.h>    // memset() prototype
#include <aros/macros.h>

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>


/*********  BitMap::PutPixel()  ***************************/
static VOID MNAME(putpixel)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutPixel *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);
    ULONG offset;

#ifdef OnBitmap
    offset = (msg->x*data->bytesperpix)+(msg->y*data->data->bytesperline);
#else
    offset = (msg->x + (msg->y*data->width))*data->bytesperpix;
#endif
    if (data->bytesperpix == 1)
	*((UBYTE*)(data->VideoData + offset)) = msg->pixel;
    else if (data->bytesperpix == 2)
	*((UWORD*)(data->VideoData + offset)) = msg->pixel;
    else if (data->bytesperpix == 4)
	*((ULONG*)(data->VideoData + offset)) = msg->pixel;

    return;
}

/*********  BitMap::GetPixel()  *********************************/
static HIDDT_Pixel MNAME(getpixel)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetPixel *msg)
{
    HIDDT_Pixel pixel;
    struct BitmapData *data = OOP_INST_DATA(cl, o);
    ULONG offset;

#ifdef OnBitmap
    offset = (msg->x*data->bytesperpix)+(msg->y*data->data->bytesperline);
#else
    offset = (msg->x + (msg->y*data->width))*data->bytesperpix;
#endif
    if (data->bytesperpix == 1)
	pixel = *((UBYTE*)(data->VideoData + offset));
    else if (data->bytesperpix == 2)
	pixel = *((UWORD*)(data->VideoData + offset));
    else if (data->bytesperpix == 4)
	pixel = *((ULONG*)(data->VideoData + offset));
    return pixel;
}

static VOID MNAME(fillrect)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    struct BitmapData *data =OOP_INST_DATA(cl, o);
    HIDDT_Pixel fg = GC_FG(msg->gc);
    HIDDT_DrawMode mode = GC_DRMD(msg->gc);
    ULONG mod;

#ifdef OnBitmap
    mod = data->data->bytesperline;
#else
    mod = data->width * data->bytesperpix;
#endif

    switch(mode)
    {
    #if 1
        case vHidd_GC_DrawMode_Copy:
	    switch(data->bytesperpix)
	    {
	    	case 1:
		    HIDD_BM_FillMemRect8(o,
	    	    	    		 data->VideoData,
	    	    	    		 msg->minX,
					 msg->minY,
					 msg->maxX,
					 msg->maxY,
					 mod,
					 fg);
		    break;
		    
		case 2:
		    HIDD_BM_FillMemRect16(o,
	    	    	    		 data->VideoData,
	    	    	    		 msg->minX,
					 msg->minY,
					 msg->maxX,
					 msg->maxY,
					 mod,
					 fg);
		    break;
	    
	    	case 4:
		    HIDD_BM_FillMemRect32(o,
	    	    	    		 data->VideoData,
	    	    	    		 msg->minX,
					 msg->minY,
					 msg->maxX,
					 msg->maxY,
					 mod,
					 fg);
		    break;
		
	    }
	    break;
    #endif   
    
	case vHidd_GC_DrawMode_Invert:
	    HIDD_BM_InvertMemRect(o,
	    	    	    	 data->VideoData,
	    	    	    	 msg->minX * data->bytesperpix,
				 msg->minY,
				 msg->maxX * data->bytesperpix + data->bytesperpix - 1,
				 msg->maxY,
				 mod);
	    break;
	    
	default:
	    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	    break;
	    
    } /* switch(mode) */

}

/*** BitMap::Get() *******************************************/
static VOID MNAME(get)(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    if (IS_VesaGfxBM_ATTR(msg->attrID, idx))
    {
	switch (idx)
	{
	    case aoHidd_VesaGfxBitMap_Drawable:
		*msg->storage = (ULONG)data->VideoData;
		break;
	    default:
		OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	}
    }
    else
    {
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
}

