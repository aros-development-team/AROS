/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$

    Internal GadTools slider class.
*/
 

#include <proto/exec.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <intuition/intuition.h>
#include <intuition/cghooks.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <utility/tagitem.h>
#include <devices/inputevent.h>
#include <proto/alib.h>
#include <proto/utility.h>

#include <string.h> /* memset() */

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#include "gadtools_intern.h"

/**********************************************************************************************/

#undef GadToolsBase

STATIC VOID notifylevel(Class *cl, Object *o, WORD level, struct GadgetInfo *ginfo,
			struct GadToolsBase_intern *GadToolsBase)
{			

    struct TagItem ntags[] =
    {
    	{GTSL_Level	, (IPTR)NULL	},
    	{TAG_DONE			}
    };

    ntags[0].ti_Data = (IPTR)level;
    DoSuperMethod(cl, o, OM_NOTIFY, (IPTR) ntags, (IPTR) ginfo, 0);
    
    return;
}

#define GadToolsBase ((struct GadToolsBase_intern *)(cl->cl_UserData))

/**********************************************************************************************/

IPTR GTSlider__OM_SET(Class * cl, Object * o, struct opSet * msg)
{
    IPTR 		retval = 0UL;
    struct TagItem 	*tag, *tstate, *dosuper_tags, tags[] =
    {
    	{PGA_Total	, 0	},
    	{PGA_Top	, 0	},
   	{TAG_MORE	, 0	}
    };
    struct SliderData	*data = INST_DATA(cl, o);
    
    BOOL 		val_set = FALSE;

    EnterFunc(bug("Slider::Set(attrlist=%p)\n", msg->ops_AttrList));
    dosuper_tags = msg->ops_AttrList;

    tstate = msg->ops_AttrList;
    while ((tag = NextTagItem(&tstate)))
    {
    	IPTR tidata = tag->ti_Data;
    	
    	switch (tag->ti_Tag)
    	{
    	    case GTSL_Min:
    	    	data->min = (WORD)tidata;
		val_set = TRUE;
		break;
		
    	    case GTSL_Max:
    	    	data->max = (WORD)tidata;
    	    	val_set = TRUE;
    	    	break;
    	    	
    	    case GTSL_Level:	/* [ISN] */
    	    	if (tidata != data->level)
    	    	{
    	    	    data->level = data->freedom==FREEHORIZ?(WORD)tidata:data->max-(WORD)tidata+data->min;
    	    	    notifylevel(cl, o, data->level, msg->ops_GInfo, GadToolsBase);
		    val_set = TRUE;
		    
    	    	}
    	    	break;
    	    	
    	} /* switch () */
    	
    } /* while (iterate taglist) */
    
    if (val_set)
    {
    	tags[0].ti_Data = data->max - data->min + 1;
    	tags[1].ti_Data = data->level - data->min;
    	tags[2].ti_Data = (IPTR)msg->ops_AttrList;
   
   	dosuper_tags = tags;
    	
    	retval = 1UL;
    }
        
    ReturnInt ("Slider::Set", IPTR, DoSuperMethod(cl, o, OM_SET, (IPTR) dosuper_tags, (IPTR) msg->ops_GInfo));
}

/**********************************************************************************************/

IPTR GTSlider__OM_NEW(Class * cl, Object * o, struct opSet *msg)
{
    struct DrawInfo	*dri;
    
    struct TagItem 	fitags[] =
    {
	{IA_Width	, 0UL		},
	{IA_Height	, 0UL		},
	{IA_Resolution	, 0UL		},
	{IA_FrameType	, FRAME_BUTTON	},
	{IA_EdgesOnly	, TRUE		},
	{TAG_DONE			}
    };
    
    EnterFunc(bug("Slider::New()\n"));
    
    o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);
    if (o)
    {
    	struct SliderData *data = INST_DATA(cl, o);
  
	dri = (struct DrawInfo *)GetTagData(GA_DrawInfo, (IPTR) NULL, msg->ops_AttrList);
	
	fitags[0].ti_Data = GetTagData(GA_Width, 0, msg->ops_AttrList) + BORDERPROPSPACINGX * 2;
	fitags[1].ti_Data = GetTagData(GA_Height, 0, msg->ops_AttrList) + BORDERPROPSPACINGY * 2;
	fitags[2].ti_Data = (dri->dri_Resolution.X << 16) + dri->dri_Resolution.Y;

	data->frame = NewObjectA(NULL, FRAMEICLASS, fitags);
	if (data->frame)
	{
	    data->freedom=GetTagData(PGA_Freedom,FREEHORIZ,msg->ops_AttrList);

	    data->min   = 0;
	    data->max   = 15;
	    data->level = data->freedom==FREEHORIZ?data->min:data->max;

	    GTSlider__OM_SET(cl, o, msg);
	    
	    data->labelplace = GetTagData(GA_LabelPlace, GV_LabelPlace_Left, msg->ops_AttrList);
	} else {
	    CoerceMethod(cl, o, OM_DISPOSE);
	    o = NULL;
	}
    }
    ReturnPtr("Slider::New", IPTR, (IPTR)o);
    
}

