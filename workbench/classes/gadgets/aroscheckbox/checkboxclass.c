/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: AROS specific checkbox class implementation.
    Lang: english
*/

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
#include <proto/alib.h>

#include "aroscheckbox_intern.h"


#undef AROSCheckboxBase
#define AROSCheckboxBase ((struct CBBase_intern *)(cl->cl_UserData))



void check_setnew(Class * cl, Object * obj, struct opSet *msg)
{
    struct CheckData *data = INST_DATA(cl, obj);
    struct TagItem *tag, *taglist = msg->ops_AttrList;

    while ((tag = NextTagItem(&taglist))) {
	switch (tag->ti_Tag) {
	case GA_DrawInfo:
	    data->dri = (struct DrawInfo *) tag->ti_Data;
	    break;
	case AROSCB_Checked:
	    if (tag->ti_Data)
		data->flags |= CF_Checked;
	    else
		data->flags &= ~CF_Checked;
	    break;
	}
    }
}

Object *check_new(Class * cl, Object * obj, struct opSet *msg)
{
    struct CheckData *data;
    struct TagItem supertags[] =
    {
	{GA_RelVerify, TRUE},
	{TAG_MORE, (IPTR) msg->ops_AttrList}
    };
    struct TagItem tags[] =
    {
	{IA_Width, 0UL},
	{IA_Height, 0UL},
	{SYSIA_DrawInfo, 0UL},
	{SYSIA_Which, CHECKIMAGE},
	{TAG_DONE, 0L}
    };

    obj = (Object *) DoSuperMethod(cl, obj, OM_NEW, &supertags, msg->ops_GInfo);
    if (!obj)
	return NULL;

    data = INST_DATA(cl, obj);
    data->image = NULL;
    data->dri = NULL;
    data->flags = 0;
    check_setnew(cl, obj, (struct opSet *) msg);

    tags[0].ti_Data = G(obj)->Width;
    tags[1].ti_Data = G(obj)->Height;
    tags[2].ti_Data = (IPTR) data->dri;
    data->image = (struct Image *) NewObjectA(NULL, SYSICLASS, tags);
    if ((!data->dri) || (!data->image)) {
	CoerceMethod(cl, obj, OM_DISPOSE);
	return NULL;
    }
    return obj;
}


