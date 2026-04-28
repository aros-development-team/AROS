/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction space.gadget - BOOPSI class implementation
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
#include <gadgets/space.h>
#include <utility/tagitem.h>

#include <string.h>

#include "space_intern.h"

#define SpaceBase ((struct Library *)(cl->cl_UserData))

/******************************************************************************/

static void space_set(Class *cl, Object *o, struct opSet *msg)
{
    struct SpaceData *data = INST_DATA(cl, o);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case SPACE_MinWidth:
                data->sd_MinWidth = (UWORD)tag->ti_Data;
                break;
            case SPACE_MinHeight:
                data->sd_MinHeight = (UWORD)tag->ti_Data;
                break;
            case SPACE_BevelStyle:
                data->sd_BevelStyle = tag->ti_Data;
                break;
            case SPACE_Transparent:
                data->sd_Transparent = (BOOL)tag->ti_Data;
                break;
        }
    }
}

/******************************************************************************/

IPTR Space__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval)
    {
        struct SpaceData *data = INST_DATA(cl, (Object *)retval);

        memset(data, 0, sizeof(struct SpaceData));
        data->sd_MinWidth   = 1;
        data->sd_MinHeight  = 1;
        data->sd_BevelStyle = 0;
        data->sd_Transparent = FALSE;

        space_set(cl, (Object *)retval, msg);
    }

    return retval;
}

/******************************************************************************/

IPTR Space__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    return DoSuperMethodA(cl, o, msg);
}

/******************************************************************************/

IPTR Space__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval = DoSuperMethodA(cl, o, (Msg)msg);
    space_set(cl, o, msg);
    return retval;
}

/******************************************************************************/

IPTR Space__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct SpaceData *data = INST_DATA(cl, o);

    switch (msg->opg_AttrID)
    {
        case SPACE_MinWidth:
            *msg->opg_Storage = data->sd_MinWidth;
            return TRUE;

        case SPACE_MinHeight:
            *msg->opg_Storage = data->sd_MinHeight;
            return TRUE;

        case SPACE_BevelStyle:
            *msg->opg_Storage = data->sd_BevelStyle;
            return TRUE;

        case SPACE_Transparent:
            *msg->opg_Storage = data->sd_Transparent;
            return TRUE;
    }

    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

IPTR Space__GM_RENDER(Class *cl, Object *o, struct gpRender *msg)
{
    struct SpaceData *data = INST_DATA(cl, o);
    struct RastPort *rp = msg->gpr_RPort;
    struct DrawInfo *dri = msg->gpr_GInfo ? msg->gpr_GInfo->gi_DrInfo : NULL;
    struct Gadget *gad = G(o);
    WORD x, y, w, h;

    if (!rp && msg->gpr_GInfo)
        rp = ObtainGIRPort(msg->gpr_GInfo);

    if (!rp)
        return FALSE;

    x = gad->LeftEdge;
    y = gad->TopEdge;
    w = gad->Width;
    h = gad->Height;

    /* Fill background unless transparent */
    if (!data->sd_Transparent)
    {
        SetAPen(rp, dri ? dri->dri_Pens[BACKGROUNDPEN] : 0);
        RectFill(rp, x, y, x + w - 1, y + h - 1);
    }

    /* Draw beveled frame if a bevel style is set */
    if (data->sd_BevelStyle != 0 && w > 2 && h > 2)
    {
        UWORD shine  = dri ? dri->dri_Pens[SHINEPEN]  : 2;
        UWORD shadow = dri ? dri->dri_Pens[SHADOWPEN] : 1;

        /* Top and left edges */
        SetAPen(rp, shine);
        Move(rp, x, y + h - 1);
        Draw(rp, x, y);
        Draw(rp, x + w - 1, y);

        /* Bottom and right edges */
        SetAPen(rp, shadow);
        Move(rp, x + 1, y + h - 1);
        Draw(rp, x + w - 1, y + h - 1);
        Draw(rp, x + w - 1, y + 1);
    }

    if (!msg->gpr_RPort && msg->gpr_GInfo)
        ReleaseGIRPort(rp);

    return TRUE;
}
