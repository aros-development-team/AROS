/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
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

#define GadToolsBase 	    ((struct GadToolsBase_intern *)cl->cl_UserData)

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

IPTR GTCycle__OM_NEW(Class *cl, Object *objcl, struct opSet *msg)
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
    struct Gadget    	*g;
    
    g = (struct Gadget *)DoSuperMethodA(cl, objcl, (Msg)msg);
    if (!g)
        return (IPTR)NULL;

    data = INST_DATA(cl, g);
    
    data->active     = GetTagData(GTCY_Active, 0, msg->ops_AttrList);
    data->labels     = (STRPTR *)GetTagData(GTCY_Labels, (IPTR)NULL, msg->ops_AttrList);
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

    tattr = (struct TextAttr *)GetTagData(GA_TextAttr, (IPTR)NULL, msg->ops_AttrList);
    if (tattr) data->font = OpenFont(tattr);
    
    imgtags[0].ti_Data = (IPTR)g->Width;
    imgtags[1].ti_Data = (IPTR)g->Height;

    g->GadgetRender = NewObjectA(NULL, FRAMEICLASS, imgtags);
    if (!(g->GadgetRender))
    {
        CoerceMethod(cl, (Object *)g, OM_DISPOSE);
        g = NULL;
    }

    return (IPTR)g;
}

/**********************************************************************************************/

IPTR GTCycle__OM_DISPOSE(Class *cl, struct Gadget * g, Msg msg)
{
    struct CycleData *data = INST_DATA(cl, g);
    
    if (g->GadgetRender)
        DisposeObject(g->GadgetRender);
	
    if (data->font) CloseFont(data->font);
    
    return DoSuperMethodA(cl,(Object *)g,msg);
}

/**********************************************************************************************/

IPTR GTCycle__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    struct CycleData 	*data = INST_DATA(cl, o);
    struct TagItem  	*tag;
    const struct TagItem *taglist = msg->ops_AttrList;
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

IPTR GTCycle__OM_GET(Class *cl, Object *o, struct opGet *msg)
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

IPTR GTCycle__GM_RENDER(Class *cl, struct Gadget *g, struct gpRender *msg)
{
    struct CycleData *data = INST_DATA(cl, g);

    /* Full redraw: clear and draw border */
    DrawImageState(msg->gpr_RPort,(struct Image *)g->GadgetRender,
                   g->LeftEdge, g->TopEdge,
                   (g->Flags & GFLG_SELECTED)? IDS_SELECTED : IDS_NORMAL,
                   msg->gpr_GInfo->gi_DrInfo);

    if (data->font)
    	SetFont(msg->gpr_RPort, data->font);
    else
	SetFont(msg->gpr_RPort, msg->gpr_GInfo->gi_DrInfo->dri_Font);

    if (data->labels)
    {
        rendercyclelabel(cl, g, data->labels[data->active], msg->gpr_RPort, msg->gpr_GInfo);
    }
    
    /* Draw disabled pattern */
    if (g->Flags & GFLG_DISABLED)
    {
        DoDisabledPattern(msg->gpr_RPort,
                          g->LeftEdge,
			  g->TopEdge,
                          g->LeftEdge + g->Width - 1,
			  g->TopEdge + g->Height - 1,
			  msg->gpr_GInfo->gi_DrInfo->dri_Pens[SHADOWPEN],
			  GadToolsBase);
    }
    
    if (msg->gpr_Redraw == GREDRAW_REDRAW)
    {
       renderlabel(GadToolsBase, g, msg->gpr_RPort, data->labelplace);
    }

    return 0;
}

/**********************************************************************************************/

IPTR GTCycle__GM_HITTEST(Class *cl, struct Gadget *g, struct gpHitTest *msg)
{
    return pointingadget(g,
    			 msg->gpht_GInfo,
			 msg->gpht_Mouse.X,
			 msg->gpht_Mouse.Y) ? GMR_GADGETHIT : 0;
}

/**********************************************************************************************/

IPTR GTCycle__GM_GOACTIVE(Class *cl, struct Gadget *g, struct gpInput *msg)
{	
    struct CycleData 	*data;    
    struct RastPort 	*rp;
    IPTR    	    	retval;
    
    data = INST_DATA(cl, g);
    
    g->Flags |= GFLG_SELECTED;
    
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
        DoMethodA((Object *)g, (Msg)&rmsg);
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

IPTR GTCycle__GM_HANDLEINPUT(Class *cl, struct Gadget *g, struct gpInput *msg)
{
    struct RastPort 	*rp;
    struct CycleData 	*data;
    IPTR    	    	retval = GMR_MEACTIVE;

    data = INST_DATA(cl, g);
    
    if (msg->gpi_IEvent->ie_Class == IECLASS_RAWMOUSE)
    {
        if (msg->gpi_IEvent->ie_Code == SELECTUP)
        {
            if (g->Flags & GFLG_SELECTED)
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
	    
            if (DoMethodA((Object *)g, (Msg)&htmsg) != GMR_GADGETHIT)
            {
                if (g->Flags & GFLG_SELECTED)
                {
                    g->Flags &= ~GFLG_SELECTED;
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
			
                        DoMethodA((Object *)g, (Msg)&rmsg);
                        ReleaseGIRPort(rp);
                    }
                }
            }
	    else
            {
                if (!(g->Flags & GFLG_SELECTED))
                {
                    g->Flags |= GFLG_SELECTED;
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
			
                        DoMethodA((Object *)g, (Msg)&rmsg);
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

IPTR GTCycle__GM_GOINACTIVE(Class *cl, struct Gadget *g, struct gpGoInactive *msg)
{
    struct RastPort *rp;

    g->Flags &= ~GFLG_SELECTED;

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
	
        DoMethodA((Object *)g, (Msg)&rmsg);
        ReleaseGIRPort(rp);
    }
    
    return 0;
}

/**********************************************************************************************/
