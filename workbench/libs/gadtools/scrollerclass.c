/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal GadTools scroller class.
    Lang: English
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

#define GadToolsBase ((struct GadToolsBase_intern *)cl->cl_UserData)

/**********************************************************************************************/

STATIC IPTR scroller_set(Class * cl, Object * o, struct opSet * msg)
{
    IPTR 		retval = 0UL;
    struct TagItem 	*tag, tags[] =
    {
    	{PGA_Total	, 0	},
    	{PGA_Top	, 0	},
    	{PGA_Visible	, 0	},
    	{TAG_MORE		}
    };
    const struct TagItem *tstate = msg->ops_AttrList;

    struct ScrollerData *data = INST_DATA(cl, o);

    D(bug("scroller_set(cl 0x%lx o 0x%lx msg 0x%lx)\n",cl,o,msg));

    tags[3].ti_Data = (IPTR)msg->ops_AttrList;
    
    /* Get old values */
    DoSuperMethod(cl, o, OM_GET, PGA_Total, 	(IPTR) &(tags[0].ti_Data));
    DoSuperMethod(cl, o, OM_GET, PGA_Top, 	(IPTR) &(tags[1].ti_Data));
    DoSuperMethod(cl, o, OM_GET, PGA_Visible, 	(IPTR) &(tags[2].ti_Data));

    D(bug("scroller_set: Old Total %ld Top %ld Visible %ld\n",
		tags[0].ti_Data,
		tags[1].ti_Data,
		tags[2].ti_Data));

    
    while ((tag = NextTagItem(&tstate)))
    {
    	
    	switch (tag->ti_Tag)
    	{
	     case GTA_GadgetKind:
		data->gadgetkind = tag->ti_Data;
		break;

    	     case GTSC_Total:
		D(bug("scroller_set: GTSC_Total %ld\n",tag->ti_Data));
    	     	tags[0].ti_Data  = tag->ti_Data;
    	     	break;

    	     case GTSC_Top:
		D(bug("scroller_set: GTSC_Top %ld\n",tag->ti_Data));
		tags[1].ti_Data  = tag->ti_Data;
		break;
    	     	
    	     case GTSC_Visible:
		D(bug("scroller_set: GTSC_Visible %ld\n",tag->ti_Data));
            	tags[2].ti_Data  = tag->ti_Data;
            	break;
            	
             case GTA_Scroller_Dec:
            	if (tags[1].ti_Data > 0)
            	{
            	    tags[1].ti_Data --;
            	    retval = 1UL;
            	}
                break;
            
            case GTA_Scroller_Inc:
        	/* Top < (Total - Visible) ? */
        	if (tags[1].ti_Data < (tags[0].ti_Data - tags[2].ti_Data))
        	{
            	    tags[1].ti_Data ++;
           	    retval = 1UL;
        	}
                break;
            
	    case GTA_Scroller_Arrow1:
	        data->arrow1 = (struct Gadget *)tag->ti_Data;
		break;
		
	    case GTA_Scroller_Arrow2:
	        data->arrow2 = (struct Gadget *)tag->ti_Data;
		break;
	
	    case GA_Disabled:
	        {
		    struct TagItem set_tags[] = 
		    {
		        {GA_Disabled	, tag->ti_Data	},
			{TAG_DONE			}
		    };
		    
		    if (data->arrow1)
		    {
		        if (msg->ops_GInfo)
			{
			    SetGadgetAttrsA(data->arrow1, msg->ops_GInfo->gi_Window, 0, set_tags);
			} else {
			    SetAttrsA((Object *)data->arrow1, set_tags);
			}
		    }
		    
		    if (data->arrow2)
		    {
		        if (msg->ops_GInfo)
			{
			    SetGadgetAttrsA(data->arrow2, msg->ops_GInfo->gi_Window, 0, set_tags);
			} else {
			    SetAttrsA((Object *)data->arrow2, set_tags);
			}
		    }
		}
		break;
		
    	} /* switch (tag->ti_Tag) */
    	
    } /* while ((tag = NextTagItem(&tstate))) */

    DoSuperMethod(cl, o, OM_SET, (IPTR) tags, (IPTR) msg->ops_GInfo);

    return (retval);
}

/**********************************************************************************************/

