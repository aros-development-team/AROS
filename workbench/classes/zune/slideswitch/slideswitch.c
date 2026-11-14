/*
    Copyright (C) 2026, Andy Rexx. All rights reserved.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <devices/inputevent.h>
#include <graphics/text.h>
#include <intuition/intuition.h>

#include <proto/alib.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include "slideswitch.h"
#include "slideswitch_private.h"

//#define DEBUG 1
#include <aros/debug.h>

#define SLIDESWITCH_MIN_CELL_SIZE 14
#define SLIDESWITCH_CELL_PADDING   6

/****** SlideSwitch.mcc/MUIC_SlideSwitch ***********************************
*
*   NAME
*       MUIC_SlideSwitch
*
*   SUPERCLASS
*       Numeric.mui
*
*   DESCRIPTION
*       Display a binary Numeric value as a two-position slide switch. A value
*       of zero selects Off; a value of one selects On.
*
*       The selected half contains the thumb. Off is marked with "O". On is
*       marked with "I" and uses MPEN_FILL. MUIA_Numeric_Reverse exchanges
*       the visual positions of the two states without changing their values.
*
*   NOTES
*       Use MUIA_Numeric_Value to read or change the state. The public values
*       MUIV_SlideSwitch_Off and MUIV_SlideSwitch_On are 0 and 1,
*       respectively. Values assigned to MUIA_Numeric_Value or
*       MUIA_Numeric_Default are normalized to Off for zero and On for any
*       nonzero value. Use ordinary MUIA_Numeric_Value notifications to observe
*       state changes.
*
*       SlideSwitch adds no class-specific attributes or methods. The class
*       fixes MUIA_Numeric_Min and MUIA_Numeric_Max at 0 and 1. Attempts to
*       change either attribute are ignored.
*
*       Click either half to select its state. During a drag, the state changes
*       when the pointer crosses the midpoint. Shift-click does not start a
*       drag. Numeric.mui continues to handle raw-key and mouse-wheel input.
*
*       The constructor supplies MUII_SliderBack, MUIV_Font_Knob and
*       MUIV_Frame_Slider as the default background, font and frame.
*
*/

/* Return one cell size: at least 14 pixels, or the current font height
 * plus six pixels of padding when that is larger. */
static LONG SlideSwitch_CellSize(Object *obj)
{
    LONG size = SLIDESWITCH_MIN_CELL_SIZE;

    if (_font(obj))
    {
        LONG font_size = _font(obj)->tf_YSize + SLIDESWITCH_CELL_PADDING;
        if (font_size > size)
            size = font_size;
    }

    return size;
}

/* Re-register the instance event handler after enabling or disabling
 * IDCMP_MOUSEMOVE. Preserve the mouse-button request and handler identity. */
static void SlideSwitch_UpdateMouseMove(Object *obj,
    struct SlideSwitch_DATA *data, BOOL enable)
{
    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ss_Ehn);

    if (enable)
        data->ss_Ehn.ehn_Events |= IDCMP_MOUSEMOVE;
    else
        data->ss_Ehn.ehn_Events &= ~IDCMP_MOUSEMOVE;

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ss_Ehn);
}

/* Map a window-relative x coordinate to Off or On at the object midpoint.
 * MUIA_Numeric_Reverse exchanges the two values before they are stored. */
static void SlideSwitch_SetFromMouse(Object *obj, LONG mouse_x)
{
    IPTR reverse = FALSE;
    LONG value;
    LONG midpoint = _mleft(obj) + (_mwidth(obj) / 2);

    value = (mouse_x >= midpoint) ? MUIV_SlideSwitch_On : MUIV_SlideSwitch_Off;

    get(obj, MUIA_Numeric_Reverse, &reverse);
    if (reverse)
        value = !value;

    set(obj, MUIA_Numeric_Value, value);
}

/* Draw the thumb relief with MUI shine and shadow pens. Exchange the pens
 * while dragging to show the pressed state. */
static void SlideSwitch_DrawThumbFrame(Object *obj, LONG left, LONG top,
    LONG right, LONG bottom, BOOL pressed)
{
    ULONG shine = _pens(obj)[pressed ? MPEN_SHADOW : MPEN_SHINE];
    ULONG shadow = _pens(obj)[pressed ? MPEN_SHINE : MPEN_SHADOW];
    struct RastPort *rp = _rp(obj);

    SetAPen(rp, shine);
    Move(rp, left, bottom);
    Draw(rp, left, top);
    Draw(rp, right, top);

    SetAPen(rp, shadow);
    Draw(rp, right, bottom);
    Draw(rp, left, bottom);
}

/* Construct Numeric.mui with a fixed 0..1 range. Ignore incoming Min/Max,
 * normalize Default/Value to binary, supply slider-style defaults, and pass
 * the remaining writable tag list to the superclass. */
