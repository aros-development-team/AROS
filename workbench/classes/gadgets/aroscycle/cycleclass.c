/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROS specific cycle class implementation.
    Lang: english
*/

/***********************************************************************************/

#define USE_BOOPSI_STUBS
#include <exec/libraries.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/imageclass.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <proto/graphics.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <proto/utility.h>
#include <utility/tagitem.h>
#include <devices/inputevent.h>
#include <gadgets/aroscycle.h>
#include <proto/alib.h>

#ifndef DEBUG
#   define DEBUG 0
#endif
#include <aros/debug.h>

#include "aroscycle_intern.h"

/***********************************************************************************/

#define IM(o) ((struct Image *)(o))
#define EG(o) ((struct Gadget *)(o))


#undef AROSCycleBase
#define AROSCycleBase ((struct CycleBase_intern *)(cl->cl_UserData))


#include <clib/boopsistubs.h>

/***********************************************************************************/

Object *cycle_new(Class *cl, Class *rootcl, struct opSet *msg)
{
    struct CycleData 	*data;
    struct TextAttr 	*tattr;
    struct TagItem  	imgtags[] = {
        { IA_Width	, 0 		},
        { IA_Height	, 0 		},
        { IA_EdgesOnly	, FALSE		},
	{ IA_FrameType	, FRAME_BUTTON	},
        { TAG_DONE	, 0UL 		}
    };
    STRPTR  	    	*labels;
    Object  	    	*o;

    o = (Object *)DoSuperMethodA(cl, (Object *)rootcl, (Msg)msg);
    if (!o)
        return NULL;

    data = INST_DATA(cl, o);
    data->active = GetTagData(AROSCYCLE_Active, 0, msg->ops_AttrList);
    data->labels = (STRPTR *)GetTagData(AROSCYCLE_Labels, (IPTR) NULL, msg->ops_AttrList);
    data->numlabels = 0;

    labels = data->labels;
    if (labels)
    {
        while (labels[0])
        {
            data->numlabels++;
            labels++;
        }
    }

    tattr = (struct TextAttr *)GetTagData(GA_TextAttr, (IPTR) NULL, msg->ops_AttrList);
    if (tattr) data->font = OpenFont(tattr);
    
    imgtags[0].ti_Data = (IPTR)EG(o)->Width;
    imgtags[1].ti_Data = (IPTR)EG(o)->Height;

    EG(o)->GadgetRender = NewObjectA(NULL, FRAMEICLASS, imgtags);
    if (!EG(o)->GadgetRender)
    {
        IPTR methodid = OM_DISPOSE;
        CoerceMethodA(cl, o, (Msg)&methodid);
        return NULL;
    }

    return o;
}

/***********************************************************************************/

VOID cycle_dispose(Class *cl, Object *o, Msg msg)
{
    struct CycleData *data = INST_DATA(cl, o);
    
    if (EG(o)->GadgetRender)
        DisposeObject(EG(o)->GadgetRender);
	
    if (data->font) CloseFont(data->font);
    
    DoSuperMethodA(cl,o,msg);
}

/***********************************************************************************/

IPTR cycle_get(Class *cl, Object *o, struct opGet *msg)
{
    struct CycleData *data = INST_DATA(cl, o);
    IPTR    	     retval = 1;

    switch(msg->opg_AttrID)
    {
	case AROSCYCLE_Active:
	    *(msg->opg_Storage) = (IPTR)data->active;
	    break;

	case AROSCYCLE_Labels:
	    *(msg->opg_Storage) = (IPTR)data->labels;
	    break;

	default:
	    retval = DoSuperMethodA(cl, o, (Msg)msg);
	    break;
    }
    
    return retval;
}

/***********************************************************************************/

