/*
    (C) 1997-2001 AROS - The Amiga Research OS
    $Id$

    Desc: AROS specific checkbox class implementation.
    Lang: English
*/

/****************************************************************************************/

#undef SDEBUG
#define SDEBUG 0
#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

#define USE_BOOPSI_STUBS

/****************************************************************************************/

#include <proto/exec.h>
#include <exec/libraries.h>
#include <proto/intuition.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/imageclass.h>
#include <intuition/intuition.h>
#include <gadgets/aroscheckbox.h>
#include <proto/graphics.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <proto/utility.h>
#include <utility/tagitem.h>
#include <devices/inputevent.h>
#include <libraries/gadtools.h>
#include <proto/alib.h>

#include "aroscheckbox_intern.h"

/****************************************************************************************/

#undef AROSCheckboxBase
#define AROSCheckboxBase ((struct CBBase_intern *)(cl->cl_UserData))

#include <clib/boopsistubs.h>

/****************************************************************************************/

void drawimage(Class *cl, struct Gadget *gad, struct RastPort *rport,
               BOOL checked, BOOL disabled)
{
    struct CheckData *data = INST_DATA(cl, (Object *)gad);
    struct Image     *img;
    ULONG   	     state = IDS_NORMAL;

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

    DrawImageState(rport, img, gad->LeftEdge, gad->TopEdge, state, data->dri);

    /* Draw disabled pattern, if not supported by imageclass. */
    if ((disabled) && !(gad->Flags & GFLG_IMAGEDISABLE))
    {
	drawdisabledpattern(AROSCheckboxBase,
                            rport, data->dri->dri_Pens[SHADOWPEN],
			    gad->LeftEdge, gad->TopEdge,
			    gad->Width, gad->Height);
    }
    
}

/****************************************************************************************/

IPTR check_set(Class * cl, Object * obj, struct opSet * msg)
{
    struct CheckData *data = INST_DATA(cl, obj);
    struct TagItem   *tag, *taglist = msg->ops_AttrList;
    struct RastPort  *rport;
    IPTR    	     retval = FALSE;

    if (data->flags & CF_CustomImage)
    {
        tag = FindTagItem(GA_Image, taglist);
        if (tag)
        {
            DisposeObject(G(obj)->GadgetRender);
            G(obj)->GadgetRender = NULL;
            data->flags &= ~CF_CustomImage;
        }
    }

    if (msg->MethodID != OM_NEW)
        retval = DoSuperMethodA(cl, obj, (Msg)msg);

    while ((tag = NextTagItem((const struct TagItem **)&taglist)))
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
		
	    case AROSCB_Checked:
		if (tag->ti_Data)
		    data->flags |= CF_Checked;
		else
		    data->flags &= ~CF_Checked;
        	retval = TRUE;
		break;
		
	} /* switch (tag->ti_Tag) */
	
    } /* while ((tag = NextTagItem((const struct TagItem **)&taglist))) */

    if (G(obj)->Width == 0)
        G(obj)->Width = CHECKBOX_WIDTH;
    if (G(obj)->Height == 0)
        G(obj)->Height = CHECKBOX_HEIGHT;

    /* Redraw ourself? */
    
    if ((retval) && (msg->MethodID != OM_NEW) &&
        ((msg->MethodID != OM_UPDATE) || (OCLASS(obj) == cl)))
    {
	rport = ObtainGIRPort(msg->ops_GInfo);
	if (rport)
	{
	    DoMethod(obj, GM_RENDER, msg->ops_GInfo, rport, GREDRAW_UPDATE);
	    ReleaseGIRPort(rport);
	    retval = FALSE;
	}
    }

    return retval;
}

/****************************************************************************************/

