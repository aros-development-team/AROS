/*
    Copyright © 2002-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <stdio.h>

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <graphics/gfxmacros.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <string.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "textengine.h"
#include "support.h"
#include "support_classes.h"
#include "colorfield_private.h"

/*  #define MYDEBUG 1 */
#include "debug.h"

extern struct Library *MUIMasterBase;

#define FLAG_FIXED_PEN 	    1
#define FLAG_PEN_ALLOCATED  2
#define FLAG_NO_PEN 	    4


/****** Colorfield.mui/MUIA_Colorfield_Pen ***********************************
*
*   NAME
*       MUIA_Colorfield_Pen -- (V4) [ISG], ULONG
*
*   FUNCTION
*       The used pen. If not set explicitly, a new one is obtained
*       automatically.
*
*       This attribute was incorrectly documented as gettable-only in the MUI
*       documentation.
*
******************************************************************************
*
*/

/****** Colorfield.mui/MUIA_Colorfield_Red ***********************************
*
*   NAME
*       MUIA_Colorfield_Red -- (V4) [ISG], ULONG
*
*   FUNCTION
*       The red component of the color field as an unsigned 32-bit value
*       (0 to 0xFFFFFFFF).
*
*   SEE ALSO
*       MUIA_Colorfield_RGB, MUIA_Colorfield_Green, MUIA_Colorfield_Blue
*
******************************************************************************
*
*/

/****** Colorfield.mui/MUIA_Colorfield_Green *********************************
*
*   NAME
*       MUIA_Colorfield_Green -- (V4) [ISG], ULONG
*
*   FUNCTION
*       The green component of the color field as an unsigned 32-bit value
*       (0 to 0xFFFFFFFF).
*
*   SEE ALSO
*       MUIA_Colorfield_RGB, MUIA_Colorfield_Red, MUIA_Colorfield_Blue
*
******************************************************************************
*
*/

/****** Colorfield.mui/MUIA_Colorfield_Blue **********************************
*
*   NAME
*       MUIA_Colorfield_Blue -- (V4) [ISG], ULONG
*
*   FUNCTION
*       The blue component of the color field as an unsigned 32-bit value
*       (0 to 0xFFFFFFFF).
*
*   SEE ALSO
*       MUIA_Colorfield_RGB, MUIA_Colorfield_Red, MUIA_Colorfield_Green
*
******************************************************************************
*
*/

/****** Colorfield.mui/MUIA_Colorfield_RGB ***********************************
*
*   NAME
*       MUIA_Colorfield_RGB -- (V4) [ISG], ULONG *
*
*   FUNCTION
*       The red, green and blue components of the color field as an array of
*       unsigned 32-bit values (0 to 0xFFFFFFFF).
*
*   SEE ALSO
*       MUIA_Colorfield_Red, MUIA_Colorfield_Green, MUIA_Colorfield_Blue
*
******************************************************************************
*
*/

IPTR Colorfield__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Colorfield_DATA *data;
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
        case MUIA_Colorfield_Red:
            data->rgb[0] = (ULONG) tag->ti_Data;
            break;

        case MUIA_Colorfield_Green:
            data->rgb[1] = (ULONG) tag->ti_Data;
            break;

        case MUIA_Colorfield_Blue:
            data->rgb[2] = (ULONG) tag->ti_Data;
            break;

        case MUIA_Colorfield_RGB:
            rgb = (ULONG *) tag->ti_Data;
            data->rgb[0] = *rgb++;
            data->rgb[1] = *rgb++;
            data->rgb[2] = *rgb++;
            break;

        case MUIA_Colorfield_Pen:
            if ((data->pen = (UBYTE) tag->ti_Data) != (UBYTE)-1)
                data->flags |= FLAG_FIXED_PEN;
            break;

        }
    }

    return (IPTR) obj;
}

