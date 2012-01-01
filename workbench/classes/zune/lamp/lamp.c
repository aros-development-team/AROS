/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <proto/alib.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/graphics.h>

#include "Lamp_mcc.h"
#include "lamp_private.h"

//#define DEBUG 1
#include <aros/debug.h>

// Values for lmp_PenChanged and lmp_PenChangedOld
#define PCH_NONE        (0)
#define PCH_NEWPEN      (1)
#define PCH_NEWSPEC     (2)

// Lamp sizes from MUIV_Lamp_Type_Tiny to MUIV_Lamp_Type_Huge
const static ULONG lampsizes[] = {5, 6, 7, 8, 9};

// Lamp default colors
const static ULONG defaultcolors[][3] =
{
    {0x00000000, 0x00000000, 0x00000000}, // MUIV_Lamp_Color_Off
    {0x00000000, 0xffffffff, 0x00000000}, // MUIV_Lamp_Color_Ok
    {0x80808080, 0x00000000, 0x00000000}, // MUIV_Lamp_Color_Warning
    {0xc0c0c0c0, 0x00000000, 0x00000000}, // MUIV_Lamp_Color_Error
    {0xffffffff, 0x00000000, 0x00000000}, // MUIV_Lamp_Color_FatalError
    {0x00000000, 0xc0c0c0c0, 0x00000000}, // MUIV_Lamp_Color_Processing
    {0xc0c0c0c0, 0x00000000, 0x00000000}, // MUIV_Lamp_Color_LookingUp
    {0xffffffff, 0x00000000, 0x00000000}, // MUIV_Lamp_Color_Connecting
    {0xffffffff, 0xffffffff, 0x00000000}, // MUIV_Lamp_Color_SendingData
    {0x00000000, 0x00000000, 0xffffffff}, // MUIV_Lamp_Color_ReceivingData
    {0x00000000, 0x00000000, 0xffffffff}, // MUIV_Lamp_Color_LoadingData
    {0xffffffff, 0xffffffff, 0x00000000}, // MUIV_Lamp_Color_SavingData
};


/*** Methods ****************************************************************/
Object *Lamp__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Lamp_DATA           *data;
    struct TagItem             *tstate = msg->ops_AttrList;
    struct TagItem             *tag;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj) return FALSE;

    data = INST_DATA(cl, obj);

    // Defaults
    data->lmp_Type = MUIV_Lamp_Type_Medium;
    data->lmp_ColorSpec = MUIV_Lamp_Color_Off;
    data->lmp_Color[0] = defaultcolors[0][0];
    data->lmp_Color[1] = defaultcolors[0][1];
    data->lmp_Color[2] = defaultcolors[0][2];
    data->lmp_ColorType = MUIV_Lamp_ColorType_UserDefined;
    data->lmp_PenChanged = PCH_NEWPEN;
    data->lmp_PenChangedOld = PCH_NONE;
    data->lmp_PenNr = -1;

    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_Lamp_Type:
                data->lmp_Type = tag->ti_Data;
                break;

            case MUIA_Lamp_Color:
                if (tag->ti_Data < 12)
                {
                    data->lmp_ColorSpec = tag->ti_Data;
                    data->lmp_Color[0] = defaultcolors[data->lmp_ColorSpec][0];
                    data->lmp_Color[1] = defaultcolors[data->lmp_ColorSpec][1];
                    data->lmp_Color[2] = defaultcolors[data->lmp_ColorSpec][2];
                    data->lmp_ColorType = MUIV_Lamp_ColorType_UserDefined;
                }
                else
                {
                    ULONG *cols = (ULONG *)tag->ti_Data;
                    data->lmp_Color[0] = cols[0];
                    data->lmp_Color[1] = cols[1];
                    data->lmp_Color[2] = cols[2];
                    data->lmp_ColorType = MUIV_Lamp_ColorType_Color;
                }
                break;

            case MUIA_Lamp_Red:
                data->lmp_Color[0] = tag->ti_Data;
                data->lmp_ColorType = MUIV_Lamp_ColorType_Color;
                break; 

            case MUIA_Lamp_Green:
                data->lmp_Color[1] = tag->ti_Data;
                data->lmp_ColorType = MUIV_Lamp_ColorType_Color;
                break;

            case MUIA_Lamp_Blue:
                data->lmp_Color[2] = tag->ti_Data;
                data->lmp_ColorType = MUIV_Lamp_ColorType_Color;
                break;

            case MUIA_Lamp_PenSpec:
                data->lmp_PenSpec = *((struct MUI_PenSpec *)tag->ti_Data);
                data->lmp_ColorType = MUIV_Lamp_ColorType_PenSpec;
                break;
        }
    }

    return obj;
}