Object *SlideSwitch__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct TagItem *state = msg->ops_AttrList;
    struct TagItem *tag;

    /* The class owns a fixed binary Numeric range. Numeric requires writable
     * tags for its own setter semantics, so normalize/ignore the inherited
     * binary-state tags before construction reaches Numeric. */
    while ((tag = NextTagItem(&state)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Numeric_Min:
        case MUIA_Numeric_Max:
            tag->ti_Tag = TAG_IGNORE;
            break;

        case MUIA_Numeric_Default:
        case MUIA_Numeric_Value:
            tag->ti_Data = tag->ti_Data ? MUIV_SlideSwitch_On
                                        : MUIV_SlideSwitch_Off;
            break;
        }
    }

    obj = (Object *)DoSuperNewTags
    (
        cl, obj, NULL,
        MUIA_Numeric_Min,     MUIV_SlideSwitch_Off,
        MUIA_Numeric_Max,     MUIV_SlideSwitch_On,
        MUIA_Numeric_Default, MUIV_SlideSwitch_Off,
        MUIA_Background,      MUII_SliderBack,
        MUIA_Font,            MUIV_Font_Knob,
        MUIA_Frame,           MUIV_Frame_Slider,
        TAG_MORE,             (IPTR)msg->ops_AttrList
    );

    if (obj)
    {
        struct SlideSwitch_DATA *data = INST_DATA(cl, obj);
        data->ss_Dragging = FALSE;
    }

    return obj;
}

/* Keep the Numeric range binary on OM_SET. Ignore Min/Max, normalize
 * Default/Value, and redraw after Reverse changes because Numeric.mui does
 * not redraw for that attribute itself. Return the superclass result. */
IPTR SlideSwitch__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct TagItem *state = msg->ops_AttrList;
    struct TagItem *tag;
    BOOL redraw = FALSE;
    IPTR result;

    while ((tag = NextTagItem(&state)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Numeric_Min:
        case MUIA_Numeric_Max:
            tag->ti_Tag = TAG_IGNORE;
            break;

        case MUIA_Numeric_Default:
        case MUIA_Numeric_Value:
            tag->ti_Data = tag->ti_Data ? MUIV_SlideSwitch_On
                                        : MUIV_SlideSwitch_Off;
            break;

        case MUIA_Numeric_Reverse:
            redraw = TRUE;
            break;
        }
    }

    result = DoSuperMethodA(cl, obj, (Msg)msg);

    /* Numeric does not redraw when Reverse changes, but Reverse changes the
     * visual side that represents each binary value. */
    if (redraw)
        MUI_Redraw(obj, MADF_DRAWUPDATE);

    return result;
}

/* Set up Numeric.mui, then register this instance for mouse-button events.
 * Return FALSE without registering the handler if superclass setup fails. */
IPTR SlideSwitch__MUIM_Setup(struct IClass *cl, Object *obj,
    struct MUIP_Setup *msg)
{
    struct SlideSwitch_DATA *data = INST_DATA(cl, obj);

    if (!DoSuperMethodA(cl, obj, (Msg)msg))
        return FALSE;

    data->ss_Ehn.ehn_Events = IDCMP_MOUSEBUTTONS;
    data->ss_Ehn.ehn_Priority = 0;
    data->ss_Ehn.ehn_Flags = MUI_EHF_GUIMODE;
    data->ss_Ehn.ehn_Object = obj;
    data->ss_Ehn.ehn_Class = cl;
    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ss_Ehn);

    return TRUE;
}

/* Remove the instance event handler, clear drag state, and then forward
 * MUIM_Cleanup to Numeric.mui. */
IPTR SlideSwitch__MUIM_Cleanup(struct IClass *cl, Object *obj,
    struct MUIP_Cleanup *msg)
{
    struct SlideSwitch_DATA *data = INST_DATA(cl, obj);

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ss_Ehn);
    data->ss_Dragging = FALSE;

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/* Extend Numeric.mui's minimum, default and maximum bounds by two switch
 * cell widths and one cell height. */
IPTR SlideSwitch__MUIM_AskMinMax(struct IClass *cl, Object *obj,
    struct MUIP_AskMinMax *msg)
{
    LONG cell;

    DoSuperMethodA(cl, obj, (Msg)msg);

    cell = SlideSwitch_CellSize(obj);

    msg->MinMaxInfo->MinWidth += cell * 2;
    msg->MinMaxInfo->DefWidth += cell * 2;
    msg->MinMaxInfo->MaxWidth += cell * 2;

    msg->MinMaxInfo->MinHeight += cell;
    msg->MinMaxInfo->DefHeight += cell;
    msg->MinMaxInfo->MaxHeight += cell;

    return TRUE;
}

/* Draw Numeric.mui first, then render the two-cell switch for DRAWOBJECT or
 * DRAWUPDATE. ValueToScale selects the visual side, so Reverse is inherited;
 * On uses MPEN_FILL and "I", while Off uses MPEN_BACKGROUND and "O". */