IPTR Colorfield__OM_SET(struct IClass *cl, Object *obj,
    struct opSet *msg)
{
    struct Colorfield_DATA *data;
    struct opSet supMsg;
    struct TagItem *tags;
    struct TagItem *tag;
    ULONG *rgb;
    BOOL newcol = FALSE;
    IPTR retval;
    struct TagItem extra_tags[] = {{TAG_IGNORE, 0}, {TAG_IGNORE, 0},
        {TAG_IGNORE, 0}, {TAG_IGNORE, 0},
        {TAG_MORE, (IPTR)msg->ops_AttrList}};

    data = INST_DATA(cl, obj);

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Colorfield_Red:
            data->rgb[0] = (ULONG) tag->ti_Data;
            newcol = TRUE;
            extra_tags[3].ti_Tag = MUIA_Colorfield_RGB;
            break;

        case MUIA_Colorfield_Green:
            data->rgb[1] = (ULONG) tag->ti_Data;
            newcol = TRUE;
            extra_tags[3].ti_Tag = MUIA_Colorfield_RGB;
            break;

        case MUIA_Colorfield_Blue:
            data->rgb[2] = (ULONG) tag->ti_Data;
            newcol = TRUE;
            extra_tags[3].ti_Tag = MUIA_Colorfield_RGB;
            break;

        case MUIA_Colorfield_RGB:
            rgb = (ULONG *) tag->ti_Data;
            data->rgb[0] = *rgb++;
            data->rgb[1] = *rgb++;
            data->rgb[2] = *rgb++;
            newcol = TRUE;
            extra_tags[0].ti_Tag = MUIA_Colorfield_Red;
            extra_tags[1].ti_Tag = MUIA_Colorfield_Green;
            extra_tags[2].ti_Tag = MUIA_Colorfield_Blue;
            break;

        case MUIA_Colorfield_Pen:
            if ((data->flags & FLAG_PEN_ALLOCATED) && (data->cm))
            {
                ULONG disposepen = data->pen;
                struct ColorMap *cm = data->cm;
                data->flags &= ~(FLAG_PEN_ALLOCATED | FLAG_NO_PEN);
                data->cm = NULL;
                ReleasePen(cm, disposepen);
            }
            if ((data->pen = (UBYTE) tag->ti_Data) != (UBYTE)-1)
                data->flags |= FLAG_FIXED_PEN;
            newcol = TRUE;
            break;

        }
    }

    /* Prepare notification values for inter-dependent attributes */
    extra_tags[0].ti_Data = data->rgb[0];
    extra_tags[1].ti_Data = data->rgb[1];
    extra_tags[2].ti_Data = data->rgb[2];
    extra_tags[3].ti_Data = (IPTR) data->rgb;
    supMsg.MethodID = msg->MethodID;
    supMsg.ops_AttrList = extra_tags;
    supMsg.ops_GInfo = msg->ops_GInfo;
    
    retval = DoSuperMethodA(cl, obj, (Msg) &supMsg);

    if (newcol && (_flags(obj) & MADF_SETUP)
        && !(data->flags & FLAG_NO_PEN))
    {
        if (_screen(obj))
        {
            SetRGB32(&_screen(obj)->ViewPort, data->pen, data->rgb[0],
                data->rgb[1], data->rgb[2]);

            if (GetBitMapAttr(_rp(obj)->BitMap, BMA_DEPTH) > 8)
            {
                MUI_Redraw(obj, MADF_DRAWUPDATE);
            }
        }
    }

    return retval;
}

IPTR Colorfield__OM_GET(struct IClass *cl, Object *obj,
    struct opGet *msg)
{
    struct Colorfield_DATA *data = INST_DATA(cl, obj);
    IPTR *store = msg->opg_Storage;

    switch (msg->opg_AttrID)
    {
    case MUIA_Colorfield_Red:
        *store = data->rgb[0];
        break;

    case MUIA_Colorfield_Green:
        *store = data->rgb[1];
        break;

    case MUIA_Colorfield_Blue:
        *store = data->rgb[2];
        break;

    case MUIA_Colorfield_RGB:
        *(ULONG **) store = data->rgb;
        break;

    case MUIA_Colorfield_Pen:
        *store = data->pen;
        break;

    default:
        return DoSuperMethodA(cl, obj, (Msg) msg);
    }

    return TRUE;
}

