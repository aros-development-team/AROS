/*
    Copyright (C) 2026, Andy Rexx. All rights reserved.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <devices/rawkeycodes.h>
#include <dos/dos.h>
#include <intuition/intuition.h>

#include <proto/alib.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include <stdio.h>
#include <string.h>

#include "stepper.h"
#include "stepper_private.h"

#define STEPPER_BUTTON_INSET 18
#define STEPPER_MIN_COLUMNS   3
#define STEPPER_MAX_COLUMNS   11
#define STEPPER_STRING_LEN    12

#define STEPPERF_CHECKALLSIZES (1UL << 0)
#define STEPPERF_REVERSE       (1UL << 1)
#define STEPPERF_REVLEFTRIGHT  (1UL << 2)
#define STEPPERF_REVUPDOWN     (1UL << 3)

enum StepperSide
{
    STEPPER_SIDE_NONE = 0,
    STEPPER_SIDE_LEFT,
    STEPPER_SIDE_RIGHT
};

/****** Stepper.mcc/MUIC_Stepper ********************************************
*
*   NAME
*       MUIC_Stepper
*
*   SUPERCLASS
*       String.mui
*
*   DESCRIPTION
*       Edit a signed Numeric-compatible value in a compact String field with
*       unit-step buttons on both sides. The normal arrangement is minus,
*       editable value, plus. MUIA_Numeric_Reverse exchanges the visible
*       button actions without changing the range or value.
*
*   NOTES
*       Stepper implements the public Numeric attributes and methods used by
*       ordinary Numeric consumers, but uses String.mui for text editing,
*       cursor handling, selection and refresh. The editor accepts signed
*       decimal input. Press Return to commit typed text; clicking either step
*       button commits the current editor value before applying its unit step.
*
*       Stepper adds no class-specific public attributes or methods. Numeric
*       Value notifications report committed changes. MUIA_Numeric_Format is
*       retained by MUIM_Numeric_Stringify; the editable field itself shows
*       plain signed decimal text so it remains directly editable.
*
*       The initial Min and Max select a fixed editor width from their decimal
*       representations, with a three-column minimum. Later range changes keep
*       that width; String.mui scrolls longer values when necessary.
*
*/

static LONG Stepper_Clamp(LONG value, LONG min, LONG max)
{
    if (min > max)
        return value;
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}

static void Stepper_SetFlag(ULONG *flags, ULONG mask, IPTR value)
{
    if (value)
        *flags |= mask;
    else
        *flags &= ~mask;
}

static LONG Stepper_DecimalLength(LONG value)
{
    char text[16];
    LONG len;

    len = snprintf(text, sizeof(text), "%ld", (long)value);
    if (len < 1)
        len = 1;
    return len;
}

static LONG Stepper_InitialColumns(struct TagItem *tags)
{
    struct TagItem *state = tags;
    struct TagItem *tag;
    LONG min = 0;
    LONG max = 100;
    LONG columns;
    LONG len;

    while ((tag = NextTagItem(&state)) != NULL)
    {
        if (tag->ti_Tag == MUIA_Numeric_Min)
            min = (LONG)tag->ti_Data;
        else if (tag->ti_Tag == MUIA_Numeric_Max)
            max = (LONG)tag->ti_Data;
    }

    columns = Stepper_DecimalLength(min);
    len = Stepper_DecimalLength(max);
    if (len > columns)
        columns = len;

    if (columns < STEPPER_MIN_COLUMNS)
        columns = STEPPER_MIN_COLUMNS;
    if (columns > STEPPER_MAX_COLUMNS)
        columns = STEPPER_MAX_COLUMNS;

    return columns;
}

static void Stepper_ButtonBounds(Object *obj, UBYTE side, LONG *left,
    LONG *right)
{
    IPTR inset = STEPPER_BUTTON_INSET;

    if (side == STEPPER_SIDE_LEFT)
    {
        get(obj, MUIA_InnerLeft, &inset);
        *left = _mleft(obj) - (LONG)inset;
        *right = _mleft(obj) - 1;
    }
    else
    {
        get(obj, MUIA_InnerRight, &inset);
        *left = _mright(obj) + 1;
        *right = _mright(obj) + (LONG)inset;
    }
}

