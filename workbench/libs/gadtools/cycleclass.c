/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal GadTools cycle class.
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

#define CYCLEIMAGEWIDTH     19 

#define G(x) 	    	    ((struct Gadget *)(x))
#define EG(x) 	    	    ((struct ExtGadget *)(x))
#define IM(x)	    	    ((struct Image *)(x))

#define GadToolsBase 	    ((struct GadToolsBase_intern *)cl->cl_UserData)

/**********************************************************************************************/

struct CycleData
{
    struct TextFont *font;
    STRPTR  	    *labels;
    UWORD   	    active;
    UWORD   	    numlabels;
    UWORD   	    labelplace;
};

/**********************************************************************************************/

STATIC VOID rendercyclelabel(Class *cl, struct Gadget *gad, STRPTR string,
                      	     struct RastPort *rp, struct GadgetInfo *ginfo)
{
    UWORD   *pens = ginfo->gi_DrInfo->dri_Pens;
    WORD    x,y,h;
    int     len = strlen(string);

    SetABPenDrMd(rp, pens[TEXTPEN], pens[BACKGROUNDPEN], JAM1);
    Move(rp,
         gad->LeftEdge + (gad->Width - CYCLEIMAGEWIDTH - TextLength(rp, string, len)) / 2,
         gad->TopEdge + (gad->Height - rp->Font->tf_YSize) / 2 + rp->Font->tf_Baseline);
    Text(rp, string, len);
    
    x = gad->LeftEdge + gad->Width - CYCLEIMAGEWIDTH;
    
    /* separator bar */
    
    SetAPen(rp, pens[SHINEPEN]);
    RectFill(rp, x + 1, gad->TopEdge + 2, x + 1, gad->TopEdge + gad->Height - 1 - 2);
    SetAPen(rp, pens[SHADOWPEN]);
    RectFill(rp, x, gad->TopEdge + 2, x, gad->TopEdge + gad->Height - 1 - 2);

    /* cycle image */
    
    h = gad->Height / 2;
    
    x += 6;

    for(y = 0; y < 4; y++)
    {
    	RectFill(rp,x + y,
		       gad->TopEdge + gad->Height - 1 - h - y - 1,
		       x + 6 - y, 
		       gad->TopEdge + gad->Height - 1 - h - y - 1); 
		       
	RectFill(rp,x + y,
		       gad->TopEdge + h + y + 1,
		       x + 6 - y,
		       gad->TopEdge + h + y + 1); 
    }
}

/**********************************************************************************************/

BOOL pointingadget(struct Gadget *gad, struct GadgetInfo *gi, WORD x, WORD y)
{
    WORD gadw, gadh;
    
    gadw = gad->Width;
    if (gad->Flags & GFLG_RELWIDTH) gadw += gi->gi_Domain.Width;
    
    gadh = gad->Height;
    if (gad->Flags & GFLG_RELHEIGHT) gadh += gi->gi_Domain.Height;
    
    return ((x >= 0) && (y >= 0) && (x < gadw) && (y < gadh)) ? TRUE : FALSE;
}

/**********************************************************************************************/

STATIC IPTR cycle_new(Class *cl, Object *objcl, struct opSet *msg)
{ 
    struct CycleData 	*data;
    struct TextAttr 	*tattr;
    struct TagItem  	imgtags[] =
    {
        { IA_Width	, 0 		},
        { IA_Height	, 0 		},
        { IA_EdgesOnly	, FALSE		},
	{ IA_FrameType	, FRAME_BUTTON	},
        { TAG_DONE	, 0UL 		}
    };
    STRPTR  	    	*labels;
    Object  	    	*o;
    
    o = (Object *)DoSuperMethodA(cl, objcl, (Msg)msg);
    if (!o)
        return NULL;

    data = INST_DATA(cl, o);
    
    data->active     = GetTagData(GTCY_Active, 0, msg->ops_AttrList);
    data->labels     = (STRPTR *)GetTagData(GTCY_Labels, NULL, msg->ops_AttrList);
    data->labelplace = GetTagData(GA_LabelPlace, GV_LabelPlace_Left, msg->ops_AttrList);

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

    tattr = (struct TextAttr *)GetTagData(GA_TextAttr, NULL, msg->ops_AttrList);
    if (tattr) data->font = OpenFont(tattr);
    
    imgtags[0].ti_Data = (IPTR)EG(o)->Width;
    imgtags[1].ti_Data = (IPTR)EG(o)->Height;

    EG(o)->GadgetRender = NewObjectA(NULL, FRAMEICLASS, imgtags);
    if (!(EG(o)->GadgetRender))
    {
        CoerceMethod(cl, o, OM_DISPOSE);
        o = NULL;
    }

    return (IPTR)o;
}

/**********************************************************************************************/

