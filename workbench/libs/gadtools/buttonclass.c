/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal GadTools button class.
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

#define GadToolsBase ((struct GadToolsBase_intern *)cl->cl_UserData)

/**********************************************************************************************/

struct ButtonData
{
    struct DrawInfo 	*dri;
    struct Image 	*frame;
};

/**********************************************************************************************/

STATIC Object *button_new(Class * cl, Object * obj, struct opSet *msg)
{
    struct ButtonData	*data;
    struct DrawInfo 	*dri;
    struct Image 	*frame;
    struct TagItem 	tags[] =
    {
	{IA_Width	, 0UL		},
	{IA_Height	, 0UL		},
	{IA_Resolution	, 0UL		},
	{IA_FrameType	, FRAME_BUTTON	},
	{TAG_DONE			}
    };

    dri = (struct DrawInfo *) GetTagData(GA_DrawInfo, (IPTR) NULL, msg->ops_AttrList);
    if (!dri)
	return NULL;

    tags[0].ti_Data = GetTagData(GA_Width, 0, msg->ops_AttrList);
    tags[1].ti_Data = GetTagData(GA_Height, 0, msg->ops_AttrList);
    tags[2].ti_Data = (dri->dri_Resolution.X << 16) + dri->dri_Resolution.Y;
    frame = (struct Image *) NewObjectA(NULL, FRAMEICLASS, tags);
    if (!frame)
	return NULL;

    tags[0].ti_Tag = GA_Image;
    tags[0].ti_Data = (IPTR) frame;
    tags[1].ti_Tag = TAG_MORE;
    tags[1].ti_Data = (IPTR) msg->ops_AttrList;
    obj = (Object *) DoSuperMethod(cl, obj, OM_NEW, (IPTR) tags, (IPTR) msg->ops_GInfo);
    if (!obj) {
	DisposeObject(frame);
	return NULL;
    }
    data = INST_DATA(cl, obj);
    data->dri = dri;
    data->frame = frame;

    return obj;
}

/**********************************************************************************************/

STATIC IPTR button_set(Class * cl, Object * obj, struct opSet * msg)
{
    IPTR 		retval = 0UL;
    struct TagItem 	*tag, tags[2];
    struct RastPort 	*rport;

    /* Catch everything, but GA_Disabled. */
    tag = FindTagItem(GA_Disabled, msg->ops_AttrList);
    if (tag) {
	tags[0].ti_Tag = GA_Disabled;
	tags[0].ti_Data = tag->ti_Data;
	tags[1].ti_Tag = TAG_DONE;
	DoSuperMethod(cl, obj, OM_SET, (IPTR) tags, (IPTR) msg->ops_GInfo);
	retval = TRUE;
    }

    /* Redraw the gadget, if an attribute was changed and if this is the
       objects' base-class. */
    if ((retval) && (OCLASS(obj) == cl)) {
	rport = ObtainGIRPort(msg->ops_GInfo);
	if (rport) {
	    DoMethod(obj, GM_RENDER, (IPTR) msg->ops_GInfo, (IPTR) rport, GREDRAW_UPDATE);
	    ReleaseGIRPort(rport);
	    retval = FALSE;
	}
    }

    return retval;
}

/**********************************************************************************************/

static IPTR button_get(Class * cl, Object * obj, struct opGet *msg)
{
    IPTR retval;
    
    switch (msg->opg_AttrID)
    {
	case GTA_GadgetKind:
	case GTA_ChildGadgetKind:
	    *(msg->opg_Storage) = BUTTON_KIND;
	    retval = 1UL;
	    break;

	default:
	    retval = DoSuperMethodA(cl, obj, (Msg)msg);
	    break;

    }
    
    return retval;  
}

/**********************************************************************************************/

STATIC IPTR button_render(Class * cl, Object * obj, struct gpRender * msg)
{
    IPTR 		retval = 0UL;
    UWORD 		old_gadgetflags;
    struct IntuiText 	*old_gadgettext;
    
    /* Georg Steger: Hack, because IntuiTexts are not centered
       by button gadget class */
       
    old_gadgetflags = G(obj)->Flags;
    old_gadgettext = G(obj)->GadgetText;
    
    G(obj)->Flags &= ~GFLG_LABELMASK;
    G(obj)->Flags |= GFLG_LABELITEXT;
    G(obj)->GadgetText = 0;
    
    retval = DoSuperMethodA(cl, obj, (Msg)msg);
 
    G(obj)->GadgetText  = old_gadgettext;
    G(obj)->Flags = old_gadgetflags;
 
    renderlabel(GadToolsBase, (struct Gadget *)obj, msg->gpr_RPort, GV_LabelPlace_In);
  
    return retval;
}

/**********************************************************************************************/

STATIC IPTR button_dispose(Class * cl, Object * obj, Msg msg)
{
    struct ButtonData *data = INST_DATA(cl, obj);
    
    if (data->frame) DisposeObject(data->frame);
    
    return DoSuperMethodA(cl, obj, msg);
}

/**********************************************************************************************/

AROS_UFH3S(IPTR, dispatch_buttonclass,
	  AROS_UFHA(Class *, cl, A0),
	  AROS_UFHA(Object *, obj, A2),
	  AROS_UFHA(Msg, msg, A1)
)
{
    AROS_USERFUNC_INIT

    IPTR retval;

    switch (msg->MethodID)
    {
	case OM_NEW:
	    retval = (IPTR) button_new(cl, obj, (struct opSet *) msg);
	    break;

	case OM_DISPOSE:
            retval = button_dispose(cl, obj, msg);
	    break;

	case OM_SET:
	    retval = button_set(cl, obj, (struct opSet *) msg);
	    break;

	case OM_GET:
	    retval = button_get(cl, obj, (struct opGet *) msg);
	    break;

	case GM_RENDER:
    	    retval = button_render(cl, obj, (struct gpRender *) msg);
	    break;

	default:
	    retval = DoSuperMethodA(cl, obj, msg);
	    break;
    }

    return retval;

    AROS_USERFUNC_EXIT
}

/**********************************************************************************************/

#undef GadToolsBase

Class *makebuttonclass(struct GadToolsBase_intern * GadToolsBase)
{
    Class *cl;

    ObtainSemaphore(&GadToolsBase->classsema);

    cl = GadToolsBase->buttonclass;
    if (!cl)
    {	
	cl = MakeClass(NULL, FRBUTTONCLASS, NULL, sizeof(struct ButtonData), 0UL);
	if (cl)
	{
	    cl->cl_Dispatcher.h_Entry = (APTR) AROS_ASMSYMNAME(dispatch_buttonclass);
	    cl->cl_Dispatcher.h_SubEntry = NULL;
	    cl->cl_UserData = (IPTR) GadToolsBase;

	    GadToolsBase->buttonclass = cl;
	}
    }
    
    ReleaseSemaphore(&GadToolsBase->classsema);

    return cl;
}

/**********************************************************************************************/
