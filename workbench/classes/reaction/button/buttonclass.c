/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction button.gadget - BOOPSI class implementation
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <proto/alib.h>

#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/cghooks.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <gadgets/button.h>
#include <utility/tagitem.h>

#include <string.h>

#include "button_intern.h"

#define ButtonBase ((struct Library *)(cl->cl_UserData))

/******************************************************************************/

static void button_set(Class *cl, Object *o, struct opSet *msg)
{
    struct ButtonData *data = INST_DATA(cl, o);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case BUTTON_Pushed:
                data->bd_Pushed = (BOOL)tag->ti_Data;
                break;
            case BUTTON_Child:
                data->bd_Child = (Object *)tag->ti_Data;
                break;
            case BUTTON_SelChild:
                data->bd_SelChild = (Object *)tag->ti_Data;
                break;
            case BUTTON_AutoButton:
                data->bd_AutoButton = tag->ti_Data;
                break;
            case BUTTON_BevelStyle:
                data->bd_BevelStyle = tag->ti_Data;
                break;
            case BUTTON_Justification:
                data->bd_Justification = tag->ti_Data;
                break;
            case BUTTON_SoftStyle:
                data->bd_SoftStyle = tag->ti_Data;
                break;
            case BUTTON_TextPen:
                data->bd_TextPen = (UWORD)tag->ti_Data;
                break;
            case BUTTON_BackgroundPen:
                data->bd_BackgroundPen = (UWORD)tag->ti_Data;
                break;
            case BUTTON_FillTextPen:
                data->bd_FillTextPen = (UWORD)tag->ti_Data;
                break;
            case BUTTON_FillPen:
                data->bd_FillPen = (UWORD)tag->ti_Data;
                break;
            case BUTTON_Transparent:
                data->bd_Transparent = (BOOL)tag->ti_Data;
                break;
            case BUTTON_RenderMode:
                data->bd_RenderMode = tag->ti_Data;
                break;
            case BUTTON_DomainString:
                data->bd_DomainString = (STRPTR)tag->ti_Data;
                break;
        }
    }
}

/******************************************************************************/

IPTR Button__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval)
    {
        struct ButtonData *data = INST_DATA(cl, (Object *)retval);

        memset(data, 0, sizeof(struct ButtonData));
        data->bd_Justification = BCJ_CENTER;

        button_set(cl, (Object *)retval, msg);
    }

    return retval;
}

/******************************************************************************/

IPTR Button__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct ButtonData *data = INST_DATA(cl, o);

    if (data->bd_Child)
    {
        DisposeObject(data->bd_Child);
        data->bd_Child = NULL;
    }
    if (data->bd_SelChild)
    {
        DisposeObject(data->bd_SelChild);
        data->bd_SelChild = NULL;
    }

    return DoSuperMethodA(cl, o, msg);
}

/******************************************************************************/

IPTR Button__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval = DoSuperMethodA(cl, o, (Msg)msg);
    button_set(cl, o, msg);
    return retval;
}

/******************************************************************************/

IPTR Button__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct ButtonData *data = INST_DATA(cl, o);

    switch (msg->opg_AttrID)
    {
        case BUTTON_Pushed:
            *msg->opg_Storage = data->bd_Pushed;
            return TRUE;

        case BUTTON_Justification:
            *msg->opg_Storage = data->bd_Justification;
            return TRUE;

        case BUTTON_AutoButton:
            *msg->opg_Storage = data->bd_AutoButton;
            return TRUE;

        case BUTTON_BevelStyle:
            *msg->opg_Storage = data->bd_BevelStyle;
            return TRUE;
    }

    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

IPTR Button__GM_RENDER(Class *cl, Object *o, struct gpRender *msg)
{
    struct ButtonData *data = INST_DATA(cl, o);
    struct RastPort *rp = msg->gpr_RPort;
    struct DrawInfo *dri = msg->gpr_GInfo ? msg->gpr_GInfo->gi_DrInfo : NULL;
    struct Gadget *gad = G(o);
    WORD x, y;
    BOOL selected;

    if (!rp && msg->gpr_GInfo)
        rp = ObtainGIRPort(msg->gpr_GInfo);

    if (!rp)
        return FALSE;

    x = gad->LeftEdge;
    y = gad->TopEdge;
    selected = (gad->Flags & GFLG_SELECTED) || data->bd_Pushed;

    /* Draw button frame - use super class rendering first */
    DoSuperMethodA(cl, o, (Msg)msg);

    /* Draw child image if present */
    if (data->bd_Child)
    {
        Object *img = selected && data->bd_SelChild ? data->bd_SelChild : data->bd_Child;
        UWORD state = selected ? IDS_SELECTED : IDS_NORMAL;

        if (gad->Flags & GFLG_DISABLED)
            state = IDS_DISABLED;

        DrawImageState(rp, (struct Image *)img, x, y, state, dri);
    }

    if (!msg->gpr_RPort && msg->gpr_GInfo)
        ReleaseGIRPort(rp);

    return TRUE;
}

/******************************************************************************/

IPTR Button__GM_GOACTIVE(Class *cl, Object *o, struct gpInput *msg)
{
    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

IPTR Button__GM_HANDLEINPUT(Class *cl, Object *o, struct gpInput *msg)
{
    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

IPTR Button__GM_GOINACTIVE(Class *cl, Object *o, struct gpGoInactive *msg)
{
    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

IPTR Button__GM_DOMAIN(Class *cl, Object *o, struct gpDomain *msg)
{
    struct ButtonData *data = INST_DATA(cl, o);
    struct Gadget *gad = G(o);
    STRPTR text = NULL;
    UWORD minW = 40, minH = 14;

    /* Use domain string or gadget text for sizing */
    if (data->bd_DomainString)
        text = data->bd_DomainString;
    else if (gad->GadgetText)
        text = gad->GadgetText->IText;

    if (text && msg->gpd_RPort)
    {
        struct TextExtent te;
        TextExtent(msg->gpd_RPort, text, strlen(text), &te);
        minW = te.te_Width + 16;
        minH = te.te_Height + 6;
    }

    msg->gpd_Domain.Left   = 0;
    msg->gpd_Domain.Top    = 0;
    msg->gpd_Domain.Width  = minW;
    msg->gpd_Domain.Height = minH;

    return TRUE;
}