IPTR SlideSwitch__MUIM_Draw(struct IClass *cl, Object *obj,
    struct MUIP_Draw *msg)
{
    struct SlideSwitch_DATA *data = INST_DATA(cl, obj);
    struct RastPort *rp;
    IPTR value = MUIV_SlideSwitch_Off;
    LONG left, top, right, bottom, midpoint;
    LONG thumb_left, thumb_right;
    LONG text_width, text_x, text_y;
    BOOL right_side;
    char label;

    DoSuperMethodA(cl, obj, (Msg)msg);

    if (!(msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE)))
        return FALSE;

    left = _mleft(obj);
    top = _mtop(obj);
    right = _mright(obj);
    bottom = _mbottom(obj);
    midpoint = left + (_mwidth(obj) / 2);
    rp = _rp(obj);

    get(obj, MUIA_Numeric_Value, &value);
    right_side = DoMethod(obj, MUIM_Numeric_ValueToScale, 0, 1) != 0;

    SetAPen(rp, _pens(obj)[MPEN_BACKGROUND]);
    RectFill(rp, left, top, right, bottom);

    SetAPen(rp, _pens(obj)[MPEN_SHADOW]);
    Move(rp, midpoint, top);
    Draw(rp, midpoint, bottom);

    if (right_side)
    {
        thumb_left = midpoint + 1;
        thumb_right = right;
    }
    else
    {
        thumb_left = left;
        thumb_right = midpoint - 1;
    }

    if (thumb_right < thumb_left)
        thumb_right = thumb_left;

    if ((LONG)value == MUIV_SlideSwitch_On)
        SetAPen(rp, _pens(obj)[MPEN_FILL]);
    else
        SetAPen(rp, _pens(obj)[MPEN_BACKGROUND]);

    RectFill(rp, thumb_left, top, thumb_right, bottom);
    SlideSwitch_DrawThumbFrame(obj, thumb_left, top, thumb_right, bottom,
        data->ss_Dragging);

    label = ((LONG)value == MUIV_SlideSwitch_On) ? 'I' : 'O';
    SetFont(rp, _font(obj));
    SetABPenDrMd(rp, _pens(obj)[MPEN_TEXT], _pens(obj)[MPEN_BACKGROUND], JAM1);

    text_width = TextLength(rp, &label, 1);
    text_x = thumb_left + ((thumb_right - thumb_left + 1 - text_width) / 2);
    text_y = top + ((_mheight(obj) - _font(obj)->tf_YSize) / 2)
        + _font(obj)->tf_Baseline;

    Move(rp, text_x, text_y);
    Text(rp, &label, 1);

    return TRUE;
}

/* Delegate non-mouse input to Numeric.mui. An unshifted SELECTDOWN inside
 * the object starts a drag; mouse movement and SELECTUP select by x position.
 * Handled drag events are eaten. Other mouse events return zero. */
IPTR SlideSwitch__MUIM_HandleEvent(struct IClass *cl, Object *obj,
    struct MUIP_HandleEvent *msg)
{
    struct SlideSwitch_DATA *data = INST_DATA(cl, obj);

    if (!msg->imsg || (msg->imsg->Class != IDCMP_MOUSEBUTTONS
        && msg->imsg->Class != IDCMP_MOUSEMOVE))
        return DoSuperMethodA(cl, obj, (Msg)msg);

    if (msg->imsg->Class == IDCMP_MOUSEBUTTONS)
    {
        if (msg->imsg->Code == SELECTDOWN)
        {
            if (msg->imsg->MouseX >= _mleft(obj)
                && msg->imsg->MouseX <= _mright(obj)
                && msg->imsg->MouseY >= _mtop(obj)
                && msg->imsg->MouseY <= _mbottom(obj)
                && (msg->imsg->Qualifier
                    & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)) == 0)
            {
                data->ss_Dragging = TRUE;
                SlideSwitch_UpdateMouseMove(obj, data, TRUE);
                SlideSwitch_SetFromMouse(obj, msg->imsg->MouseX);
                MUI_Redraw(obj, MADF_DRAWUPDATE);
                return MUI_EventHandlerRC_Eat;
            }
        }
        else if (msg->imsg->Code == SELECTUP && data->ss_Dragging)
        {
            SlideSwitch_SetFromMouse(obj, msg->imsg->MouseX);
            SlideSwitch_UpdateMouseMove(obj, data, FALSE);
            data->ss_Dragging = FALSE;
            MUI_Redraw(obj, MADF_DRAWUPDATE);
            return MUI_EventHandlerRC_Eat;
        }
    }
    else if (data->ss_Dragging)
    {
        SlideSwitch_SetFromMouse(obj, msg->imsg->MouseX);
        return MUI_EventHandlerRC_Eat;
    }

    return 0;
}
