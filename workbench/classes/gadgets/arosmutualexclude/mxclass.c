/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: AROS specific mutualexclude class implementation.
    Lang: english
*/
#include <exec/libraries.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/imageclass.h>
#include <intuition/intuition.h>
#include <proto/graphics.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <proto/utility.h>
#include <utility/tagitem.h>
#include <devices/inputevent.h>
#include <gadgets/arosmx.h>
#include <proto/alib.h>

#ifndef DEBUG
#   define DEBUG 0
#endif
#include <aros/debug.h>

#include "arosmutualexclude_intern.h"


#undef AROSMutualExcludeBase
#define AROSMutualExcludeBase ((struct MXBase_intern *)(cl->cl_UserData))



void mx_setnew(Class * cl, Object * obj, struct opSet *msg)
{
    struct MXData *data = INST_DATA(cl, obj);
    struct TagItem *tag, *taglist = msg->ops_AttrList;

    while ((tag = NextTagItem(&taglist))) {
	switch (tag->ti_Tag) {
	case GA_DrawInfo:
	    data->dri = (struct DrawInfo *) tag->ti_Data;
	    break;
        case GA_TextAttr:
            data->tattr = (struct TextAttr *) tag->ti_Data;
            break;
        case GA_LabelPlace:
            data->labelplace = (LONG) tag->ti_Data;
            break;
	case AROSMX_Active:
	    data->active = tag->ti_Data;
	    break;
	case AROSMX_Labels:
	    data->labels = (STRPTR *) tag->ti_Data;
	    data->numlabels = 0;
	    while (data->labels[data->numlabels])
		data->numlabels++;
	    break;
	case AROSMX_Spacing:
	    data->spacing = tag->ti_Data;
	    break;
        case AROSMX_TickLabelPlace:
            data->ticklabelplace = (LONG) tag->ti_Data;
            break;
	}
    }
}