IPTR cycle_set(Class *cl, Object *o, struct opSet *msg)
{
    struct CycleData 	 *data = INST_DATA(cl, o);
    const struct TagItem *tag, *taglist = msg->ops_AttrList;
    STRPTR  	    	 *mylabels;
    BOOL    	    	  rerender = FALSE;
    IPTR    	    	  result;

    result = DoSuperMethodA(cl, o, (Msg)msg);

    while((tag = NextTagItem(&taglist)))
    {
        switch(tag->ti_Tag)
        {
            case AROSCYCLE_Labels:
        	data->labels = (STRPTR *)tag->ti_Data;
        	data->numlabels = 0;
		data->active = 0;
        	mylabels = data->labels;
        	if (mylabels)
        	{
                    while (mylabels[0])
                    {
                	data->numlabels++;
                	mylabels++;
                    }
        	}
        	rerender = TRUE;
        	break;

            case AROSCYCLE_Active:
        	data->active = tag->ti_Data;
        	rerender = TRUE;
        	break;

	    case GA_Disabled:
		rerender = TRUE;
		break;
        }
    }

    /* SDuvan: Removed test (cl == OCLASS(o)) */

    if(rerender)
    {
        struct RastPort *rport;

        if(data->active > data->numlabels-1)
	    data->active = 0;

	//kprintf("Rerendering\n");

        rport = ObtainGIRPort(msg->ops_GInfo);
        if(rport)
        {
            DoMethod(o, GM_RENDER, (IPTR)msg->ops_GInfo, (IPTR)rport, GREDRAW_UPDATE);
            ReleaseGIRPort(rport);
            result = FALSE;
        }
    }

    return result;
}

/***********************************************************************************/

IPTR cycle_hittest(Class *cl, Object *o, struct gpHitTest *msg)
{
    return pointingadget((struct Gadget *)o,
    			 msg->gpht_GInfo,
			 msg->gpht_Mouse.X,
			 msg->gpht_Mouse.Y) ? GMR_GADGETHIT : 0;
}

/***********************************************************************************/

VOID cycle_render(Class *cl, Object *o, struct gpRender *msg)
{
    struct CycleData *data = INST_DATA(cl, o);

    /* Full redraw: clear and draw border */
    DrawImageState(msg->gpr_RPort,IM(EG(o)->GadgetRender),
                   EG(o)->LeftEdge, EG(o)->TopEdge,
                   EG(o)->Flags&GFLG_SELECTED?IDS_SELECTED:IDS_NORMAL,
                   msg->gpr_GInfo->gi_DrInfo);

    if (data->font)
    	SetFont(msg->gpr_RPort, data->font);
    else
	SetFont(msg->gpr_RPort, msg->gpr_GInfo->gi_DrInfo->dri_Font);

    if (data->labels)
        renderlabel(AROSCycleBase, EG(o),
                    data->labels[data->active],
                    msg->gpr_RPort, msg->gpr_GInfo);
		    
    /* Draw disabled pattern */
    if (G(o)->Flags & GFLG_DISABLED)
        drawdisabledpattern(AROSCycleBase, msg->gpr_RPort,
                            msg->gpr_GInfo->gi_DrInfo->dri_Pens[SHADOWPEN],
                            G(o)->LeftEdge, G(o)->TopEdge,
                            G(o)->Width, G(o)->Height);

}

/***********************************************************************************/

IPTR cycle_goactive(Class *cl, Object *o, struct gpInput *msg)
{	
    struct CycleData 	*data;    
    struct RastPort 	*rport;
    IPTR    	    	retval;
    
    data = INST_DATA(cl, o);
    
    EG(o)->Flags |= GFLG_SELECTED;
    
    rport = ObtainGIRPort(msg->gpi_GInfo);
    if (rport)
    {
        struct gpRender rmsg =
            { GM_RENDER, msg->gpi_GInfo, rport, GREDRAW_UPDATE };
        DoMethodA(o, (Msg)&rmsg);
        ReleaseGIRPort(rport);
        retval = GMR_MEACTIVE;
    } else
        retval = GMR_NOREUSE;
   
    return retval;
}

/***********************************************************************************/