Object *check_new(Class *cl, Class *rootcl, struct opSet *msg)
{
    struct CheckData 	*data;
    struct TagItem  	tags[] =
    {
	{IA_Width   	, 0UL	    },
	{IA_Height  	, 0UL	    },
	{SYSIA_DrawInfo , 0UL	    },
	{SYSIA_Which	, CHECKIMAGE},
	{TAG_DONE   	    	    }
    };
    Object  	    	*obj;

    //EnterFunc(bug("CheckBox::New()\n"));

    obj = (Object *)DoSuperMethodA(cl, (Object *)rootcl, (Msg)msg);
    if (!obj)
	return NULL;

    G(obj)->Activation |= GACT_RELVERIFY;

    data = INST_DATA(cl, obj);
    data->dri = NULL;
    data->flags = 0;
    check_set(cl, obj, msg);

    if (!G(obj)->GadgetRender)
    {
        tags[0].ti_Data = G(obj)->Width;
        tags[1].ti_Data = G(obj)->Height;
        tags[2].ti_Data = (IPTR) data->dri;
        G(obj)->GadgetRender = (struct Image *) NewObjectA(NULL, SYSICLASS,
                                                           tags);
        data->flags |= CF_CustomImage;
    }
    
    if ((!data->dri) || (!G(obj)->GadgetRender)) 
    {
	CoerceMethod(cl, obj, OM_DISPOSE);
	return NULL;
    }

    ReturnPtr("CheckBox::New", Object *, obj);
}

/****************************************************************************************/

IPTR check_render(Class * cl, Object * obj, struct gpRender * msg)
{
    struct CheckData *data = INST_DATA(cl, obj);
    IPTR    	     result = TRUE;

    /* Render image */
    drawimage(cl, G(obj), msg->gpr_RPort,
              data->flags&CF_Checked, G(obj)->Flags&GFLG_DISABLED);

    /* Render gadget label */
    if (msg->gpr_Redraw == GREDRAW_REDRAW)
    {
        result = renderlabel(AROSCheckboxBase,
                             G(obj), msg->gpr_RPort, data->labelplace);
    }
    
    return result;
}

/****************************************************************************************/