STATIC IPTR cycle_dispose(Class *cl, Object *o, Msg msg)
{
    struct CycleData *data = INST_DATA(cl, o);
    
    if (EG(o)->GadgetRender)
        DisposeObject(EG(o)->GadgetRender);
	
    if (data->font) CloseFont(data->font);
    
    return DoSuperMethodA(cl,o,msg);
}

/**********************************************************************************************/

STATIC IPTR cycle_set(Class *cl, Object *o, struct opSet *msg)
{
    struct CycleData 	*data = INST_DATA(cl, o);
    struct TagItem  	*tag, *taglist = msg->ops_AttrList;
    STRPTR  	    	*mylabels;
    BOOL    	    	rerender = FALSE;
    IPTR    	    	result;

    result = DoSuperMethodA(cl, o, (Msg)msg);

    while((tag = NextTagItem(&taglist)))
    {
        switch(tag->ti_Tag)
        {
            case GTCY_Labels:
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

            case GTCY_Active:
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
        struct RastPort *rp;

        if(data->active > data->numlabels-1)
	    data->active = 0;

	//kprintf("Rerendering\n");

        rp = ObtainGIRPort(msg->ops_GInfo);
        if(rp)
        {
            DoMethod(o, GM_RENDER, (IPTR)msg->ops_GInfo, (IPTR)rp, GREDRAW_UPDATE);
            ReleaseGIRPort(rp);
            result = FALSE;
        }
    }

    return result;
}

/**********************************************************************************************/

STATIC IPTR cycle_get(Class *cl, Object *o, struct opGet *msg)
{
    struct CycleData *data   = INST_DATA(cl, o);
    IPTR    	      retval = FALSE;
    
    switch (msg->opg_AttrID)
    {
	case GTA_GadgetKind:
	case GTA_ChildGadgetKind:
	    *(msg->opg_Storage) = CYCLE_KIND;
	    retval = 1UL;
	    break;

    	case GTCY_Active:
	    *(msg->opg_Storage) = (IPTR)data->active;
	    break;
	    
	case GTCY_Labels:
	    *(msg->opg_Storage) = (IPTR)data->labels;
	    break;
	    
	default:
	    retval = DoSuperMethodA(cl, o, (Msg)msg);
	    break;
    }
    
    return retval;
}

/**********************************************************************************************/

STATIC IPTR cycle_render(Class *cl, Object *o, struct gpRender *msg)
{
    struct CycleData *data = INST_DATA(cl, o);

    /* Full redraw: clear and draw border */
    DrawImageState(msg->gpr_RPort,IM(EG(o)->GadgetRender),
                   EG(o)->LeftEdge, EG(o)->TopEdge,
                   (EG(o)->Flags & GFLG_SELECTED)? IDS_SELECTED : IDS_NORMAL,
                   msg->gpr_GInfo->gi_DrInfo);

    if (data->font)
    	SetFont(msg->gpr_RPort, data->font);
    else
	SetFont(msg->gpr_RPort, msg->gpr_GInfo->gi_DrInfo->dri_Font);

    if (data->labels)
    {
        rendercyclelabel(cl, G(o), data->labels[data->active], msg->gpr_RPort, msg->gpr_GInfo);
    }
    
    /* Draw disabled pattern */
    if (G(o)->Flags & GFLG_DISABLED)
    {
        DoDisabledPattern(msg->gpr_RPort,
                          G(o)->LeftEdge,
			  G(o)->TopEdge,
                          G(o)->LeftEdge + G(o)->Width - 1,
			  G(o)->TopEdge + G(o)->Height - 1,
			  msg->gpr_GInfo->gi_DrInfo->dri_Pens[SHADOWPEN],
			  GadToolsBase);
    }
    
    if (msg->gpr_Redraw == GREDRAW_REDRAW)
    {
       renderlabel(GadToolsBase, (struct Gadget *)o, msg->gpr_RPort, data->labelplace);
    }

    return 0;
}

/**********************************************************************************************/

STATIC IPTR cycle_hittest(Class *cl, Object *o, struct gpHitTest *msg)
{
    return pointingadget(G(o),
    			 msg->gpht_GInfo,
			 msg->gpht_Mouse.X,
			 msg->gpht_Mouse.Y) ? GMR_GADGETHIT : 0;
}

/**********************************************************************************************/

STATIC IPTR cycle_goactive(Class *cl, Object *o, struct gpInput *msg)
{	
    struct CycleData 	*data;    
    struct RastPort 	*rp;
    IPTR    	    	retval;
    
    data = INST_DATA(cl, o);
    
    EG(o)->Flags |= GFLG_SELECTED;
    
    rp = ObtainGIRPort(msg->gpi_GInfo);
    if (rp)
    {
        struct gpRender rmsg =
        {
	    GM_RENDER,
	    msg->gpi_GInfo,
	    rp,
	    GREDRAW_UPDATE
	};
        DoMethodA(o, (Msg)&rmsg);
        ReleaseGIRPort(rp);
        retval = GMR_MEACTIVE;
    }
    else
    {
        retval = GMR_NOREUSE;
    }
    
    return retval;
}

/**********************************************************************************************/

STATIC IPTR cycle_handleinput(Class *cl, Object *o, struct gpInput *msg)
{
    struct RastPort 	*rp;
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
            }
	    else
	    {
                /* mouse is not over gadget */
                retval = GMR_NOREUSE;
	    }

        }
	else if (msg->gpi_IEvent->ie_Code == IECODE_NOBUTTON)
        {
            struct gpHitTest htmsg =
            {
	    	GM_HITTEST,
		msg->gpi_GInfo,
                { msg->gpi_Mouse.X, msg->gpi_Mouse.Y },
            };
	    
            if (DoMethodA(o, (Msg)&htmsg) != GMR_GADGETHIT)
            {
                if (EG(o)->Flags & GFLG_SELECTED)
                {
                    G(o)->Flags &= ~GFLG_SELECTED;
                    rp = ObtainGIRPort(msg->gpi_GInfo);
                    if (rp)
                    {
                        struct gpRender rmsg =
                        {
			    GM_RENDER,
			    msg->gpi_GInfo,
			    rp,
			    GREDRAW_UPDATE
			};
			
                        DoMethodA(o, (Msg)&rmsg);
                        ReleaseGIRPort(rp);
                    }
                }
            }
	    else
            {
                if (!(EG(o)->Flags & GFLG_SELECTED))
                {
                    EG(o)->Flags |= GFLG_SELECTED;
                    rp = ObtainGIRPort(msg->gpi_GInfo);
                    if (rp)
                    {
                        struct gpRender rmsg =
                        {
			    GM_RENDER,
			    msg->gpi_GInfo,
			    rp,
			    GREDRAW_UPDATE
			};
			
                        DoMethodA(o, (Msg)&rmsg);
                        ReleaseGIRPort(rp);
                    }
                }
            }
        }
	else if (msg->gpi_IEvent->ie_Code == MENUDOWN)
	{
            retval = GMR_REUSE;
	}
    }
    
    return retval;
}

