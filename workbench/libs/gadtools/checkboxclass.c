/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal GadTools checkbox class.
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

#define CF_MouseOverGad 0x0001
#define CF_CustomImage  0x0002

#define GadToolsBase ((struct GadToolsBase_intern *)cl->cl_UserData)

/**********************************************************************************************/

STATIC VOID drawimage(Class *cl, struct Gadget *gad, struct RastPort *rp,
                      BOOL checked, BOOL disabled)
{
    struct CheckBoxData *data;
    struct Image     	*img;
    ULONG   	     	state = IDS_NORMAL;

    data = INST_DATA(cl, (Object *)gad);
    
    if (checked)
    {
        if (gad->SelectRender)
        {
            img = gad->SelectRender;
            state = IDS_NORMAL;
        }
	else
        {
            img = gad->GadgetRender;
            state = IDS_SELECTED;
        }
    }
    else
    {
        img = gad->GadgetRender;
        state = IDS_NORMAL;
    }

    if (disabled)
    {
        if ((gad->Flags & GFLG_IMAGEDISABLE) && (state == IDS_NORMAL))
	{
            state = IDS_DISABLED;
	}
        else if (gad->Flags & GFLG_IMAGEDISABLE)
	{
            state = IDS_SELECTEDDISABLED;
	}
    }

    DrawImageState(rp, img, gad->LeftEdge, gad->TopEdge, state, data->dri);

    /* Draw disabled pattern, if not supported by imageclass. */
    if ((disabled) && !(gad->Flags & GFLG_IMAGEDISABLE))
    {
	DoDisabledPattern(rp,
			  gad->LeftEdge,
			  gad->TopEdge,
			  gad->LeftEdge + gad->Width - 1,
			  gad->TopEdge + gad->Height - 1,
			  data->dri->dri_Pens[SHADOWPEN],
			  GadToolsBase);
    }
    
}

/**********************************************************************************************/

IPTR GTCheckBox__OM_SET(Class *cl, struct Gadget *g, struct opSet *msg)
{
    struct CheckBoxData *data;
    struct TagItem   	*tag;
    const struct TagItem *taglist = msg->ops_AttrList;
    struct RastPort  	*rp;
    IPTR    	     	retval = FALSE;

    data = INST_DATA(cl, g);
    
    if (data->flags & CF_CustomImage)
    {
        tag = FindTagItem(GA_Image, taglist);
        if (tag)
        {
            DisposeObject(g->GadgetRender);
            g->GadgetRender = NULL;
            data->flags &= ~CF_CustomImage;
        }
    }

    if (msg->MethodID != OM_NEW)
        retval = DoSuperMethodA(cl, (Object *)g, (Msg)msg);

    while ((tag = NextTagItem(&taglist)))
    {
	switch (tag->ti_Tag)
	{
            case GA_Disabled:
        	retval = TRUE;
        	break;
		
	    case GA_DrawInfo:
        	if (msg->MethodID == OM_NEW)
                    data->dri = (struct DrawInfo *) tag->ti_Data;
		break;
		
            case GA_Image:
            case GA_SelectRender:
        	retval = TRUE;
        	break;
		
            case GA_LabelPlace:
        	if (msg->MethodID == OM_NEW)
                    data->labelplace = (LONG)tag->ti_Data;
        	break;
		
	    case GTCB_Checked:
		if (tag->ti_Data)
		    g->Flags |= GFLG_SELECTED;
		else
		    g->Flags &= ~GFLG_SELECTED;
        	retval = TRUE;
		break;
		
	} /* switch (tag->ti_Tag) */
	
    } /* while ((tag = NextTagItem(&taglist))) */

    if (g->Width == 0)
        g->Width = CHECKBOX_WIDTH;
    if (g->Height == 0)
        g->Height = CHECKBOX_HEIGHT;

    /* Redraw ourself? */
    
    if ((retval) && (msg->MethodID != OM_NEW) &&
        ((msg->MethodID != OM_UPDATE) || (OCLASS(g) == cl)))
    {
	rp = ObtainGIRPort(msg->ops_GInfo);
	if (rp)
	{
	    DoMethod((Object *)g, GM_RENDER, (IPTR) msg->ops_GInfo, (IPTR) rp, GREDRAW_UPDATE);
	    ReleaseGIRPort(rp);
	    retval = FALSE;
	}
    }

    return retval;
}

/**********************************************************************************************/

IPTR GTCheckBox__OM_GET(Class *cl, struct Gadget *g, struct opGet *msg)
{
    struct CheckBoxData *data;
    IPTR    	    	retval;
    
    data = INST_DATA(cl, g);
    
    switch (msg->opg_AttrID)
    {
	case GTA_GadgetKind:
	case GTA_ChildGadgetKind:
	    *(msg->opg_Storage) = CHECKBOX_KIND;
	    retval = 1UL;
	    break;

    	case GTCB_Checked:
	    *(msg->opg_Storage) = (g->Flags & GFLG_SELECTED) ? TRUE : FALSE;
	    retval = 1UL;
	    break;
	    
	default:
	    retval = DoSuperMethodA(cl, (Object *)g, (Msg)msg);
	    break;
    }
    
    return retval;
}

/**********************************************************************************************/