static UBYTE Stepper_SideAt(Object *obj, LONG mouse_x, LONG mouse_y)
{
    LONG left;
    LONG right;

    if (mouse_y < _mtop(obj) || mouse_y > _mbottom(obj))
        return STEPPER_SIDE_NONE;

    Stepper_ButtonBounds(obj, STEPPER_SIDE_LEFT, &left, &right);
    if (mouse_x >= left && mouse_x <= right)
        return STEPPER_SIDE_LEFT;

    Stepper_ButtonBounds(obj, STEPPER_SIDE_RIGHT, &left, &right);
    if (mouse_x >= left && mouse_x <= right)
        return STEPPER_SIDE_RIGHT;

    return STEPPER_SIDE_NONE;
}

static LONG Stepper_ActionForSide(struct Stepper_DATA *data, UBYTE side)
{
    BOOL reverse = (data->st_Flags & STEPPERF_REVERSE) != 0;

    if (side == STEPPER_SIDE_LEFT)
        return reverse ? 1 : -1;
    if (side == STEPPER_SIDE_RIGHT)
        return reverse ? -1 : 1;
    return 0;
}

static void Stepper_DrawButton(Object *obj, struct Stepper_DATA *data,
    UBYTE side, ULONG text_pen)
{
    struct RastPort *rp = _rp(obj);
    LONG left;
    LONG right;
    LONG top = _mtop(obj);
    LONG bottom = _mbottom(obj);
    LONG action;
    LONG x;
    LONG y;
    BOOL pressed = data->st_PressedSide == side;
    char text[2];

    Stepper_ButtonBounds(obj, side, &left, &right);
    if (right < left || bottom < top)
        return;

    DoMethod(obj, MUIM_DrawBackground, left, top, right - left + 1,
        bottom - top + 1, left, top, 0);

    if (side == STEPPER_SIDE_LEFT)
    {
        SetAPen(rp, _pens(obj)[pressed ? MPEN_SHINE : MPEN_SHADOW]);
        Move(rp, right, top);
        Draw(rp, right, bottom);
    }
    else
    {
        SetAPen(rp, _pens(obj)[pressed ? MPEN_SHADOW : MPEN_SHINE]);
        Move(rp, left, top);
        Draw(rp, left, bottom);
    }

    action = Stepper_ActionForSide(data, side);
    text[0] = action > 0 ? '+' : '-';
    text[1] = '\0';

    SetFont(rp, _font(obj));
    x = left + ((right - left + 1 - TextLength(rp, text, 1)) / 2);
    y = top + ((bottom - top + 1 - _font(obj)->tf_YSize) / 2)
        + _font(obj)->tf_Baseline;

    if (pressed)
    {
        x++;
        y++;
    }

    SetABPenDrMd(rp, text_pen, _pens(obj)[MPEN_BACKGROUND], JAM1);
    Move(rp, x, y);
    Text(rp, text, 1);
}

static void Stepper_SyncEditor(Object *obj, LONG value)
{
    nnset(obj, MUIA_String_Integer, value);
}

static void Stepper_CommitEditor(Object *obj)
{
    IPTR value = 0;

    get(obj, MUIA_String_Integer, &value);
    set(obj, MUIA_Numeric_Value, (LONG)value);
}

static BOOL Stepper_Adjust(Object *obj, struct Stepper_DATA *data,
    LONG action)
{
    LONG value = data->st_Value;

    if (data->st_Min > data->st_Max)
        return FALSE;

    if (action > 0 && value < data->st_Max)
    {
        set(obj, MUIA_Numeric_Value, value + 1);
        return TRUE;
    }

    if (action < 0 && value > data->st_Min)
    {
        set(obj, MUIA_Numeric_Value, value - 1);
        return TRUE;
    }

    return FALSE;
}

