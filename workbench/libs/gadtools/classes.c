/*
   (C) 1997 AROS - The Amiga Replacement OS
   $Id$

   Desc: Internal GadTools classes.
   Lang: English
 */
#include <exec/libraries.h>
#include <proto/dos.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/imageclass.h>
#include <intuition/intuition.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <utility/tagitem.h>
#include <devices/inputevent.h>
#include <proto/alib.h>
#include <proto/utility.h>

#include "gadtools_intern.h"

#define G(x) ((struct Gadget *)(x))

#define GadToolsBase ((struct GadToolsBase_intern *)cl->cl_UserData)

/*************************** BUTTON_KIND *****************************/

struct ButtonData {
    struct DrawInfo *dri;
    struct Image *frame;
};


Object *button_new(Class * cl, Object * obj, struct opSet *msg)
{
    struct ButtonData *data;
    struct DrawInfo *dri;
    struct Image *frame;
    struct TagItem tags[] =
    {
	{IA_Width, 0UL},
	{IA_Height, 0UL},
	{IA_Resolution, 0UL},
	{IA_FrameType, FRAME_BUTTON},
	{TAG_DONE, 0UL}
    };

    dri = (struct DrawInfo *) GetTagData(GA_DrawInfo, NULL, msg->ops_AttrList);
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
    obj = (Object *) DoSuperMethod(cl, obj, OM_NEW, tags, msg->ops_GInfo);
    if (!obj) {
	DisposeObject(frame);
	return NULL;
    }
    data = INST_DATA(cl, obj);
    data->dri = dri;
    data->frame = frame;

    return obj;
}


IPTR button_set(Class * cl, Object * obj, struct opSet * msg)
{
    IPTR retval = 0UL;
    struct TagItem *tag, tags[2];
    struct RastPort *rport;

    tag = FindTagItem(GA_Disabled, msg->ops_AttrList);
    if (tag) {
	tags[0].ti_Tag = GA_Disabled;
	tags[0].ti_Data = tag->ti_Data;
	tags[1].ti_Tag = TAG_DONE;
	DoSuperMethod(cl, obj, OM_SET, tags, msg->ops_GInfo);
	retval = TRUE;
    }
    if ((retval) && (((Class *) (*(obj - sizeof(Class *)))) == cl)) {
	rport = ObtainGIRPort(msg->ops_GInfo);
	if (rport) {
	    DoMethod(obj, GM_RENDER, msg->ops_GInfo, rport, GREDRAW_UPDATE);
	    ReleaseGIRPort(rport);
	    retval = FALSE;
	}
    }
    return retval;
}


AROS_UFH3(static IPTR, dispatch_buttonclass,
	  AROS_UFHA(Class *, cl, A0),
	  AROS_UFHA(Object *, obj, A2),
	  AROS_UFHA(Msg, msg, A1)
)
{
    struct ButtonData *data;
    IPTR retval = 0UL;

    switch (msg->MethodID) {
    case OM_NEW:
	retval = (IPTR) button_new(cl, obj, (struct opSet *) msg);
	break;

    case OM_DISPOSE:
	data = INST_DATA(cl, obj);
	DisposeObject(data->frame);
	retval = DoSuperMethodA(cl, obj, msg);
	break;

    case OM_SET:
	retval = button_set(cl, obj, (struct opSet *) msg);
	break;

#define OPG(x) ((struct opGet *)(x))
    case OM_GET:
	data = INST_DATA(cl, obj);
	if (OPG(msg)->opg_AttrID == GA_Disabled)
	    retval = DoSuperMethodA(cl, obj, msg);
	else {
	    *(OPG(msg)->opg_Storage) = 0UL;
	    retval = 0UL;
	}
	break;

    default:
	retval = DoSuperMethodA(cl, obj, msg);
	break;
    }

    return retval;
}

/*************************** Classes *****************************/

#undef GadToolsBase

Class *makebuttonclass(struct GadToolsBase_intern * GadToolsBase)
{
    Class *cl;

    if (GadToolsBase->buttonclass)
	return GadToolsBase->buttonclass;

    cl = MakeClass(NULL, FRBUTTONCLASS, NULL, sizeof(struct ButtonData), 0UL);
    if (!cl)
	return NULL;
    cl->cl_Dispatcher.h_Entry = (APTR) AROS_ASMSYMNAME(dispatch_buttonclass);
    cl->cl_Dispatcher.h_SubEntry = NULL;
    cl->cl_UserData = (IPTR) GadToolsBase;

    GadToolsBase->buttonclass = cl;

    return cl;
}
