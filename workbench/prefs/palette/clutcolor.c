/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <clib/alib_protos.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <libraries/mui.h>
#include <zune/customclasses.h>
#include <graphics/gfxmacros.h>

#include "locale.h"
#include "paleditor.h"
#include "clutcolor.h"
#include "prefs.h"

/*** Instance data **********************************************************/
struct CLUTColor_DATA
{
    UBYTE colorindex;
};

IPTR CLUTColor__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct CLUTColor_DATA *data;
    struct TagItem *tags;
    struct TagItem *tag;
    ULONG *rgb;

    obj = (Object *) DoSuperMethodA(cl, obj, (Msg) msg);
    if (!obj)
        return FALSE;

    data = INST_DATA(cl, obj);

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_CLUTColor_Index:
            data->colorindex = (UBYTE) tag->ti_Data;
            break;

        }
    }

    return (IPTR) obj;
}

IPTR CLUTColor__OM_SET(struct IClass *cl, Object *obj,
    struct opSet *msg)
{
    struct CLUTColor_DATA *data;
    struct TagItem *tags;
    struct TagItem *tag;
    IPTR retval;

    data = INST_DATA(cl, obj);

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_CLUTColor_Index:
            data->colorindex = (UBYTE) tag->ti_Data;
            break;

        }
    }

    retval = DoSuperMethodA(cl, obj, (Msg) msg);

    return retval;
}

IPTR CLUTColor__OM_GET(struct IClass *cl, Object *obj,
    struct opGet *msg)
{
    struct CLUTColor_DATA *data = INST_DATA(cl, obj);
    IPTR *store = msg->opg_Storage;

    switch (msg->opg_AttrID)
    {
    case MUIA_CLUTColor_Index:
        *store = data->colorindex;
        break;

    default:
        return DoSuperMethodA(cl, obj, (Msg) msg);
    }

    return TRUE;
}

IPTR CLUTColor__MUIM_Setup(struct IClass *cl, Object *obj,
    struct MUIP_Setup *msg)
{
    struct CLUTColor_DATA *data = INST_DATA(cl, obj);
    LONG pen;

    if (!(DoSuperMethodA(cl, obj, (Msg) msg)))
        return 0;

    return 1;
}

IPTR CLUTColor__MUIM_Cleanup(struct IClass *cl, Object *obj,
    struct MUIP_Cleanup *msg)
{
    struct CLUTColor_DATA *data = INST_DATA(cl, obj);

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

IPTR CLUTColor__MUIM_AskMinMax(struct IClass *cl, Object *obj,
    struct MUIP_AskMinMax *msg)
{
    DoSuperMethodA(cl, obj, (Msg) msg);

    msg->MinMaxInfo->MinWidth += 1;
    msg->MinMaxInfo->MinHeight += 1;
    msg->MinMaxInfo->DefWidth += 16;
    msg->MinMaxInfo->DefHeight += 16;
    msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;
    msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;

    return 0;
}

IPTR CLUTColor__MUIM_Draw(struct IClass *cl, Object *obj,
    struct MUIP_Draw *msg)
{
    struct CLUTColor_DATA *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg) msg);

    if (!(msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE)))
        return FALSE;

    SetABPenDrMd(_rp(obj), data->colorindex, 0, JAM1);

    RectFill(_rp(obj), _mleft(obj), _mtop(obj), _mright(obj),
        _mbottom(obj));

    SetAfPt(_rp(obj), NULL, 0);

    return 0;
}

ZUNE_CUSTOMCLASS_7
(
    CLUTColor, NULL, MUIC_Area, NULL,
    OM_NEW,                     struct opSet *,
    OM_SET,                     struct opSet *,
    OM_GET,                     struct opGet *,
    MUIM_Setup,                 struct MUIP_Setup *,
    MUIM_Cleanup,               struct MUIP_Cleanup *,
    MUIM_AskMinMax,             struct MUIP_AskMinMax *,
    MUIM_Draw,                  struct MUIP_Draw *
);
