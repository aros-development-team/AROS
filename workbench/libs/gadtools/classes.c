/*
   (C) 1997 AROS - The Amiga Replacement OS
   $Id$

   Desc: Internal GadTools classes
   Lang: English
 */

#include <proto/dos.h>

#include <exec/libraries.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/imageclass.h>
#include <intuition/intuition.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <proto/utility.h>
#include <utility/tagitem.h>
#include <devices/inputevent.h>
#include <proto/alib.h>

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

IPTR button_render(Class * cl, Object * obj, struct gpRender * msg)
{
    struct ButtonData *data = INST_DATA(cl, obj);

    /* Let our superclass render first */
    if (!DoSuperMethodA(cl, obj, (Msg) msg))
	return FALSE;

    /* Draw disabled pattern */
    if ((G(obj)->Flags & GFLG_DISABLED))
	drawdisabledpattern(GadToolsBase,
			  msg->gpr_RPort, data->dri->dri_Pens[SHADOWPEN],
			    G(obj)->LeftEdge, G(obj)->TopEdge,
			    G(obj)->Width, G(obj)->Height);

    return TRUE;
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

    case GM_RENDER:
	retval = button_render(cl, obj, (struct gpRender *) msg);
	break;

    default:
	retval = DoSuperMethodA(cl, obj, msg);
	break;
    }

    return retval;
}

/******************************* MX_KIND *******************************/

struct MXData {
    struct DrawInfo *dri;
    struct Image *mximage;
    ULONG active, newactive;
    STRPTR *labels;
    ULONG numlabels;
    UWORD spacing;
};


void mx_setnew(Class * cl, Object * obj, struct opSet *msg)
{
    struct MXData *data = INST_DATA(cl, obj);
    struct TagItem *tag, *taglist = msg->ops_AttrList;

    while ((tag = NextTagItem(&taglist))) {
	switch (tag->ti_Tag) {
	case GA_DrawInfo:
	    data->dri = (struct DrawInfo *) tag->ti_Data;
	    break;
	case GTMX_Active:
	    data->active = tag->ti_Data;
	    break;
	case GTMX_Labels:
	    data->labels = (STRPTR *) tag->ti_Data;
	    data->numlabels = 0;
	    while (data->labels[data->numlabels])
		data->numlabels++;
	    break;
	case GTMX_Spacing:
	    data->spacing = tag->ti_Data;
	    break;
	}
    }
}


Object *mx_new(Class * cl, Class * rootcl, struct opSet *msg)
{
    struct MXData *data;
    Object *obj;
    struct TagItem supertags[] =
    {
	{GA_Immediate, TRUE},
	{TAG_MORE, (IPTR) msg->ops_AttrList}
    };
    struct TagItem tags[] =
    {
	{IA_Width, 0},
	{IA_Height, 0},
	{SYSIA_DrawInfo, (IPTR) NULL},
	{SYSIA_Which, MXIMAGE},
	{TAG_DONE, 0L}
    };

    obj = (Object *) DoSuperMethod(cl, (Object *) rootcl, OM_NEW, supertags, msg->ops_GInfo);
    if (!obj)
	return NULL;

    data = INST_DATA(cl, obj);
    data->dri = NULL;
    data->active = 0;
    data->labels = NULL;
    data->spacing = 1;
    mx_setnew(cl, obj, msg);

    tags[0].ti_Data = G(obj)->Width;
    if (data->numlabels)
	tags[1].ti_Data = (G(obj)->Height - data->spacing) / data->numlabels - data->spacing;
    tags[2].ti_Data = (IPTR) data->dri;
    data->mximage = (struct Image *) NewObjectA(NULL, SYSICLASS, tags);

    if ((!data->dri) || (!data->labels) || (!data->mximage) || (!data->numlabels)) {
	CoerceMethod(cl, obj, OM_DISPOSE);
	return NULL;
    }
    return obj;
}


IPTR mx_set(Class *cl, Object *obj, struct opSet *msg)
{
    IPTR retval = FALSE;
    struct MXData *data = INST_DATA(cl, obj);
    struct TagItem *tag;
    struct RastPort *rport;

    retval = DoSuperMethodA(cl, obj, (Msg)msg);

    tag = FindTagItem(GTMX_Active, msg->ops_AttrList);
    if (tag) {
        if ((tag->ti_Data >= 0) && (tag->ti_Data < data->numlabels)) {
            data->active = tag->ti_Data;
            retval = TRUE;
        }
    }

    if (FindTagItem(GA_Disabled, msg->ops_AttrList))
        retval = TRUE;

    if ((retval) && (((Class *) (*(obj - sizeof(Class *)))) == cl)) {
	rport = ObtainGIRPort(msg->ops_GInfo);
	if (rport) {
	    DoMethod(obj, GM_RENDER, msg->ops_GInfo, rport, GREDRAW_REDRAW);
	    ReleaseGIRPort(rport);
	    retval = FALSE;
	}
    }
    return retval;
}