IPTR check_handleinput(Class * cl, Object * obj, struct gpInput * msg)
{
    struct CheckData 	*data = INST_DATA(cl, obj);
    struct RastPort 	*rport;
    IPTR    	    	retval = GMR_MEACTIVE;

    if (msg->gpi_IEvent->ie_Class == IECLASS_RAWMOUSE) 
    {
	if (msg->gpi_IEvent->ie_Code == SELECTUP)
	{
	    if (G(obj)->Flags & GFLG_SELECTED)
	    {
		/* mouse is over gadget */
		if (data->flags & CF_Checked)
		    data->flags &= ~CF_Checked;
		else
		    data->flags |= CF_Checked;
		*msg->gpi_Termination = data->flags&CF_Checked?TRUE:FALSE;
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
	    struct gpHitTest htmsg;

	    htmsg.MethodID = GM_HITTEST;
	    htmsg.gpht_GInfo = msg->gpi_GInfo;
	    htmsg.gpht_Mouse.X = msg->gpi_Mouse.X;
	    htmsg.gpht_Mouse.Y = msg->gpi_Mouse.Y;
	    
	    if (DoMethodA(obj, (Msg) & htmsg) != GMR_GADGETHIT)
	    {
		if (G(obj)->Flags & GFLG_SELECTED)
		{
		    rport = ObtainGIRPort(msg->gpi_GInfo);
		    if (rport)
		    {
                        drawimage(cl, G(obj), rport,
                                  data->flags&CF_Checked, FALSE);
			ReleaseGIRPort(rport);
		    }
		    G(obj)->Flags &= ~GFLG_SELECTED;
		}
	    }
	    else
	    {
		if (!(G(obj)->Flags & GFLG_SELECTED))
		{
		    rport = ObtainGIRPort(msg->gpi_GInfo);
		    if (rport)
		    {
                        drawimage(cl, G(obj), rport,
                                  (data->flags&CF_Checked)?FALSE:TRUE, FALSE);
			ReleaseGIRPort(rport);
		    }
		    G(obj)->Flags |= GFLG_SELECTED;
		}
	    }
	    
	} /* else if (msg->gpi_IEvent->ie_Code == IECODE_NOBUTTON) */
	else if (msg->gpi_IEvent->ie_Code == MENUDOWN)
	{
	    retval = GMR_REUSE;
	}
	
    } /* if (msg->gpi_IEvent->ie_Class == IECLASS_RAWMOUSE) */
    
    return retval;
}

/****************************************************************************************/

AROS_UFH3S(IPTR, dispatch_checkclass,
	  AROS_UFHA(Class *, cl, A0),
	  AROS_UFHA(Object *, obj, A2),
	  AROS_UFHA(Msg, msg, A1)
)
{
    AROS_USERFUNC_INIT

    struct CheckData *data;
    struct RastPort  *rport;
    IPTR    	     retval = 0UL;

    EnterFunc(bug("CheckBox::_dispatcher()\n"));

    switch (msg->MethodID)
    {
	case OM_NEW:
	    retval = (IPTR) check_new(cl, (Class *)obj, (struct opSet *) msg);
	    break;

	case OM_DISPOSE:
	    data = INST_DATA(cl, obj);
            if (data->flags & CF_CustomImage)
            {
        	DisposeObject(G(obj)->GadgetRender);
        	G(obj)->GadgetRender = NULL;
            }
	    retval = DoSuperMethodA(cl, obj, msg);
	    break;

	case OM_UPDATE:
	case OM_SET:
	    retval = check_set(cl, obj, (struct opSet *) msg);
	    break;

    #define OPG(x) ((struct opGet *)(x))
	case OM_GET:
	    data = INST_DATA(cl, obj);
	    if (OPG(msg)->opg_AttrID == AROSCB_Checked)
	    {
		*(OPG(msg)->opg_Storage) = data->flags & CF_Checked;
		retval = 1UL;
	    } else
		retval = DoSuperMethodA(cl, obj, msg);
	    break;

    #define GPI(x) ((struct gpInput *)(x))
	case GM_GOACTIVE:
	    data = INST_DATA(cl, obj);
            G(obj)->Flags |= GFLG_SELECTED;
            rport = ObtainGIRPort(GPI(msg)->gpi_GInfo);
            if (rport)
	    {
        	drawimage(cl, G(obj), rport,
                	  (data->flags&CF_Checked)?FALSE:TRUE, FALSE);
        	ReleaseGIRPort(rport);
        	retval = GMR_MEACTIVE;
	    }
	    else
            {
	    	retval = GMR_NOREUSE;
	    }
	    break;

    #define GPGI(x) ((struct gpGoInactive *)(x))
	case GM_GOINACTIVE:
	    data = INST_DATA(cl, obj);
	    G(obj)->Flags &= ~GFLG_SELECTED;
	    rport = ObtainGIRPort(GPGI(msg)->gpgi_GInfo);
	    if (rport)
	    {
        	drawimage(cl, G(obj), rport,
                	  data->flags & CF_Checked, FALSE);
		ReleaseGIRPort(rport);
	    }
	    break;

	case GM_RENDER:
	    retval = check_render(cl, obj, (struct gpRender *) msg);
	    break;

	case GM_HANDLEINPUT:
	    retval = check_handleinput(cl, obj, (struct gpInput *) msg);
	    break;

	default:
	    retval = DoSuperMethodA(cl, obj, msg);
	    break;
    }

    ReturnPtr("CheckBox::_dispatcher()", IPTR, retval);

    AROS_USERFUNC_EXIT

}

/****************************************************************************************/

#undef AROSCheckboxBase

/****************************************************************************************/

struct IClass *InitCheckboxClass (struct CBBase_intern * AROSCheckboxBase)
{
    Class *cl = NULL;

    cl = MakeClass(AROSCHECKBOXCLASS, GADGETCLASS, NULL, sizeof(struct CheckData), 0);
    if (cl)
    {
	cl->cl_Dispatcher.h_Entry    = (APTR)AROS_ASMSYMNAME(dispatch_checkclass);
	cl->cl_Dispatcher.h_SubEntry = NULL;
	cl->cl_UserData 	     = (IPTR)AROSCheckboxBase;

	AddClass (cl);
    }

    return (cl);
}

/****************************************************************************************/
