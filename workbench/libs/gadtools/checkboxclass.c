/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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

#define G(x) ((struct Gadget *)(x))
#define EG(X) ((struct ExtGadget *)(x))

#define CF_MouseOverGad 0x0001
#define CF_CustomImage  0x0002

#define GadToolsBase ((struct GadToolsBase_intern *)cl->cl_UserData)

/**********************************************************************************************/

struct CheckBoxData
{
    struct DrawInfo *dri;
    LONG    	    labelplace;
    LONG    	    flags;
};

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

STATIC IPTR checkbox_set(Class *cl, Object *o, struct opSet *msg)
{
    struct CheckBoxData *data;
    struct TagItem   	*tag, *taglist;
    struct RastPort  	*rp;
    IPTR    	     	retval = FALSE;

    data = INST_DATA(cl, o);
    taglist = msg->ops_AttrList;
    
    if (data->flags & CF_CustomImage)
    {
        tag = FindTagItem(GA_Image, taglist);
        if (tag)
        {
            DisposeObject(G(o)->GadgetRender);
            G(o)->GadgetRender = NULL;
            data->flags &= ~CF_CustomImage;
        }
    }

    if (msg->MethodID != OM_NEW)
        retval = DoSuperMethodA(cl, o, (Msg)msg);

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
		    G(o)->Flags |= GFLG_SELECTED;
		else
		    G(o)->Flags &= ~GFLG_SELECTED;
        	retval = TRUE;
		break;
		
	} /* switch (tag->ti_Tag) */
	
    } /* while ((tag = NextTagItem(&taglist))) */

    if (G(o)->Width == 0)
        G(o)->Width = CHECKBOX_WIDTH;
    if (G(o)->Height == 0)
        G(o)->Height = CHECKBOX_HEIGHT;

    /* Redraw ourself? */
    
    if ((retval) && (msg->MethodID != OM_NEW) &&
        ((msg->MethodID != OM_UPDATE) || (OCLASS(o) == cl)))
    {
	rp = ObtainGIRPort(msg->ops_GInfo);
	if (rp)
	{
	    DoMethod(o, GM_RENDER, (IPTR) msg->ops_GInfo, (IPTR) rp, GREDRAW_UPDATE);
	    ReleaseGIRPort(rp);
	    retval = FALSE;
	}
    }

    return retval;
}

/**********************************************************************************************/

STATIC IPTR checkbox_get(Class *cl, Object *o, struct opGet *msg)
{
    struct CheckBoxData *data;
    IPTR    	    	retval;
    
    data = INST_DATA(cl, o);
    
    switch (msg->opg_AttrID)
    {
	case GTA_GadgetKind:
	case GTA_ChildGadgetKind:
	    *(msg->opg_Storage) = CHECKBOX_KIND;
	    retval = 1UL;
	    break;

    	case GTCB_Checked:
	    *(msg->opg_Storage) = (G(o)->Flags & GFLG_SELECTED) ? TRUE : FALSE;
	    retval = 1UL;
	    break;
	    
	default:
	    retval = DoSuperMethodA(cl, o, (Msg)msg);
	    break;
    }
    
    return retval;
}

/**********************************************************************************************/

STATIC IPTR checkbox_new(Class *cl, Object *supercl, struct opSet *msg)
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
    Object  	    	*o;

    o = (Object *)DoSuperMethodA(cl, supercl, (Msg)msg);
    if (!o)
	return 0;

    G(o)->Activation |= GACT_RELVERIFY;

    data = INST_DATA(cl, o);
    data->dri = NULL;
    data->flags = 0;
    checkbox_set(cl, o, msg);

    if (!G(o)->GadgetRender)
    {
        tags[0].ti_Data = G(o)->Width;
        tags[1].ti_Data = G(o)->Height;
        tags[2].ti_Data = (IPTR) data->dri;
        G(o)->GadgetRender = (struct Image *) NewObjectA(NULL, SYSICLASS,
                                                           tags);
        data->flags |= CF_CustomImage;
    }
    
    if ((!data->dri) || (!G(o)->GadgetRender)) 
    {
	CoerceMethod(cl, o, OM_DISPOSE);
	o = 0;
    }

    return (IPTR)o;
}

/**********************************************************************************************/

STATIC IPTR checkbox_dispose(Class *cl, Object *o, Msg msg)
{
    struct CheckBoxData *data;
    IPTR    	    	retval;
    
    data = INST_DATA(cl, o);
    
    if (data->flags & CF_CustomImage)
    {
        DisposeObject(G(o)->GadgetRender);
        G(o)->GadgetRender = NULL;
    }
    retval = DoSuperMethodA(cl, o, (Msg)msg);
    
    return retval;
}

/**********************************************************************************************/

