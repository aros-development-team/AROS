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
    UBYTE flags;
    UBYTE colorindex;
    UBYTE pen;
};

#define FLAG_PEN_ALLOCATED  1
#define FLAG_NO_PEN 	    2

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
            if (data->flags & FLAG_PEN_ALLOCATED)
            {
                ReleasePen(_screen(obj)->ViewPort.ColorMap, data->pen);
                data->flags &= ~(FLAG_PEN_ALLOCATED | FLAG_NO_PEN);
            }
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
        *store = data->pen;
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

    pen = ObtainPen(_screen(obj)->ViewPort.ColorMap,
        (ULONG)data->colorindex, 0, 0, 0, PEN_NO_SETCOLOR);

    if (pen == -1)
    {
        data->flags |= FLAG_NO_PEN;
        data->pen = 0;
    }
    else
    {
        data->pen = (UBYTE) pen;
        data->flags |= FLAG_PEN_ALLOCATED;
    }

    return 1;
}

IPTR CLUTColor__MUIM_Cleanup(struct IClass *cl, Object *obj,
    struct MUIP_Cleanup *msg)
{
    struct CLUTColor_DATA *data = INST_DATA(cl, obj);

    if (data->flags & FLAG_PEN_ALLOCATED)
    {
        ReleasePen(_screen(obj)->ViewPort.ColorMap, data->pen);
        data->flags &= ~FLAG_PEN_ALLOCATED;
        data->pen = 0;
    }
    data->flags &= ~FLAG_NO_PEN;

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

    if (data->flags & FLAG_NO_PEN)
    {
        static UWORD pat[] = { 0x1111, 0x2222, 0x4444, 0x8888 };

        SetAfPt(_rp(obj), pat, 2);
        SetABPenDrMd(_rp(obj), _pens(obj)[MPEN_SHADOW],
            _pens(obj)[MPEN_BACKGROUND], JAM2);
    }
    else
    {
        SetABPenDrMd(_rp(obj), data->pen, 0, JAM1);
    }

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