Object *Stepper__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Stepper_DATA *data;
    struct TagItem *state;
    struct TagItem *tag;
    LONG columns;
    BOOL value_set = FALSE;

    columns = Stepper_InitialColumns(msg->ops_AttrList);

    obj = (Object *)DoSuperNewTags
    (
        cl, obj, NULL,
        MUIA_Frame,             MUIV_Frame_String,
        MUIA_InnerLeft,         STEPPER_BUTTON_INSET,
        MUIA_InnerRight,        STEPPER_BUTTON_INSET,
        MUIA_String_Accept,     (IPTR)"-0123456789",
        MUIA_String_MaxLen,     STEPPER_STRING_LEN,
        MUIA_String_Columns,    columns,
        MUIA_String_Format,     MUIV_String_Format_Right,
        MUIA_CycleChain,        TRUE,
        TAG_MORE,               (IPTR)msg->ops_AttrList
    );

    if (!obj)
        return NULL;

    data = INST_DATA(cl, obj);
    data->st_Min = 0;
    data->st_Max = 100;
    data->st_Default = 0;
    data->st_Value = 0;
    data->st_Format = "%ld";
    data->st_PressedSide = STEPPER_SIDE_NONE;
    data->st_Flags = 0;

    state = msg->ops_AttrList;
    while ((tag = NextTagItem(&state)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Numeric_CheckAllSizes:
            Stepper_SetFlag(&data->st_Flags, STEPPERF_CHECKALLSIZES,
                tag->ti_Data);
            break;
        case MUIA_Numeric_Default:
            data->st_Default = (LONG)tag->ti_Data;
            break;
        case MUIA_Numeric_Format:
            data->st_Format = (STRPTR)tag->ti_Data;
            break;
        case MUIA_Numeric_Max:
            data->st_Max = (LONG)tag->ti_Data;
            break;
        case MUIA_Numeric_Min:
            data->st_Min = (LONG)tag->ti_Data;
            break;
        case MUIA_Numeric_Reverse:
            Stepper_SetFlag(&data->st_Flags, STEPPERF_REVERSE,
                tag->ti_Data);
            break;
        case MUIA_Numeric_RevLeftRight:
            Stepper_SetFlag(&data->st_Flags, STEPPERF_REVLEFTRIGHT,
                tag->ti_Data);
            break;
        case MUIA_Numeric_RevUpDown:
            Stepper_SetFlag(&data->st_Flags, STEPPERF_REVUPDOWN,
                tag->ti_Data);
            break;
        case MUIA_Numeric_Value:
            data->st_Value = (LONG)tag->ti_Data;
            value_set = TRUE;
            break;
        }
    }

    data->st_Value = Stepper_Clamp(
        value_set ? data->st_Value : data->st_Default,
        data->st_Min, data->st_Max);

    Stepper_SyncEditor(obj, data->st_Value);

    DoMethod(obj, MUIM_Notify, MUIA_String_Integer, MUIV_EveryTime,
        obj, 3, MUIM_Set, MUIA_Numeric_Value, MUIV_TriggerValue);

    return obj;
}