STATIC IPTR checkbox_render(Class *cl, Object *o, struct gpRender *msg)
{
    struct CheckBoxData *data;
    IPTR    	     	result = TRUE;

    data = INST_DATA(cl, o);
    
    /* Render image */
    drawimage(cl, G(o), msg->gpr_RPort,
              G(o)->Flags&GFLG_SELECTED, G(o)->Flags&GFLG_DISABLED);

    /* Render gadget label */
    if (msg->gpr_Redraw == GREDRAW_REDRAW)
    {
        result = renderlabel(GadToolsBase, G(o), msg->gpr_RPort, data->labelplace);
    }
    
    return result;
}

/**********************************************************************************************/

STATIC IPTR checkbox_goactive(Class *cl, Object *o, struct gpInput *msg)
{
    struct CheckBoxData *data;
    struct RastPort 	*rp;
    IPTR    	    	retval;
    
    data = INST_DATA(cl, o);
    data->flags |= CF_MouseOverGad;
    
    rp = ObtainGIRPort(msg->gpi_GInfo);
    if (rp)
    {
        drawimage(cl, G(o), rp, (G(o)->Flags&GFLG_SELECTED) ? FALSE : TRUE, FALSE);
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

STATIC IPTR checkbox_handleinput(Class *cl, Object *o, struct gpInput *msg)
{
    struct CheckBoxData *data;
    struct RastPort 	*rp;
    IPTR    	    	retval = GMR_MEACTIVE;

    data = INST_DATA(cl, o);
    
    if (msg->gpi_IEvent->ie_Class == IECLASS_RAWMOUSE) 
    {
	if (msg->gpi_IEvent->ie_Code == SELECTUP)
	{
	    if (data->flags & CF_MouseOverGad)
	    {
		/* mouse is over gadget */
		
		G(o)->Flags ^= GFLG_SELECTED;

		*msg->gpi_Termination = G(o)->Flags&GFLG_SELECTED?TRUE:FALSE;
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
	    	(msg->gpi_Mouse.X >= G(o)->Width ) || (msg->gpi_Mouse.Y >= G(o)->Height))
	    {
		if (data->flags & CF_MouseOverGad)
		{
		    rp = ObtainGIRPort(msg->gpi_GInfo);
		    if (rp)
		    {
                        drawimage(cl, G(o), rp,
                                  G(o)->Flags&GFLG_SELECTED, FALSE);
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
                        drawimage(cl, G(o), rp,
                                  (G(o)->Flags&GFLG_SELECTED)?FALSE:TRUE, FALSE);
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

STATIC IPTR checkbox_goinactive(Class *cl, Object *o, struct gpGoInactive *msg)
{
    struct CheckBoxData *data;
    struct RastPort    	*rp;
    
    data = INST_DATA(cl, o);
    data->flags &= ~CF_MouseOverGad;
    rp = ObtainGIRPort(msg->gpgi_GInfo);
    if (rp)
    {
        drawimage(cl, G(o), rp, G(o)->Flags & GFLG_SELECTED, FALSE);
	ReleaseGIRPort(rp);
    }
    
    return 0;
}

/**********************************************************************************************/

AROS_UFH3S(IPTR, dispatch_checkboxclass,
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
	    retval = checkbox_new(cl, o, (struct opSet *)msg);
	    break;
	    
	case OM_DISPOSE:
	    retval = checkbox_dispose(cl, o, msg);
	    break;
	
	case OM_SET:
	case OM_UPDATE:
	    retval = checkbox_set(cl, o, (struct opSet *)msg);
	    break;
	    	   
	case OM_GET:
	    retval = checkbox_get(cl, o, (struct opGet *)msg);
	    break;
	    
	case GM_RENDER:
	    retval = checkbox_render(cl, o, (struct gpRender *)msg);
	    break;
	    
	case GM_GOACTIVE:
	    retval = checkbox_goactive(cl, o, (struct gpInput *)msg);
	    break;
	    
	case GM_HANDLEINPUT:
	    retval = checkbox_handleinput(cl, o, (struct gpInput *)msg);
	    break;
	    
	case GM_GOINACTIVE:
	    retval = checkbox_goinactive(cl, o, (struct gpGoInactive *)msg);
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

Class *makecheckboxclass(struct GadToolsBase_intern *GadToolsBase)
{
    Class *cl;

    ObtainSemaphore(&GadToolsBase->classsema);

    cl = GadToolsBase->checkboxclass;
    if (!cl)
    {
	cl = MakeClass(NULL, GADGETCLASS, NULL, sizeof(struct CheckBoxData), 0UL);
	if (cl)
	{
	    cl->cl_Dispatcher.h_Entry = (APTR) AROS_ASMSYMNAME(dispatch_checkboxclass);
	    cl->cl_Dispatcher.h_SubEntry = NULL;
	    cl->cl_UserData = (IPTR) GadToolsBase;

	    GadToolsBase->checkboxclass = cl;
	}
    }
    
    ReleaseSemaphore(&GadToolsBase->classsema);
  
    return cl;
}

/**********************************************************************************************/
