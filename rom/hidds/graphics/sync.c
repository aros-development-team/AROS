/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Sync info class
    Lang: English.
*/

/****************************************************************************************/

#include <proto/oop.h>
#include <proto/utility.h>
#include <exec/memory.h>
#include <oop/oop.h>
#include <utility/tagitem.h>
#include <hidd/graphics.h>

#define DEBUG 0
#include <aros/debug.h>

#include "graphics_intern.h"

/****************************************************************************************/

OOP_Object *Sync__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct sync_data 	*data;
    BOOL    	    	ok = FALSE;
    
    DECLARE_ATTRCHECK(sync);
    
    EnterFunc(bug("Sync::New()\n"));
    
    /* Get object from superclass */
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (NULL == o)
	return NULL;

    /* If we got a NULL attrlist we just allocate an empty object an exit */
    if (NULL == msg->attrList)
	return o;

    data = OOP_INST_DATA(cl, o);
    
    if (!parse_sync_tags(msg->attrList, data, ATTRCHECK(sync), CSD(cl) ))
    {
	D(bug("!!! ERROR PARSING SYNC ATTRS IN Sync::New() !!!\n"));
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
    
    return o;
}

/****************************************************************************************/

VOID Sync__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct sync_data 	*data;    
    ULONG   	    	idx;
    
    data = OOP_INST_DATA(cl, o);
    
    if (IS_SYNC_ATTR(msg->attrID, idx))
    {
    	switch (idx)
	{
	    case aoHidd_Sync_PixelTime:
	        if (data->pixelclock) {
	            /* According to the HOWTO, PixelTime is one million divided by pixelclock in mHz.
		       Pixelclock is not always a multiple of 1 mHz, but it seems to always be a multiple
		       of 1 kHz. We rely on this fact in order to be able to calculate everything in integers.
		       Anyway, this attribute is deprecated, don't use it. */
		    ULONG khz = data->pixelclock / 1000;

		    *msg->storage = 1000000000 / khz;
	        } else
		    *msg->storage = 0;

	    case aoHidd_Sync_PixelClock:
	        *msg->storage = data->pixelclock;
		break;

	    case aoHidd_Sync_LeftMargin:
		*msg->storage = data->htotal - data->hsync_end;
		break;

	    case aoHidd_Sync_RightMargin:
		*msg->storage = data->hsync_start - data->hdisp;
		break;

	    case aoHidd_Sync_HSyncLength:
		*msg->storage = data->hsync_end - data->hsync_start;
		break;

	    case aoHidd_Sync_UpperMargin:
		*msg->storage = data->vtotal - data->vsync_end;
		break;

	    case aoHidd_Sync_LowerMargin:
		*msg->storage = data->vsync_end - data->vdisp;
		break;

	    case aoHidd_Sync_VSyncLength:
		*msg->storage = data->vsync_end - data->vsync_start;
		break;

	    case aoHidd_Sync_HDisp:
		*msg->storage = data->hdisp;
		break;

	    case aoHidd_Sync_VDisp:
		*msg->storage = data->vdisp;
		break;

	    case aoHidd_Sync_HSyncStart:
		*msg->storage = data->hsync_start;
		break;

	    case aoHidd_Sync_HSyncEnd:
		*msg->storage = data->hsync_end;
		break;

	    case aoHidd_Sync_HTotal:
		*msg->storage = data->htotal;
		break;

	    case aoHidd_Sync_VSyncStart:
		*msg->storage = data->vsync_start;
		break;

	    case aoHidd_Sync_VSyncEnd:
		*msg->storage = data->vsync_end;
		break;

	    case aoHidd_Sync_VTotal:
		*msg->storage = data->vtotal;
		break;

	    case aoHidd_Sync_Description:
	    	*msg->storage = (IPTR)data->description;
		break;

	    case aoHidd_Sync_HMin:
		*msg->storage = data->hmin;
		break;

	    case aoHidd_Sync_HMax:
		*msg->storage = data->hmax;
		break;

	    case aoHidd_Sync_VMin:
		*msg->storage = data->vmin;
		break;

	    case aoHidd_Sync_VMax:
		*msg->storage = data->vmax;
		break;

	    case aoHidd_Sync_Flags:
	    *msg->storage = data->flags;
		break;

	    default:
	     	D(bug("!!! TRYING TO GET UNKNOWN ATTR FROM SYNC OBJECT !!!\n"));
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