IPTR Stepper__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Stepper_DATA *data = INST_DATA(cl, obj);
    struct TagItem *state = msg->ops_AttrList;
    struct TagItem *tag;
    LONG old_value = data->st_Value;
    LONG old_min = data->st_Min;
    LONG old_max = data->st_Max;
    ULONG old_flags = data->st_Flags;
    BOOL value_tag_seen = FALSE;
    BOOL bounds_tag_seen = FALSE;
    BOOL reverse_tag_seen = FALSE;
    IPTR result;

    while ((tag = NextTagItem(&state)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Numeric_CheckAllSizes:
            Stepper_SetFlag(&data->st_Flags, STEPPERF_CHECKALLSIZES,
                tag->ti_Data);
            break;
        case MUIA_Numeric_Default:
            data->st_Default = (LONG)tag->ti_Data;
            break;
        case MUIA_Numeric_Format:
            data->st_Format = (STRPTR)tag->ti_Data;
            break;
        case MUIA_Numeric_Max:
            bounds_tag_seen = TRUE;
            data->st_Max = (LONG)tag->ti_Data;
            data->st_Value = Stepper_Clamp(data->st_Value,
                data->st_Min, data->st_Max);
            break;
        case MUIA_Numeric_Min:
            bounds_tag_seen = TRUE;
            data->st_Min = (LONG)tag->ti_Data;
            data->st_Value = Stepper_Clamp(data->st_Value,
                data->st_Min, data->st_Max);
            break;
        case MUIA_Numeric_Reverse:
            reverse_tag_seen = TRUE;
            Stepper_SetFlag(&data->st_Flags, STEPPERF_REVERSE,
                tag->ti_Data);
            break;
        case MUIA_Numeric_RevLeftRight:
            Stepper_SetFlag(&data->st_Flags, STEPPERF_REVLEFTRIGHT,
                tag->ti_Data);
            break;
        case MUIA_Numeric_RevUpDown:
            Stepper_SetFlag(&data->st_Flags, STEPPERF_REVUPDOWN,
                tag->ti_Data);
            break;
        case MUIA_Numeric_Value:
            value_tag_seen = TRUE;
            tag->ti_Data = Stepper_Clamp((LONG)tag->ti_Data,
                data->st_Min, data->st_Max);
            if (data->st_Value == (LONG)tag->ti_Data)
                tag->ti_Tag = TAG_IGNORE;
            else
                data->st_Value = (LONG)tag->ti_Data;
            break;
        }
    }

    result = DoSuperMethodA(cl, obj, (Msg)msg);

    if (value_tag_seen || bounds_tag_seen || data->st_Value != old_value
        || data->st_Min != old_min || data->st_Max != old_max)
        Stepper_SyncEditor(obj, data->st_Value);

    if (reverse_tag_seen
        || ((data->st_Flags ^ old_flags) & STEPPERF_REVERSE))
        MUI_Redraw(obj, MADF_DRAWUPDATE);

    return result;
}

IPTR Stepper__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Stepper_DATA *data = INST_DATA(cl, obj);
    IPTR *store = msg->opg_Storage;

    switch (msg->opg_AttrID)
    {
    case MUIA_Numeric_CheckAllSizes:
        *store = (data->st_Flags & STEPPERF_CHECKALLSIZES) != 0;
        return TRUE;
    case MUIA_Numeric_Default:
        *store = data->st_Default;
        return TRUE;
    case MUIA_Numeric_Format:
        *store = (IPTR)data->st_Format;
        return TRUE;
    case MUIA_Numeric_Max:
        *store = data->st_Max;
        return TRUE;
    case MUIA_Numeric_Min:
        *store = data->st_Min;
        return TRUE;
    case MUIA_Numeric_Reverse:
        *store = (data->st_Flags & STEPPERF_REVERSE) != 0;
        return TRUE;
    case MUIA_Numeric_RevLeftRight:
        *store = (data->st_Flags & STEPPERF_REVLEFTRIGHT) != 0;
        return TRUE;
    case MUIA_Numeric_RevUpDown:
        *store = (data->st_Flags & STEPPERF_REVUPDOWN) != 0;
        return TRUE;
    case MUIA_Numeric_Value:
        *store = data->st_Value;
        return TRUE;
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR Stepper__MUIM_Setup(struct IClass *cl, Object *obj,
    struct MUIP_Setup *msg)
{
    struct Stepper_DATA *data = INST_DATA(cl, obj);

    if (!DoSuperMethodA(cl, obj, (Msg)msg))
        return FALSE;

    data->st_Ehn.ehn_Events = IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY;
    data->st_Ehn.ehn_Priority = 1;
    data->st_Ehn.ehn_Flags = MUI_EHF_GUIMODE;
    data->st_Ehn.ehn_Object = obj;
    data->st_Ehn.ehn_Class = cl;

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->st_Ehn);
    return TRUE;
}