void Colorfield_SetupPen(Object *obj, struct Colorfield_DATA *data)
{
    data->cm = _screen(obj)->ViewPort.ColorMap;

    if (data->flags & FLAG_FIXED_PEN)
    {
        SetRGB32(&_screen(obj)->ViewPort,
            data->pen, data->rgb[0], data->rgb[1], data->rgb[2]);
    }
    else
    {
        LONG pen;

        pen = ObtainPen(data->cm,
            (ULONG) -1,
            data->rgb[0], data->rgb[1], data->rgb[2], PENF_EXCLUSIVE);

        if (pen == -1)
        {
            data->flags |= FLAG_NO_PEN;
            data->pen = -1;
        }
        else
        {
            data->pen = (UBYTE) pen;
            data->flags |= FLAG_PEN_ALLOCATED;
        }    
    }
}

IPTR Colorfield__MUIM_Setup(struct IClass *cl, Object *obj,
    struct MUIP_Setup *msg)
{
    struct Colorfield_DATA *data = INST_DATA(cl, obj);

    if (!(DoSuperMethodA(cl, obj, (Msg) msg)))
        return 0;

    if (!_screen(obj))
        return 1;

    Colorfield_SetupPen(obj, data);

    return 1;
}

IPTR Colorfield__MUIM_Cleanup(struct IClass *cl, Object *obj,
    struct MUIP_Cleanup *msg)
{
    struct Colorfield_DATA *data = INST_DATA(cl, obj);

    if (data->flags & FLAG_PEN_ALLOCATED)
    {
        ULONG disposepen = data->pen;
        struct ColorMap *cm = data->cm;
        data->flags &= ~FLAG_PEN_ALLOCATED;
        data->cm = NULL;
        data->pen = -1;
        ReleasePen(cm, disposepen);
    }
    data->flags &= ~FLAG_NO_PEN;

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

IPTR Colorfield__MUIM_AskMinMax(struct IClass *cl, Object *obj,
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

IPTR Colorfield__MUIM_Draw(struct IClass *cl, Object *obj,
    struct MUIP_Draw *msg)
{
    struct Colorfield_DATA *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg) msg);

    if (!(msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE)))
        return FALSE;

    if (!_rp(obj))
        return TRUE;

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

IPTR Colorfield__MUIM_ConnectParent(struct IClass *cl, Object *obj,
    struct MUIP_ConnectParent *msg)
{
    struct Colorfield_DATA *data = INST_DATA(cl, obj);
    IPTR retval;

    retval = DoSuperMethodA(cl, obj, (Msg) msg);

    if ((_flags(obj) & MADF_SETUP) && !(data->cm))
    {
        Colorfield_SetupPen(obj, data);
    }
    return retval;
}


#if ZUNE_BUILTIN_COLORFIELD
BOOPSI_DISPATCHER(IPTR, Colorfield_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Colorfield__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_SET:
        return Colorfield__OM_SET(cl, obj, (struct opSet *)msg);
    case OM_GET:
        return Colorfield__OM_GET(cl, obj, (struct opGet *)msg);
    case MUIM_Setup:
        return Colorfield__MUIM_Setup(cl, obj, (struct MUIP_Setup *)msg);
    case MUIM_Cleanup:
        return Colorfield__MUIM_Cleanup(cl, obj,
            (struct MUIP_Cleanup *)msg);
    case MUIM_ConnectParent:
        return Colorfield__MUIM_ConnectParent(cl, obj,
            (struct MUIP_ConnectParent *)msg);
    case MUIM_AskMinMax:
        return Colorfield__MUIM_AskMinMax(cl, obj,
            (struct MUIP_AskMinMax *)msg);
    case MUIM_Draw:
        return Colorfield__MUIM_Draw(cl, obj, (struct MUIP_Draw *)msg);
    default:
        return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Colorfield_desc =
{
    MUIC_Colorfield,
    MUIC_Area,
    sizeof(struct Colorfield_DATA),
    (void *) Colorfield_Dispatcher
};
#endif /* ZUNE_BUILTIN_COLORFIELD */