IPTR mx_render(Class * cl, Object * obj, struct gpRender * msg)
{
    struct MXData *data = INST_DATA(cl, obj);
    WORD ypos = G(obj)->TopEdge;
    int y, blobheight;

    blobheight = (G(obj)->Height - data->spacing * (data->numlabels - 1)) / data->numlabels;

    if (msg->gpr_Redraw == GREDRAW_UPDATE) {
        blobheight += data->spacing;
        DrawImageState(msg->gpr_RPort, data->mximage,
                       G(obj)->LeftEdge, ypos + data->active * blobheight,
                       IDS_NORMAL, data->dri);
        DrawImageState(msg->gpr_RPort, data->mximage,
                       G(obj)->LeftEdge, ypos + data->newactive * blobheight,
                       IDS_SELECTED, data->dri);
        /* disabled !!! */
    } else {
        for (y = 0; y < data->numlabels; y++) {
            ULONG state;

            if (y == data->active)
                state = IDS_SELECTED;
            else
                state = IDS_NORMAL;
            DrawImageState(msg->gpr_RPort, data->mximage,
                           G(obj)->LeftEdge, ypos,
                           state, data->dri);
            ypos += data->spacing + blobheight;
        }
        /* !!! labels */
        /* !!! disabled */
    }

    return TRUE;
}


IPTR mx_goactive(Class * cl, Object * obj, struct gpInput * msg)
{
    IPTR retval = GMR_REUSE;
    struct MXData *data = INST_DATA(cl, obj);

    if ((msg->gpi_Mouse.X >= G(obj)->LeftEdge) &&
        (msg->gpi_Mouse.X < G(obj)->LeftEdge + G(obj)->Width) &&
        (msg->gpi_Mouse.Y >= G(obj)->TopEdge) &&
        (msg->gpi_Mouse.Y < G(obj)->TopEdge + G(obj)->Height)) {
        int y, blobheight = (G(obj)->Height - data->spacing * (data->numlabels -1)) / data->numlabels;

        retval = GMR_NOREUSE;

        for (y = 0; y < data->numlabels; y++) {
            if (msg->gpi_Mouse.Y < (G(obj)->TopEdge + blobheight * (y + 1))) {
                if (y != data->active) {
                    struct RastPort *rport;

                    rport = ObtainGIRPort(msg->gpi_GInfo);
                    if (rport) {
                        data->newactive = y;
                        DoMethod(obj, GM_RENDER, msg->gpi_GInfo, rport, GREDRAW_UPDATE);
                        ReleaseGIRPort(rport);
                        data->active = y;
                    }
                }
            }
        }
    }

    return retval;
}


AROS_UFH3(static IPTR, dispatch_mxclass,
	  AROS_UFHA(Class *, cl, A0),
	  AROS_UFHA(Object *, obj, A2),
	  AROS_UFHA(Msg, msg, A1)
)
{
    IPTR retval = 0UL;
    struct MXData *data;

    switch (msg->MethodID) {
    case OM_NEW:
	retval = (IPTR) mx_new(cl, (Class *) obj, (struct opSet *) msg);
	break;

    case OM_SET:
        retval = mx_set(cl, obj, (struct opSet *)msg);
        break;

    case OM_GET:
        data = INST_DATA(cl, obj);
        if (OPG(msg)->opg_AttrID == GTMX_Active) {
            *OPG(msg)->opg_Storage = data->active;
            retval = 1UL;
        } else
            retval = DoSuperMethodA(cl, obj, msg);
        break;

    case GM_RENDER:
	retval = mx_render(cl, obj, (struct gpRender *) msg);
	break;

    case GM_GOACTIVE:
        retval = mx_goactive(cl, obj, (struct gpInput *) msg);
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


Class *makemxclass(struct GadToolsBase_intern * GadToolsBase)
{
    Class *cl;

    if (GadToolsBase->mxclass)
	return GadToolsBase->mxclass;

    cl = MakeClass(NULL, GADGETCLASS, NULL, sizeof(struct MXData), 0UL);
    if (!cl)
	return NULL;
    cl->cl_Dispatcher.h_Entry = (APTR) AROS_ASMSYMNAME(dispatch_mxclass);
    cl->cl_Dispatcher.h_SubEntry = NULL;
    cl->cl_UserData = (IPTR) GadToolsBase;

    GadToolsBase->mxclass = cl;

    return cl;
}
