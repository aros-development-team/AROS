/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
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

#define GadToolsBase ((struct GadToolsBase_intern *)cl->cl_UserData)

/**********************************************************************************************/

Object *GTButton__OM_NEW(Class * cl, Object * obj, struct opSet *msg)
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

IPTR GTButton__OM_SET(Class * cl, Object * obj, struct opSet * msg)
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

IPTR GTButton__OM_GET(Class * cl, Object * obj, struct opGet *msg)
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

IPTR GTButton__GM_RENDER(Class * cl, struct Gadget * g, struct gpRender * msg)
{
    IPTR 		retval = 0UL;
    UWORD 		old_gadgetflags;
    struct IntuiText 	*old_gadgettext;
    
    /* Georg Steger: Hack, because IntuiTexts are not centered
       by button gadget class */
       
    old_gadgetflags = g->Flags;
    old_gadgettext = g->GadgetText;
    
    g->Flags &= ~GFLG_LABELMASK;
    g->Flags |= GFLG_LABELITEXT;
    g->GadgetText = 0;
    
    retval = DoSuperMethodA(cl, (Object *)g, (Msg)msg);
 
    g->GadgetText  = old_gadgettext;
    g->Flags = old_gadgetflags;
 
    renderlabel(GadToolsBase, g, msg->gpr_RPort, GV_LabelPlace_In);
  
    return retval;
}

/**********************************************************************************************/

IPTR GTButton__OM_DISPOSE(Class * cl, Object * obj, Msg msg)
{
    struct ButtonData *data = INST_DATA(cl, obj);
    
    if (data->frame) DisposeObject(data->frame);
    
    return DoSuperMethodA(cl, obj, msg);
}

/**********************************************************************************************/