IPTR Stepper__MUIM_Cleanup(struct IClass *cl, Object *obj,
    struct MUIP_Cleanup *msg)
{
    struct Stepper_DATA *data = INST_DATA(cl, obj);

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->st_Ehn);
    data->st_PressedSide = STEPPER_SIDE_NONE;

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR Stepper__MUIM_Draw(struct IClass *cl, Object *obj,
    struct MUIP_Draw *msg)
{
    struct Stepper_DATA *data = INST_DATA(cl, obj);
    IPTR disabled = FALSE;
    ULONG text_pen;
    IPTR result;

    result = DoSuperMethodA(cl, obj, (Msg)msg);

    if (!(msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE)))
        return result;

    get(obj, MUIA_Disabled, &disabled);
    text_pen = _pens(obj)[disabled ? MPEN_SHADOW : MPEN_TEXT];

    Stepper_DrawButton(obj, data, STEPPER_SIDE_LEFT, text_pen);
    Stepper_DrawButton(obj, data, STEPPER_SIDE_RIGHT, text_pen);

    return result;
}

IPTR Stepper__MUIM_HandleEvent(struct IClass *cl, Object *obj,
    struct MUIP_HandleEvent *msg)
{
    struct Stepper_DATA *data = INST_DATA(cl, obj);
    IPTR disabled = FALSE;
    UBYTE side;
    UBYTE pressed;

    get(obj, MUIA_Disabled, &disabled);

    if (msg->imsg && msg->imsg->Class == IDCMP_RAWKEY
        && !disabled
        && msg->muikey == MUIKEY_NONE
        && msg->imsg->MouseX >= _left(obj)
        && msg->imsg->MouseX <= _right(obj)
        && msg->imsg->MouseY >= _top(obj)
        && msg->imsg->MouseY <= _bottom(obj))
    {
        if (msg->imsg->Code == RAWKEY_NM_WHEEL_UP)
        {
            Stepper_CommitEditor(obj);
            Stepper_Adjust(obj, data,
                (data->st_Flags & STEPPERF_REVERSE) ? 1 : -1);
            return MUI_EventHandlerRC_Eat;
        }
        if (msg->imsg->Code == RAWKEY_NM_WHEEL_DOWN)
        {
            Stepper_CommitEditor(obj);
            Stepper_Adjust(obj, data,
                (data->st_Flags & STEPPERF_REVERSE) ? -1 : 1);
            return MUI_EventHandlerRC_Eat;
        }
    }

    if (!msg->imsg || msg->imsg->Class != IDCMP_MOUSEBUTTONS)
        return 0;

    if (disabled)
    {
        if (data->st_PressedSide != STEPPER_SIDE_NONE)
        {
            data->st_PressedSide = STEPPER_SIDE_NONE;
            MUI_Redraw(obj, MADF_DRAWUPDATE);
        }
        return 0;
    }

    if (msg->imsg->Code == SELECTDOWN)
    {
        side = Stepper_SideAt(obj, msg->imsg->MouseX, msg->imsg->MouseY);
        if (side != STEPPER_SIDE_NONE)
        {
            data->st_PressedSide = side;
            MUI_Redraw(obj, MADF_DRAWUPDATE);
            return MUI_EventHandlerRC_Eat;
        }
    }
    else if (msg->imsg->Code == SELECTUP
        && data->st_PressedSide != STEPPER_SIDE_NONE)
    {
        pressed = data->st_PressedSide;
        side = Stepper_SideAt(obj, msg->imsg->MouseX, msg->imsg->MouseY);
        data->st_PressedSide = STEPPER_SIDE_NONE;

        if (side == pressed)
        {
            Stepper_CommitEditor(obj);
            Stepper_Adjust(obj, data, Stepper_ActionForSide(data, side));
        }

        MUI_Redraw(obj, MADF_DRAWUPDATE);
        return MUI_EventHandlerRC_Eat;
    }
    else if (data->st_PressedSide != STEPPER_SIDE_NONE)
    {
        data->st_PressedSide = STEPPER_SIDE_NONE;
        MUI_Redraw(obj, MADF_DRAWUPDATE);
        return MUI_EventHandlerRC_Eat;
    }