IPTR GTScroller__OM_GET(Class * cl, Object * o, struct opGet *msg)
{
    struct ScrollerData *data = INST_DATA(cl, o);
    struct opGet 	cloned_msg = *msg;
    IPTR 		retval = 1UL;

    switch (msg->opg_AttrID)
    {
	case GTA_GadgetKind:
	    *msg->opg_Storage = data->gadgetkind;
	    break;
		
	case GTA_ChildGadgetKind:
	    *msg->opg_Storage = SCROLLER_KIND;
	    break;
	
	case GTSC_Top:
	    cloned_msg.opg_AttrID = PGA_Top;
	    retval = DoSuperMethodA(cl, o, (Msg)&cloned_msg);
	    break;
	    
	case GTSC_Total:
	    cloned_msg.opg_AttrID = PGA_Total;
	    retval = DoSuperMethodA(cl, o, (Msg)&cloned_msg);
	    break;
	    
	case GTSC_Visible:
	    cloned_msg.opg_AttrID = PGA_Visible;
	    retval = DoSuperMethodA(cl, o, (Msg)&cloned_msg);
	    break;
	    
	default:
	    retval = DoSuperMethodA(cl, o, (Msg)msg);
	    break;
    }
    
    return retval;
}

/**********************************************************************************************/

IPTR GTScroller__OM_NEW(Class * cl, Object * o, struct opSet *msg)
{
    struct ScrollerData *data;
    struct DrawInfo 	*dri;
    struct TagItem 	fitags[] =
    {
	{IA_Width	, 0UL		},
	{IA_Height	, 0UL		},
	{IA_Resolution	, 0UL		},
	{IA_FrameType	, FRAME_BUTTON	},
	{IA_EdgesOnly	, TRUE		},
	{TAG_DONE			}
    };
    
    EnterFunc(bug("Scroller::New()\n"));
    
    o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);
    if (o)
    {
    	data = INST_DATA(cl, o);
	
	dri = (struct DrawInfo *)GetTagData(GA_DrawInfo, NULL, msg->ops_AttrList);
	
	fitags[0].ti_Data = GetTagData(GA_Width, 0, msg->ops_AttrList) + BORDERPROPSPACINGX * 2;
	fitags[1].ti_Data = GetTagData(GA_Height, 0, msg->ops_AttrList) + BORDERPROPSPACINGY * 2;
	fitags[2].ti_Data = (dri->dri_Resolution.X << 16) + dri->dri_Resolution.Y;

	data->frame = NewObjectA(NULL, FRAMEICLASS, fitags);
	if (data->frame)
	{
	    scroller_set(cl, o, msg);
	    data->labelplace = GetTagData(GA_LabelPlace, GV_LabelPlace_Left, ((struct opSet *)msg)->ops_AttrList);
	} else {
	    CoerceMethod(cl, o, OM_DISPOSE);
	    o = NULL;
	}
    }
    ReturnPtr("Scroller::New", IPTR, (IPTR)o);
    
}

/**********************************************************************************************/

IPTR GTScroller__GM_RENDER(Class *cl, struct Gadget *g, struct gpRender *msg)
{
    struct ScrollerData *data;
    IPTR 		retval;
    
    data = INST_DATA(cl, g);
    
    if (msg->gpr_Redraw == GREDRAW_REDRAW)
    {
	DrawImageState(msg->gpr_RPort,
		(struct Image *)data->frame,
		g->LeftEdge - BORDERPROPSPACINGX,
		g->TopEdge  - BORDERPROPSPACINGY,
		IDS_NORMAL,
		msg->gpr_GInfo->gi_DrInfo);

        renderlabel(GadToolsBase, g, msg->gpr_RPort, data->labelplace);   
    }
    
    retval = DoSuperMethodA(cl, (Object *)g, (Msg)msg);
    
    ReturnInt("Scroller::Render", IPTR, retval);
}

/**********************************************************************************************/

IPTR GTScroller__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct ScrollerData *data = INST_DATA(cl, o);

    if (data->frame) DisposeObject(data->frame);

    return DoSuperMethodA(cl, o, msg);
}

/**********************************************************************************************/

IPTR GTScroller__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval = scroller_set(cl, o, msg);

    /* If we have been subclassed, OM_UPDATE should not cause a GM_RENDER
     * because it would circumvent the subclass from fully overriding it.
     * The check of cl == OCLASS(o) should fail if we have been
     * subclassed, and we have gotten here via DoSuperMethodA().
     */
    if ( retval && ( (msg->MethodID != OM_UPDATE) ||  (cl == OCLASS(o)) ) )
    {
	struct GadgetInfo *gi = msg->ops_GInfo;
	if (gi)
	{
	    struct RastPort *rp = ObtainGIRPort(gi);
	    if (rp)
	    {
		DoMethod(o, GM_RENDER, (IPTR) gi, (IPTR) rp, GREDRAW_REDRAW);
		ReleaseGIRPort(rp);
	    } /* if */
	} /* if */
    } /* if */
    
    return retval;
}

/**********************************************************************************************/
