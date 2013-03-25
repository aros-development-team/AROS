/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/debug.h>
#include <cybergraphx/cybergraphics.h>
#include <graphics/view.h>
#include <hidd/graphics.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include "cybergraphics_intern.h"
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
#include <clib/cybergraphics_protos.h>


	AROS_LH2(void, CVideoCtrlTagList,

/*  SYNOPSIS */
	AROS_LHA(struct ViewPort *, vp, A0),
	AROS_LHA(struct TagItem  *, tags, A1),

/*  LOCATION */
	struct Library *, CyberGfxBase, 27, Cybergraphics)

/*  FUNCTION
        Controls video output. It currently only allows adjustment of
        power-saving modes, using the following tag:
            SETVC_DPMSLevel (IPTR) - one of the following DPMS levels:
                DPMS_ON - normal operation.
                DPMS_STANDBY - less than 80% power usage.
                DPMS_SUSPEND - less than 30W power usage.
                DPMS_OFF - less than 8W power usage.

    INPUTS
        vp - a ViewPort belonging to the video output device to be controlled.
        tags - a taglist containing video control options.

    RESULT
        None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *tag, *tstate;
    ULONG dpmslevel = 0;
    OOP_Object *gfxhidd = NULL;
    
    struct TagItem htags[] =
    {
	{ aHidd_Gfx_DPMSLevel,	0UL	},
	{ TAG_DONE, 0UL }    
    };
    
    BOOL dpms_found = FALSE;
    
    HIDDT_DPMSLevel hdpms = 0;
    
    for (tstate = tags; (tag = NextTagItem(&tstate)); )
    {
    	switch (tag->ti_Tag)
	{
	    case SETVC_DPMSLevel:
	    	dpmslevel = tag->ti_Data;
		dpms_found = TRUE;
	    	break;
	    
	    default:
	    	D(bug("!!! UNKNOWN TAG IN CVideoCtrlTagList(): %x !!!\n"
			, tag->ti_Tag));
		break;
	    
	} /* switch() */
	
    } /* for (each tagitem) */
    
   
    if (dpms_found)
    {  
    
	/* Convert to hidd dpms level */
	switch (dpmslevel)
	{
	    case DPMS_ON:
	    	hdpms = vHidd_Gfx_DPMSLevel_On;
	    	break;

	    case DPMS_STANDBY:
	    	hdpms = vHidd_Gfx_DPMSLevel_Standby;
	    	break;

	    case DPMS_SUSPEND:
	    	hdpms = vHidd_Gfx_DPMSLevel_Suspend;
	    	break;

	    case DPMS_OFF:
	    	hdpms = vHidd_Gfx_DPMSLevel_Off;
	    	break;
	
	    default:
	    	D(bug("!!! UNKNOWN DPMS LEVEL IN CVideoCtrlTagList(): %x !!!\n"
	    	    , dpmslevel));
		    
		dpms_found = FALSE;
		break;
	
	}
    }
    
    if (dpms_found)
    {
	htags[0].ti_Data = hdpms;
    }
    else
    {
    	htags[0].ti_Tag = TAG_IGNORE;
    }
    
    OOP_GetAttr(HIDD_BM_OBJ(vp->RasInfo->BitMap), aHidd_BitMap_GfxHidd, (IPTR *)&gfxhidd);
    if (gfxhidd)
        OOP_SetAttrs(gfxhidd, htags);

    AROS_LIBFUNC_EXIT
} /* CVideoCtrlTagList */