IPTR cycle_handleinput(Class *cl, Object *o, struct gpInput *msg)
{
    struct RastPort 	*rport;
    struct CycleData 	*data;
    IPTR    	    	retval = GMR_MEACTIVE;

    data = INST_DATA(cl, o);
    
    if (msg->gpi_IEvent->ie_Class == IECLASS_RAWMOUSE)
    {
        if (msg->gpi_IEvent->ie_Code == SELECTUP)
        {
            if (G(o)->Flags & GFLG_SELECTED)
            {
                /* mouse is over gadget */
		
        	data->active++;
        	if (data->active == data->numlabels)
                    data->active = 0;

		
                *msg->gpi_Termination = data->active;
                retval = GMR_NOREUSE | GMR_VERIFY;
            } else
                /* mouse is not over gadget */
                retval = GMR_NOREUSE;
		
/*
            G(o)->Flags &= ~GFLG_SELECTED;
	    
            rport = ObtainGIRPort(msg->gpi_GInfo);
            if (rport)
            {
        	struct gpRender rmsg =
                    { GM_RENDER, msg->gpi_GInfo, rport, GREDRAW_UPDATE };
        	DoMethodA(o, (Msg)&rmsg);
        	ReleaseGIRPort(rport);
            }*/

        } else if (msg->gpi_IEvent->ie_Code == IECODE_NOBUTTON)
        {
            struct gpHitTest htmsg =
                { GM_HITTEST, msg->gpi_GInfo,
                  { msg->gpi_Mouse.X, msg->gpi_Mouse.Y },
                };
            if (DoMethodA(o, (Msg)&htmsg) != GMR_GADGETHIT)
            {
                if (EG(o)->Flags & GFLG_SELECTED)
                {
                    G(o)->Flags &= ~GFLG_SELECTED;
                    rport = ObtainGIRPort(msg->gpi_GInfo);
                    if (rport)
                    {
                        struct gpRender rmsg =
                            { GM_RENDER, msg->gpi_GInfo, rport, GREDRAW_UPDATE };
                        DoMethodA(o, (Msg)&rmsg);
                        ReleaseGIRPort(rport);
                    }
                }
            } else
            {
                if (!(EG(o)->Flags & GFLG_SELECTED))
                {
                    EG(o)->Flags |= GFLG_SELECTED;
                    rport = ObtainGIRPort(msg->gpi_GInfo);
                    if (rport)
                    {
                        struct gpRender rmsg =
                            { GM_RENDER, msg->gpi_GInfo, rport, GREDRAW_UPDATE };
                        DoMethodA(o, (Msg)&rmsg);
                        ReleaseGIRPort(rport);
                    }
                }
            }
        } else if (msg->gpi_IEvent->ie_Code == MENUDOWN)
            retval = GMR_REUSE;
    }
    return retval;
}

/***********************************************************************************/

IPTR cycle_goinactive(Class *cl, Object *o, struct gpGoInactive *msg)
{
    struct RastPort *rport;

    EG(o)->Flags &= ~GFLG_SELECTED;

    rport = ObtainGIRPort(msg->gpgi_GInfo);
    if (rport)
    {
        struct gpRender rmsg = { GM_RENDER, msg->gpgi_GInfo, rport, GREDRAW_UPDATE };
	
        DoMethodA(o, (Msg)&rmsg);
        ReleaseGIRPort(rport);
    }
    
    return 0;
}

/***********************************************************************************/
    
AROS_UFH3S(IPTR, dispatch_cycleclass,
	  AROS_UFHA(Class *, cl, A0),
	  AROS_UFHA(Object *, o, A2),
	  AROS_UFHA(Msg, msg, A1)
)
{
    AROS_USERFUNC_INIT

    IPTR retval = 0UL;

    switch (msg->MethodID) 
    {
	case OM_NEW:
	    retval = (IPTR)cycle_new(cl, (Class *)o, (struct opSet *)msg);
	    break;

	case OM_DISPOSE:
            cycle_dispose(cl, o, msg);
            break;

	case OM_SET:
            retval = cycle_set(cl, o, (struct opSet *)msg);
            break;

	case OM_GET:
	    retval = cycle_get(cl, o, (struct opGet *)msg);
	    break;

	case GM_RENDER:
            cycle_render(cl, o, (struct gpRender *)msg);
            break;

	case GM_HITTEST:
	    retval = cycle_hittest(cl, o, (struct gpHitTest *)msg);
	    break;
	    
	case GM_GOACTIVE:
	    retval = cycle_goactive(cl, o, (struct gpInput *)msg);
	    break;

	case GM_HANDLEINPUT:
            retval = cycle_handleinput(cl, o, (struct gpInput *)msg);
            break;

	case GM_GOINACTIVE:
	    retval = cycle_goinactive(cl, o, (struct gpGoInactive *)msg);
	    break;
	    
	default:
	    retval = DoSuperMethodA(cl, o, msg);
	    break;
    }

    return retval;

    AROS_USERFUNC_EXIT
}

/***********************************************************************************/

#undef AROSCycleBase

/***********************************************************************************/

struct IClass *InitCycleClass (struct CycleBase_intern * AROSCycleBase)
{
    Class *cl = NULL;

    cl = MakeClass(AROSCYCLECLASS, GADGETCLASS, NULL, sizeof(struct CycleData), 0);
    if (cl) {
	cl->cl_Dispatcher.h_Entry    = (APTR)AROS_ASMSYMNAME(dispatch_cycleclass);
	cl->cl_Dispatcher.h_SubEntry = NULL;
	cl->cl_UserData 	     = (IPTR)AROSCycleBase;

	AddClass (cl);
    }

    return (cl);
}

/***********************************************************************************/