/**********************************************************************************************/

STATIC IPTR cycle_goinactive(Class *cl, Object *o, struct gpGoInactive *msg)
{
    struct RastPort *rp;

    EG(o)->Flags &= ~GFLG_SELECTED;

    rp = ObtainGIRPort(msg->gpgi_GInfo);
    if (rp)
    {
        struct gpRender rmsg =
	{
	    GM_RENDER,
	    msg->gpgi_GInfo,
	    rp,
	    GREDRAW_UPDATE
	};
	
        DoMethodA(o, (Msg)&rmsg);
        ReleaseGIRPort(rp);
    }
    
    return 0;
}

/**********************************************************************************************/

AROS_UFH3S(IPTR, dispatch_cycleclass,
	  AROS_UFHA(Class *, cl, A0),
	  AROS_UFHA(Object *, o, A2),
	  AROS_UFHA(Msg, msg, A1)
)
{
    AROS_USERFUNC_INIT

    IPTR retval;
	    
    switch (msg->MethodID)
    {
	case OM_NEW:
 	    retval = cycle_new(cl, o, (struct opSet *)msg);
	    break;
	    
	case OM_DISPOSE:
	    retval = cycle_dispose(cl, o, msg);
	    break;
	    
	case OM_UPDATE:
	case OM_SET:
	    retval = cycle_set(cl, o, (struct opSet *)msg);
	    
	case OM_GET:
	    retval = cycle_get(cl, o, (struct opGet *)msg);
	    break;

	case GM_RENDER:
    	    retval = cycle_render(cl, o, (struct gpRender *)msg);
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

/**********************************************************************************************/

#undef GadToolsBase

/**********************************************************************************************/

Class *makecycleclass(struct GadToolsBase_intern *GadToolsBase)
{
    Class *cl;

    ObtainSemaphore(&GadToolsBase->classsema);

    cl = GadToolsBase->cycleclass;
    if (!cl)
    {
	cl = MakeClass(NULL, GADGETCLASS, NULL, sizeof(struct CycleData), 0UL);
	if (cl)
	{
	    cl->cl_Dispatcher.h_Entry = (APTR) AROS_ASMSYMNAME(dispatch_cycleclass);
	    cl->cl_Dispatcher.h_SubEntry = NULL;
	    cl->cl_UserData = (IPTR) GadToolsBase;

	    GadToolsBase->cycleclass = cl;
	}
    }
    
    ReleaseSemaphore(&GadToolsBase->classsema);
  
    return cl;
}

/**********************************************************************************************/