IPTR GTCheckBox__OM_NEW(Class *cl, Object *supercl, struct opSet *msg)
{
    struct CheckBoxData *data;
    struct TagItem  	tags[] =
    {
	{IA_Width   	, 0UL	    },
	{IA_Height  	, 0UL	    },
	{SYSIA_DrawInfo , 0UL	    },
	{SYSIA_Which	, CHECKIMAGE},
	{TAG_DONE   	    	    }
    };
    struct Gadget   	*g;

    g = (struct Gadget *)DoSuperMethodA(cl, supercl, (Msg)msg);
    if (!g)
	return (IPTR)0;

    g->Activation |= GACT_RELVERIFY;

    data = INST_DATA(cl, g);
    data->dri = NULL;
    data->flags = 0;
    GTCheckBox__OM_SET(cl, g, msg);

    if (!g->GadgetRender)
    {
        tags[0].ti_Data = g->Width;
        tags[1].ti_Data = g->Height;
        tags[2].ti_Data = (IPTR) data->dri;
        g->GadgetRender = (struct Image *) NewObjectA(NULL, SYSICLASS,
                                                           tags);
        data->flags |= CF_CustomImage;
    }
    
    if ((!data->dri) || (!g->GadgetRender)) 
    {
	CoerceMethod(cl, (Object *)g, OM_DISPOSE);
	g = NULL;
    }

    return (IPTR)g;
}

/**********************************************************************************************/

IPTR GTCheckBox__OM_DISPOSE(Class *cl, struct Gadget *g, Msg msg)
{
    struct CheckBoxData *data;
    IPTR    	    	retval;
    
    data = INST_DATA(cl, g);
    
    if (data->flags & CF_CustomImage)
    {
        DisposeObject(g->GadgetRender);
        g->GadgetRender = NULL;
    }
    retval = DoSuperMethodA(cl, (Object *)g, (Msg)msg);
    
    return retval;
}

/**********************************************************************************************/

IPTR GTCheckBox__GM_RENDER(Class *cl, struct Gadget *g, struct gpRender *msg)
{
    struct CheckBoxData *data;
    IPTR    	     	result = TRUE;

    data = INST_DATA(cl, g);
    
    /* Render image */
    drawimage(cl, g, msg->gpr_RPort,
              g->Flags&GFLG_SELECTED, g->Flags&GFLG_DISABLED);

    /* Render gadget label */
    if (msg->gpr_Redraw == GREDRAW_REDRAW)
    {
        result = renderlabel(GadToolsBase, g, msg->gpr_RPort, data->labelplace);
    }
    
    return result;
}

/**********************************************************************************************/

IPTR GTCheckBox__GM_GOACTIVE(Class *cl, struct Gadget *g, struct gpInput *msg)
{
    struct CheckBoxData *data;
    struct RastPort 	*rp;
    IPTR    	    	retval;
    
    data = INST_DATA(cl, g);
    data->flags |= CF_MouseOverGad;
    
    rp = ObtainGIRPort(msg->gpi_GInfo);
    if (rp)
    {
        drawimage(cl, g, rp, (g->Flags&GFLG_SELECTED) ? FALSE : TRUE, FALSE);
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

IPTR GTCheckBox__GM_HANDLEINPUT(Class *cl, struct Gadget *g, struct gpInput *msg)
{
    struct CheckBoxData *data;
    struct RastPort 	*rp;
    IPTR    	    	retval = GMR_MEACTIVE;

    data = INST_DATA(cl, g);
    
    if (msg->gpi_IEvent->ie_Class == IECLASS_RAWMOUSE) 
    {
	if (msg->gpi_IEvent->ie_Code == SELECTUP)
	{
	    if (data->flags & CF_MouseOverGad)
	    {
		/* mouse is over gadget */
		
		g->Flags ^= GFLG_SELECTED;

		*msg->gpi_Termination = g->Flags&GFLG_SELECTED?TRUE:FALSE;
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
	    if ((msg->gpi_Mouse.X < 0) || (msg->gpi_Mouse.Y < 0) ||
		(msg->gpi_Mouse.X >= g->Width ) || (msg->gpi_Mouse.Y >= g->Height))
	    {
		if (data->flags & CF_MouseOverGad)
		{
		    rp = ObtainGIRPort(msg->gpi_GInfo);
		    if (rp)
		    {
                        drawimage(cl, g, rp,
                                  g->Flags&GFLG_SELECTED, FALSE);
			ReleaseGIRPort(rp);
		    }
		    data->flags &= ~CF_MouseOverGad;
		}
	    }
	    else
	    {
		if (!(data->flags & CF_MouseOverGad))
		{
		    rp = ObtainGIRPort(msg->gpi_GInfo);
		    if (rp)
		    {
                        drawimage(cl, g, rp,
                                  (g->Flags&GFLG_SELECTED)?FALSE:TRUE, FALSE);
			ReleaseGIRPort(rp);
		    }
		    data->flags |= CF_MouseOverGad;
		}
	    }
	    
	} /* else if (msg->gpi_IEvent->ie_Code == IECODE_NOBUTTON) */
	else if (msg->gpi_IEvent->ie_Code == MENUDOWN)
	{
	    retval = GMR_NOREUSE;
	}
	
    } /* if (msg->gpi_IEvent->ie_Class == IECLASS_RAWMOUSE) */
    
    return retval;
}

/**********************************************************************************************/

IPTR GTCheckBox__GM_GOINACTIVE(Class *cl, struct Gadget *g, struct gpGoInactive *msg)
{
    struct CheckBoxData *data;
    struct RastPort    	*rp;
    
    data = INST_DATA(cl, g);
    data->flags &= ~CF_MouseOverGad;
    rp = ObtainGIRPort(msg->gpgi_GInfo);
    if (rp)
    {
        drawimage(cl, g, rp, g->Flags & GFLG_SELECTED, FALSE);
	ReleaseGIRPort(rp);
    }
    
    return 0;
}

/**********************************************************************************************/