    return 0;
}

IPTR Stepper__MUIM_Numeric_Decrease(struct IClass *cl, Object *obj,
    struct MUIP_Numeric_Decrease *msg)
{
    struct Stepper_DATA *data = INST_DATA(cl, obj);
    long long next = (long long)data->st_Value - (long long)msg->amount;

    if (next < data->st_Min)
        next = data->st_Min;
    if (next > data->st_Max)
        next = data->st_Max;

    set(obj, MUIA_Numeric_Value, (LONG)next);
    return 1;
}

IPTR Stepper__MUIM_Numeric_Increase(struct IClass *cl, Object *obj,
    struct MUIP_Numeric_Increase *msg)
{
    struct Stepper_DATA *data = INST_DATA(cl, obj);
    long long next = (long long)data->st_Value + (long long)msg->amount;

    if (next < data->st_Min)
        next = data->st_Min;
    if (next > data->st_Max)
        next = data->st_Max;

    set(obj, MUIA_Numeric_Value, (LONG)next);
    return 1;
}

IPTR Stepper__MUIM_Numeric_SetDefault(struct IClass *cl, Object *obj,
    struct MUIP_Numeric_SetDefault *msg)
{
    struct Stepper_DATA *data = INST_DATA(cl, obj);

    (void)msg;
    set(obj, MUIA_Numeric_Value,
        Stepper_Clamp(data->st_Default, data->st_Min, data->st_Max));
    return 0;
}

IPTR Stepper__MUIM_Numeric_Stringify(struct IClass *cl, Object *obj,
    struct MUIP_Numeric_Stringify *msg)
{
    struct Stepper_DATA *data = INST_DATA(cl, obj);

    snprintf(data->st_Stringify, sizeof(data->st_Stringify),
        data->st_Format ? data->st_Format : (STRPTR) "%ld", (long)msg->value);
    data->st_Stringify[sizeof(data->st_Stringify) - 1] = '\0';

    return (IPTR)data->st_Stringify;
}

IPTR Stepper__MUIM_Numeric_ScaleToValue(struct IClass *cl, Object *obj,
    struct MUIP_Numeric_ScaleToValue *msg)
{
    struct Stepper_DATA *data = INST_DATA(cl, obj);
    LONG min;
    LONG max;
    LONG value;
    LONG span;

    min = (data->st_Flags & STEPPERF_REVERSE) ? data->st_Max : data->st_Min;
    max = (data->st_Flags & STEPPERF_REVERSE) ? data->st_Min : data->st_Max;

    value = Stepper_Clamp(msg->scale - msg->scalemin,
        msg->scalemin, msg->scalemax);
    span = msg->scalemax - msg->scalemin;
    value = value * (max - min);
    if (span)
        value /= span;
    value += min;

    return value;
}

static IPTR Stepper_ValueToScale(struct Stepper_DATA *data, LONG value,
    LONG scalemin, LONG scalemax)
{
    LONG min;
    LONG max;
    LONG scale;

    min = (data->st_Flags & STEPPERF_REVERSE) ? scalemax : scalemin;
    max = (data->st_Flags & STEPPERF_REVERSE) ? scalemin : scalemax;
    value = Stepper_Clamp(value, data->st_Min, data->st_Max);

    if (data->st_Max == data->st_Min)
        return min;

    scale = value - data->st_Min;
    scale *= max - min;
    scale += (data->st_Max - data->st_Min) / 2;
    scale /= data->st_Max - data->st_Min;
    scale += min;

    if (scalemin < scalemax)
        scale = Stepper_Clamp(scale, scalemin, scalemax);
    else
        scale = Stepper_Clamp(scale, scalemax, scalemin);

    return scale;
}

IPTR Stepper__MUIM_Numeric_ValueToScale(struct IClass *cl, Object *obj,
    struct MUIP_Numeric_ValueToScale *msg)
{
    struct Stepper_DATA *data = INST_DATA(cl, obj);

    return Stepper_ValueToScale(data, data->st_Value,
        msg->scalemin, msg->scalemax);
}