/**********************************************************************************************/

IPTR GTSlider__GM_GOACTIVE(Class *cl, Object *o, struct gpInput *msg)
{
    IPTR 		retval;
    struct SliderData 	*data = INST_DATA(cl, o);

    EnterFunc(bug("Slider::GoActive()\n"));
    retval = DoSuperMethodA(cl, o, (Msg)msg);
    
    if (retval != GMR_MEACTIVE)
    {
    	data->level = data->min + (WORD)*(msg->gpi_Termination);
	notifylevel(cl, o, data->level, msg->gpi_GInfo, GadToolsBase);    	
    }
    ReturnInt("Slider::Goactive", IPTR, retval);
}

/**********************************************************************************************/

IPTR GTSlider__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct SliderData 	*data = INST_DATA(cl, o);
    IPTR 		retval = 1UL;
    
    switch (msg->opg_AttrID)
    {
	case GTA_GadgetKind:
	case GTA_ChildGadgetKind:
	    *msg->opg_Storage = SLIDER_KIND;
	    break;
	
	case GTSL_Level:
	  *msg->opg_Storage = data->freedom==FREEHORIZ?data->level:data->max-data->level+data->min;
	  break;
	
	case GTSL_Max:
	    *msg->opg_Storage = data->max;
	    break;
	
	case GTSL_Min:
	    *msg->opg_Storage = data->min;
	    break;
	    
	default:
	    retval = DoSuperMethodA(cl, o, (Msg)msg);
	    break;
    }
    
    return retval;
}

/**********************************************************************************************/

IPTR GTSlider__GM_HANDLEINPUT(Class *cl, Object *o, struct gpInput *msg)
{
    struct InputEvent 	*ie = msg->gpi_IEvent;
    IPTR 		retval;
    struct SliderData 	*data = INST_DATA(cl ,o);

    EnterFunc(bug("Slider::HandleInput()\n"));
    retval = DoSuperMethodA(cl, o, (Msg)msg);
    /* Mousemove ? */
    if ((ie->ie_Class == IECLASS_RAWMOUSE) && (ie->ie_Code == IECODE_NOBUTTON))
    {
    	LONG top;
    	
    	/* Get the PGA_Top attribute */
    	DoSuperMethod(cl, o, OM_GET, PGA_Top, (IPTR) &top);
    	
    	/* Level changed ? */
    	if (data->level - data->min != top)
    	{
    	    data->level = data->min + top;
	    notifylevel(cl, o, data->level, msg->gpi_GInfo, GadToolsBase);    	
    	}
    }
    else
    {
    	if (retval != GMR_MEACTIVE)
    	{
    	
    	    data->level = data->min + (WORD)*(msg->gpi_Termination);
	    notifylevel(cl, o, data->level, msg->gpi_GInfo, GadToolsBase);
    	}
    }
    
    ReturnInt("Slider::HandleInput", IPTR, retval);
}

/**********************************************************************************************/

IPTR GTSlider__GM_RENDER(Class *cl, struct Gadget *g, struct gpRender *msg)
{
    struct SliderData 	*data = INST_DATA(cl, g);
    IPTR 		retval;
    
    if (msg->gpr_Redraw == GREDRAW_REDRAW)
    {
	DrawImageState(msg->gpr_RPort,
		(struct Image *)data->frame,
		g->LeftEdge - BORDERPROPSPACINGX,
		g->TopEdge  - BORDERPROPSPACINGY,
		IDS_NORMAL,
		msg->gpr_GInfo->gi_DrInfo);
   
    }
    
    retval = DoSuperMethodA(cl, (Object *)g, (Msg)msg);

    renderlabel(GadToolsBase, g, msg->gpr_RPort, data->labelplace);

    ReturnInt("Slider::Render", IPTR, retval);
}

/**********************************************************************************************/

IPTR GTSlider__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct SliderData *data = INST_DATA(cl, o);

    if (data->frame) DisposeObject(data->frame);

    return DoSuperMethodA(cl, o, msg);
}

/**********************************************************************************************/