Object *mx_new(Class * cl, Class * rootcl, struct opSet *msg)
{
    struct MXData *data;
    Object *obj;
    struct TagItem tags[] =
    {
	{IA_Width, 0},
	{IA_Height, 0},
	{SYSIA_DrawInfo, (IPTR) NULL},
	{SYSIA_Which, MXIMAGE},
	{TAG_DONE, 0L}
    };

    obj = (Object *) DoSuperMethodA(cl, (Object *) rootcl, (Msg)msg);
    if (!obj)
	return NULL;

    G(obj)->Activation = GACT_IMMEDIATE;

    data = INST_DATA(cl, obj);
    data->dri = NULL;
    data->tattr = NULL;
    data->active = 0;
    data->labels = NULL;
    data->spacing = 1;
    data->labelplace = GV_LabelPlace_Above;
    data->ticklabelplace = GV_LabelPlace_Right;
    mx_setnew(cl, obj, msg);

    /* Calculate fontheight */
    if (data->tattr)
        data->fontheight = data->tattr->ta_YSize;
    else if ((G(obj)->Flags & GFLG_LABELITEXT) && (G(obj)->GadgetText))
        data->fontheight = G(obj)->GadgetText->ITextFont->ta_YSize;
    else
        data->fontheight = G(obj)->Height;

    /* Calculate gadget size */
    if (G(obj)->Width == 0)
        G(obj)->Width = MX_WIDTH;
    G(obj)->Height = (data->fontheight + data->spacing) * data->numlabels -
                     data->spacing;

    tags[0].ti_Data = G(obj)->Width;
    tags[1].ti_Data = GetTagData(AROSMX_TickHeight, MX_HEIGHT, msg->ops_AttrList);
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
    struct TagItem *tag, *taglist = msg->ops_AttrList;

    if (msg->MethodID != OM_NEW)
        retval = DoSuperMethodA(cl, obj, (Msg)msg);

    while ((tag = NextTagItem(&taglist))) {
	switch (tag->ti_Tag) {
        case GA_Disabled:
            retval = TRUE;
            break;
	case AROSMX_Active:
            if ((tag->ti_Data >= 0) && (tag->ti_Data < data->numlabels)) {
                data->active = tag->ti_Data;
                retval = TRUE;
            }
            break;
	}
    }

    if ((retval) && (((Class *) (*(obj - sizeof(Class *)))) == cl))
    {
        struct RastPort *rport;

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
    int y;

    if (msg->gpr_Redraw == GREDRAW_UPDATE) {
        /* Only redraw the current and the last tick activated */
        DrawImageState(msg->gpr_RPort, data->mximage,
                       G(obj)->LeftEdge, ypos + data->active * (data->fontheight + data->spacing),
                       IDS_NORMAL, data->dri);
        DrawImageState(msg->gpr_RPort, data->mximage,
                       G(obj)->LeftEdge, ypos + data->newactive * (data->fontheight + data->spacing),
                       IDS_SELECTED, data->dri);
    } else {
        /* Full redraw */
        STRPTR *labels;

        /* Draw ticks */
        for (y=0; y<data->numlabels; y++)
        {
            ULONG state;

            if (y == data->active)
                state = IDS_SELECTED;
            else
                state = IDS_NORMAL;
            DrawImageState(msg->gpr_RPort, data->mximage,
                           G(obj)->LeftEdge, ypos,
                           state, data->dri);
            ypos += data->fontheight + data->spacing;
        }

        /* Draw main label */
        renderlabel(AROSMutualExcludeBase,
                    G(obj), msg->gpr_RPort,
                    data->labelplace, data->ticklabelplace);

        /* Draw labels */
        SetABPenDrMd(msg->gpr_RPort,
                     data->dri->dri_Pens[TEXTPEN],
                     data->dri->dri_Pens[BACKGROUNDPEN],
                     JAM1);
        ypos = G(obj)->TopEdge + msg->gpr_RPort->Font->tf_Baseline;
        for (labels=data->labels; *labels; labels++) {
            Move(msg->gpr_RPort, G(obj)->LeftEdge + G(obj)->Width + 5, ypos);
            Text(msg->gpr_RPort, *labels, strlen(*labels));
            ypos += data->fontheight + data->spacing;
        }
    }

    /* Draw disabled pattern */
    if (G(obj)->Flags & GFLG_DISABLED)
        drawdisabledpattern(AROSMutualExcludeBase, msg->gpr_RPort,
                            data->dri->dri_Pens[SHADOWPEN],
                            G(obj)->LeftEdge, G(obj)->TopEdge,
                            G(obj)->Width-1, G(obj)->Height-1);

    return TRUE;
}


IPTR mx_goactive(Class * cl, Object * obj, struct gpInput * msg)
{
    IPTR retval = GMR_NOREUSE;
    struct MXData *data = INST_DATA(cl, obj);
    int y, blobheight = data->spacing + data->fontheight;

    D(bug("blobheight: %d\n", blobheight));

    for (y = 0; y < data->numlabels; y++)
    {
        D(bug("Mouse.Y: %d, y: %d\n", msg->gpi_Mouse.Y, y));
        if (msg->gpi_Mouse.Y < blobheight * (y+1))
        {
            if (y != data->active)
            {
                struct RastPort *rport;

                rport = ObtainGIRPort(msg->gpi_GInfo);
                if (rport)
                {
                    data->newactive = y;
                    DoMethod(obj, GM_RENDER, msg->gpi_GInfo, rport, GREDRAW_UPDATE);
                    ReleaseGIRPort(rport);
                    *msg->gpi_Termination = data->active = y;
                    retval |= GMR_VERIFY;
                }
            }
            y = data->numlabels;
        }
    }

    return retval;
}


AROS_UFH3S(IPTR, dispatch_mxclass,
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

#define OPG(x) ((struct opGet *)(x))
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

#undef AROSMutualExcludeBase

struct IClass *InitMutualExcludeClass (struct MXBase_intern * AROSMutualExcludeBase)
{
    Class *cl = NULL;

    cl = MakeClass(AROSMXCLASS, GADGETCLASS, NULL, sizeof(struct MXData), 0);
    if (cl) {
	cl->cl_Dispatcher.h_Entry    = (APTR)AROS_ASMSYMNAME(dispatch_mxclass);
	cl->cl_Dispatcher.h_SubEntry = NULL;
	cl->cl_UserData 	     = (IPTR)AROSMutualExcludeBase;

	AddClass (cl);
    }

    return (cl);
}
