/*
    Copyright (C) 2011-2026, The AROS Development Team. All rights reserved.
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


/****** Lamp.mcc/MUIC_Lamp ************
*
*   NAME
*       MUIC_Lamp
*
*   SUPERCLASS
*       Area.mui
*
*   DESCRIPTION
*       Draw a colored status lamp from a preset, RGB components or a MUI
*       pen specification. Defaults: Type_Medium and Color_Off.
*
*   NOTES
*       Color and PenSpec inputs are copied. Getter pointers into instance
*       storage are borrowed and valid only while the object remains alive.
*
*   BUGS
*       An initial PenSpec does not select its pen for the first draw; set
*       PenSpec again after creation. Type is an unchecked 0..4 index and Set
*       does not relayout. Pen acquisition can fail without an error result;
*       Cleanup can overwrite a pending color acquisition kind.
*
*/
/****** Lamp.mcc/MUIA_Lamp_Type ************
*
*   NAME
*       MUIA_Lamp_Type
*
*   FUNCTION
*       Select the status lamp size.
*
*   INPUTS
*       Initialize/set/get; ULONG. Tiny (0), Small (1), Medium (2, default),
*       Big (3), Huge (4).
*
*   NOTES
*       Only 0..4 are safe: the size table is not range-checked. The
*       table sizes (5..9) augment layout bounds; Set does not relayout.
*
*/
/****** Lamp.mcc/MUIA_Lamp_Color ************
*
*   NAME
*       MUIA_Lamp_Color
*
*   FUNCTION
*       Select one of 12 status colors, or copy three ULONG RGB values.
*
*   INPUTS
*       Initialize/set/get; IPTR. Values 0..11 are MUIV_Lamp_Color_* presets;
*       other values are pointers to three readable ULONGs. Default Off (0).
*
*   NOTES
*       The getter returns a preset index in UserDefined mode, a borrowed
*       RGB array pointer in Color mode, or zero in PenSpec mode. Query ColorType
*       before interpreting it. Input RGB arrays are copied immediately; no
*       separate RGB-component or ColorType notifications are generated.
*
*/
/****** Lamp.mcc/MUIA_Lamp_ColorType ************
*
*   NAME
*       MUIA_Lamp_ColorType
*
*   FUNCTION
*       Identify the current color representation.
*
*   INPUTS
*       Get only; ULONG: UserDefined (0), Color/RGB (1), PenSpec (2).
*
*   NOTES
*       Indicates representation, not successful pen acquisition.
*
*/
/****** Lamp.mcc/MUIA_Lamp_Red ************
*
*   NAME
*       MUIA_Lamp_Red
*
*   FUNCTION
*       Set or read the stored red RGB component.
*
*   INPUTS
*       Initialize/set/get; ULONG, default 0.
*
*   NOTES
*       Set selects RGB mode, preserves the other components and redraws.
*       Get returns the saved value, even while PenSpec mode is selected.
*       No separate Color/ColorType notification is generated.
*
*/
/****** Lamp.mcc/MUIA_Lamp_Green ************
*
*   NAME
*       MUIA_Lamp_Green
*
*   FUNCTION
*       Set or read the stored green RGB component.
*
*   INPUTS
*       Initialize/set/get; ULONG, default 0.
*
*   NOTES
*       Set selects RGB mode, preserves the other components and redraws.
*       Get returns the saved value, even while PenSpec mode is selected.
*       No separate Color/ColorType notification is generated.
*
*/
/****** Lamp.mcc/MUIA_Lamp_Blue ************
*
*   NAME
*       MUIA_Lamp_Blue
*
*   FUNCTION
*       Set or read the stored blue RGB component.
*
*   INPUTS
*       Initialize/set/get; ULONG, default 0.
*
*   NOTES
*       Set selects RGB mode, preserves the other components and redraws.
*       Get returns the saved value, even while PenSpec mode is selected.
*       No separate Color/ColorType notification is generated.
*
*/
/****** Lamp.mcc/MUIA_Lamp_PenSpec ************
*
*   NAME
*       MUIA_Lamp_PenSpec
*
*   FUNCTION
*       Set a MUI pen specification.
*
*   INPUTS
*       Initialize/set/get; non-NULL struct MUI_PenSpec *.
*
*   NOTES
*       Input is copied. Get returns a borrowed pointer to instance storage.
*       Set after creation to activate MUI pen acquisition; construction alone
*       leaves RGB acquisition pending. Pen resolution is not reported.
*
*/
/****** Lamp.mcc/MUIM_Lamp_SetRGB ************
*
*   NAME
*       MUIM_Lamp_SetRGB
*
*   SYNOPSIS
*       DoMethod(obj, MUIM_Lamp_SetRGB, red, green, blue);
*
*   FUNCTION
*       Set three ULONG RGB components together.
*
*   RESULT
*       Zero; setter success is not reported.
*
*   NOTES
*       A Color notification may receive a temporary stack array. Copy its
*       contents during the callback; never retain that pointer.
*
*/

/*** Methods ****************************************************************/
/* Create the superclass, returning NULL on failure. Initialize Medium size,
 * black preset, RGB acquisition pending, and no acquired pen. Process tags
 * in order and copy RGB or PenSpec input. PenSpec changes representation
 * but does not change the pending RGB acquisition kind.
 */
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


/* Process color and size tags in order; color changes redraw, size changes do not relayout. */
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


/* Return selectors, stored RGB values or borrowed instance pointers according to ColorType. */
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


/* Release the acquired pen and restore the old acquisition kind as pending. */
IPTR Lamp__MUIM_Cleanup(struct IClass *cl, Object *obj, Msg msg)
{
    struct Lamp_DATA *data = INST_DATA(cl,obj);

    if (data->lmp_PenChangedOld == PCH_NEWPEN)
    {
        if (data->lmp_PenNr != -1)
        {
            ReleasePen(_screen(obj)->ViewPort.ColorMap, data->lmp_PenNr);
            data->lmp_PenNr = -1;
        }
    }
    else if (data->lmp_PenChangedOld == PCH_NEWSPEC)
    {
        MUI_ReleasePen(muiRenderInfo(obj), data->lmp_PenNr);
    }

    if (data->lmp_PenChangedOld != PCH_NONE)
    {
        data->lmp_PenChanged = data->lmp_PenChangedOld;
        data->lmp_PenChangedOld = PCH_NONE;
    }

    return DoSuperMethodA(cl, obj, msg);
}


/* Acquire a pending pen, redraw the color rectangle, and keep the pen until replaced or cleaned up. */
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

    /* Every draw, not just the one that obtained the pen: whoever drew last
     * (a list row's text, say) has left its own pen in the rastport. */
    if (data->lmp_PenChangedOld == PCH_NEWSPEC)
        SetAPen(_rp(obj), MUIPEN(data->lmp_PenNr));
    else if (data->lmp_PenChangedOld == PCH_NEWPEN && data->lmp_PenNr != -1)
        SetAPen(_rp(obj), data->lmp_PenNr);

    RectFill(_rp(obj), _mleft(obj), _mtop(obj), right, bottom);

    return 0;
}


/* Add the unchecked Type table entry to superclass dimensions. */
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


/* Set Color using a temporary RGB array; notifications may expose the stack pointer. */
IPTR Lamp__MUIM_Lamp_SetRGB(struct IClass *cl, Object *obj, struct MUIP_Lamp_SetRGB *msg)
{
    ULONG rgb[3];

    rgb[0] = msg->red;
    rgb[1] = msg->green;
    rgb[2] = msg->blue;

    SET(obj, MUIA_Lamp_Color, rgb);

    return 0;
}