IPTR Lamp__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Lamp_DATA           *data   = INST_DATA(cl, obj);
    struct TagItem             *tstate = msg->ops_AttrList;
    struct TagItem             *tag;

    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_Lamp_Type:
                data->lmp_Type = tag->ti_Data;
                MUI_Redraw(obj, MADF_DRAWUPDATE);
                break;

            case MUIA_Lamp_Color:
                if (tag->ti_Data < 12)
                {
                    data->lmp_ColorSpec = tag->ti_Data;
                    data->lmp_Color[0] = defaultcolors[data->lmp_ColorSpec][0];
                    data->lmp_Color[1] = defaultcolors[data->lmp_ColorSpec][1];
                    data->lmp_Color[2] = defaultcolors[data->lmp_ColorSpec][2];
                    data->lmp_ColorType = MUIV_Lamp_ColorType_UserDefined;
                }
                else
                {
                    ULONG *cols = (ULONG *)tag->ti_Data;
                    data->lmp_Color[0] = cols[0];
                    data->lmp_Color[1] = cols[1];
                    data->lmp_Color[2] = cols[2];
                    data->lmp_ColorType = MUIV_Lamp_ColorType_Color;
                }
                data->lmp_PenChanged = PCH_NEWPEN;
                MUI_Redraw(obj, MADF_DRAWUPDATE);
                break;

            case MUIA_Lamp_Red:
                data->lmp_Color[0] = tag->ti_Data;
                data->lmp_ColorType = MUIV_Lamp_ColorType_Color;
                data->lmp_PenChanged = PCH_NEWPEN;
                MUI_Redraw(obj, MADF_DRAWUPDATE);
                break; 

            case MUIA_Lamp_Green:
                data->lmp_Color[1] = tag->ti_Data;
                data->lmp_ColorType = MUIV_Lamp_ColorType_Color;
                data->lmp_PenChanged = PCH_NEWPEN;
                MUI_Redraw(obj, MADF_DRAWUPDATE);
                break;

            case MUIA_Lamp_Blue:
                data->lmp_Color[2] = tag->ti_Data;
                data->lmp_ColorType = MUIV_Lamp_ColorType_Color;
                data->lmp_PenChanged = PCH_NEWPEN;
                MUI_Redraw(obj, MADF_DRAWUPDATE);
                break;

            case MUIA_Lamp_PenSpec:
                data->lmp_PenSpec = *((struct MUI_PenSpec *)tag->ti_Data);
                data->lmp_ColorType = MUIV_Lamp_ColorType_PenSpec;
                data->lmp_PenChanged = PCH_NEWSPEC;
                MUI_Redraw(obj, MADF_DRAWUPDATE);
                break;
        }
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}


IPTR Lamp__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Lamp_DATA   *data  = INST_DATA(cl, obj);
    IPTR               *store = msg->opg_Storage;

    switch (msg->opg_AttrID)
    {
        case MUIA_Lamp_Type:
            *store = data->lmp_Type;
            return TRUE;

        case MUIA_Lamp_Color:
            if (data->lmp_ColorType == MUIV_Lamp_ColorType_UserDefined)
            {
                *store = (IPTR)data->lmp_ColorSpec;
            }
            else if (data->lmp_ColorType == MUIV_Lamp_ColorType_Color)
            {
                *store = (IPTR)data->lmp_Color;
            }
            else // MUIV_Lamp_ColorType_PenSpec
            {
                *store = 0;
            }
            return TRUE;

        case MUIA_Lamp_ColorType:
            *store = data->lmp_ColorType;
            return TRUE;

        case MUIA_Lamp_Red:
            *store = data->lmp_Color[0];
            return TRUE;

        case MUIA_Lamp_Green:
            *store = data->lmp_Color[1];
            return TRUE;

        case MUIA_Lamp_Blue:
            *store = data->lmp_Color[2];
            return TRUE;

        case MUIA_Lamp_PenSpec:
            *store = (IPTR)&data->lmp_PenSpec;
            return TRUE;
    }
    
    return DoSuperMethodA(cl, obj, (Msg) msg);
}