IPTR check_set(Class * cl, Object * obj, struct opSet * msg)
{
    IPTR retval = FALSE;
    struct CheckData *data = INST_DATA(cl, obj);
    struct TagItem *tag;
    struct RastPort *rport;

    retval = DoSuperMethodA(cl, obj, (Msg)msg);

    tag = FindTagItem(AROSCB_Checked, msg->ops_AttrList);
    if (tag) {
	if (tag->ti_Data)
	    data->flags |= CF_Checked;
	else
	    data->flags &= ~CF_Checked;
	retval = TRUE;
    }

    if (FindTagItem(GA_Disabled, msg->ops_AttrList))
        retval = TRUE;

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

IPTR check_render(Class * cl, Object * obj, struct gpRender * msg)
{
    struct CheckData *data = INST_DATA(cl, obj);
    IPTR supportdisable = FALSE;
    ULONG state;

    /* Draw the checkbox image (possibly disabled and/or selected) */
    GetAttr(IA_SupportsDisable, (Object *) data->image, &supportdisable);
    if (data->flags & CF_Checked) {
	if ((G(obj)->Flags & GFLG_DISABLED) && (supportdisable)) {
	    state = IDS_SELECTEDDISABLED;
	} else
	    state = IDS_SELECTED;
    } else if ((G(obj)->Flags & GFLG_DISABLED) && (supportdisable))
	state = IDS_DISABLED;
    else
	state = IDS_NORMAL;
    DrawImageState(msg->gpr_RPort, data->image,
		   G(obj)->LeftEdge, G(obj)->TopEdge,
		   state, data->dri);

    /* Draw disabled pattern, if not supported by imageclass */
    if ((!supportdisable) && (G(obj)->Flags & GFLG_DISABLED))
	drawdisabledpattern(AROSCheckboxBase,
                            msg->gpr_RPort, data->dri->dri_Pens[SHADOWPEN],
			    G(obj)->LeftEdge, G(obj)->TopEdge,
			    G(obj)->Width, G(obj)->Height);

    /* Render gadget label */
    renderlabel(AROSCheckboxBase, G(obj), msg->gpr_RPort, msg->gpr_Redraw);

    return TRUE;
}

IPTR check_handleinput(Class * cl, Object * obj, struct gpInput * msg)
{
    IPTR retval = GMR_MEACTIVE;
    struct CheckData *data = INST_DATA(cl, obj);
    struct RastPort *rport;

    if (msg->gpi_IEvent->ie_Class == IECLASS_RAWMOUSE) {
	if (msg->gpi_IEvent->ie_Code == SELECTUP) {
	    if (G(obj)->Flags & GFLG_SELECTED) {
		/* mouse is over gadget */
		if (data->flags & CF_Checked)
		    data->flags &= ~CF_Checked;
		else
		    data->flags |= CF_Checked;
		*msg->gpi_Termination = IDCMP_GADGETUP;
		retval = GMR_NOREUSE | GMR_VERIFY;
	    } else
		/* mouse is not over gadget */
		retval = GMR_NOREUSE;
	} else if (msg->gpi_IEvent->ie_Code == IECODE_NOBUTTON) {
	    struct gpHitTest htmsg;

	    htmsg.MethodID = GM_HITTEST;
	    htmsg.gpht_GInfo = msg->gpi_GInfo;
	    htmsg.gpht_Mouse.X = msg->gpi_Mouse.X;
	    htmsg.gpht_Mouse.Y = msg->gpi_Mouse.Y;
	    if (DoMethodA(obj, (Msg) & htmsg) != GMR_GADGETHIT) {
		if (G(obj)->Flags & GFLG_SELECTED) {
		    rport = ObtainGIRPort(msg->gpi_GInfo);
		    if (rport) {
			int state;

			if (data->flags & CF_Checked)
			    state = IDS_SELECTED;
			else
			    state = IDS_NORMAL;
			DrawImageState(rport, data->image,
				       G(obj)->LeftEdge, G(obj)->TopEdge,
				       state, data->dri);
			ReleaseGIRPort(rport);
		    }
		    G(obj)->Flags &= ~GFLG_SELECTED;
		}
	    } else {
		if (!(G(obj)->Flags & GFLG_SELECTED)) {
		    rport = ObtainGIRPort(msg->gpi_GInfo);
		    if (rport) {
			int state;

			if (data->flags & CF_Checked)
			    state = IDS_NORMAL;
			else
			    state = IDS_SELECTED;
			DrawImageState(rport, data->image,
				       G(obj)->LeftEdge, G(obj)->TopEdge,
				       state, data->dri);
			ReleaseGIRPort(rport);
		    }
		    G(obj)->Flags |= GFLG_SELECTED;
		}
	    }
	} else if (msg->gpi_IEvent->ie_Code == MENUDOWN)
	    retval = GMR_REUSE;
    }
    return retval;
}


AROS_UFH3(static IPTR, dispatch_checkclass,
	  AROS_UFHA(Class *, cl, A0),
	  AROS_UFHA(Object *, obj, A2),
	  AROS_UFHA(Msg, msg, A1)
)
{
    IPTR retval = 0UL;
    struct CheckData *data;
    struct RastPort *rport;
    int x;

    switch (msg->MethodID) {
    case OM_NEW:
	retval = (IPTR) check_new(cl, obj, (struct opSet *) msg);
	break;

    case OM_DISPOSE:
	data = INST_DATA(cl, obj);
	DisposeObject(data->image);
	retval = DoSuperMethodA(cl, obj, msg);
	break;

    case OM_SET:
	retval = check_set(cl, obj, (struct opSet *) msg);
	break;

#define OPG(x) ((struct opGet *)(x))
    case OM_GET:
	data = INST_DATA(cl, obj);
	if (OPG(msg)->opg_AttrID == AROSCB_Checked) {
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
        if (rport) {
            if (data->flags & CF_Checked)
                x = IDS_NORMAL;
            else
                x = IDS_SELECTED;
            DrawImageState(rport, data->image,
                           G(obj)->LeftEdge, G(obj)->TopEdge,
                           x, data->dri);
            ReleaseGIRPort(rport);
            retval = GMR_MEACTIVE;
	} else
            retval = GMR_NOREUSE;
	break;

#define GPGI(x) ((struct gpGoInactive *)(x))
    case GM_GOINACTIVE:
	data = INST_DATA(cl, obj);
	G(obj)->Flags &= ~GFLG_SELECTED;
	rport = ObtainGIRPort(GPGI(msg)->gpgi_GInfo);
	if (rport) {
	    if (data->flags & CF_Checked)
		x = IDS_SELECTED;
	    else
		x = IDS_NORMAL;
	    DrawImageState(rport, data->image,
			   G(obj)->LeftEdge, G(obj)->TopEdge,
			   x, data->dri);
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

    return retval;
}

/*************************** Classes *****************************/

#undef AROSCheckboxBase

struct IClass *InitCheckboxClass (struct CBBase_intern * AROSCheckboxBase)
{
    Class *cl = NULL;

    cl = MakeClass(AROSCHECKBOXCLASS, GADGETCLASS, NULL, sizeof(struct CheckData), 0);
    if (cl) {
	cl->cl_Dispatcher.h_Entry    = (APTR)AROS_ASMSYMNAME(dispatch_checkclass);
	cl->cl_Dispatcher.h_SubEntry = NULL;
	cl->cl_UserData 	     = (IPTR)AROSCheckboxBase;

	AddClass (cl);
    }

    return (cl);
}