IPTR Lamp__MUIM_Cleanup(struct IClass *cl, Object *obj, Msg msg)
{
    struct Lamp_DATA *data = INST_DATA(cl,obj);

    if (data->lmp_PenChangedOld == PCH_NEWPEN)
    {
        if (data->lmp_PenNr != -1)
        {
            ReleasePen(_screen(obj)->ViewPort.ColorMap, data->lmp_PenNr);
        }
    }
    else if (data->lmp_PenChangedOld == PCH_NEWSPEC)
    {
        MUI_ReleasePen(muiRenderInfo(obj), data->lmp_PenNr);
    }

    return DoSuperMethodA(cl, obj, msg);
}


IPTR Lamp__MUIM_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct Lamp_DATA *data = INST_DATA(cl, obj);

    ULONG right, bottom;

    DoSuperMethodA(cl, obj, (Msg)msg);

    if (!(msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE)))
        return 0;

    if (data->lmp_PenChanged)
    {
        if (data->lmp_PenChangedOld == PCH_NEWPEN)
        {
            if (data->lmp_PenNr != -1)
            {
                ReleasePen(_screen(obj)->ViewPort.ColorMap, data->lmp_PenNr);
                D(bug("[Lamp/Draw] released pen %u\n", data->lmp_PenNr));
                data->lmp_PenNr = -1;
            }
        }
        else if (data->lmp_PenChangedOld == PCH_NEWSPEC)
        {
            MUI_ReleasePen(muiRenderInfo(obj), data->lmp_PenNr);
            D(bug("[Lamp/Draw] released MUI pen %u\n", data->lmp_PenNr));
        }

        if (data->lmp_PenChanged == PCH_NEWPEN)
        {
            data->lmp_PenNr = ObtainBestPen
            (
                _screen(obj)->ViewPort.ColorMap,
                data->lmp_Color[0],
                data->lmp_Color[1],
                data->lmp_Color[2],
                OBP_Precision, PRECISION_GUI,
                TAG_DONE
            );
            D(bug("[Lamp/Draw] obtained pen %u r %u g %u b %u\n", data->lmp_PenNr,
            data->lmp_Color[0], data->lmp_Color[1], data->lmp_Color[2]));

            if (data->lmp_PenNr != -1)
            {
                SetAPen(_rp(obj), data->lmp_PenNr);
            }
        }
        else if (data->lmp_PenChanged == PCH_NEWSPEC)
        {
            data->lmp_PenNr = MUI_ObtainPen(muiRenderInfo(obj), &data->lmp_PenSpec, 0);
            D(bug("[Lamp/Draw] obtained MUI pen %u\n", data->lmp_PenNr));
            SetAPen(_rp(obj), MUIPEN(data->lmp_PenNr));
        }

        data->lmp_PenChangedOld = data->lmp_PenChanged;
        data->lmp_PenChanged = PCH_NONE;
    }

    right = _mleft(obj) + lampsizes[data->lmp_Type];
    if (right > _mright(obj))
        right = _mright(obj);
    bottom = _mtop(obj) + lampsizes[data->lmp_Type];
    if (bottom > _mbottom(obj))
        bottom = _bottom(obj);

    D(bug("[Lamp/Draw] right %d bottom %d\n", right, bottom));

    RectFill(_rp(obj), _mleft(obj), _mtop(obj), right, bottom);

    return 0;
}


IPTR Lamp__MUIM_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct Lamp_DATA *data = INST_DATA(cl, obj);

    ULONG size = lampsizes[data->lmp_Type];

    DoSuperMethodA(cl, obj, (Msg)msg);

    msg->MinMaxInfo->MinWidth  += size;
    msg->MinMaxInfo->DefWidth  += size;
    msg->MinMaxInfo->MaxWidth  += size;

    msg->MinMaxInfo->MinHeight += size;
    msg->MinMaxInfo->DefHeight += size;
    msg->MinMaxInfo->MaxHeight += size;

    return 0;
}


IPTR Lamp__MUIM_Lamp_SetRGB(struct IClass *cl, Object *obj, struct MUIP_Lamp_SetRGB *msg)
{
    ULONG rgb[3];

    rgb[0] = msg->red;
    rgb[1] = msg->green;
    rgb[2] = msg->blue;

    SET(obj, MUIA_Lamp_Color, rgb);

    return 0;
}
