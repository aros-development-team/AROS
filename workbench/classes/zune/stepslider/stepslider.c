/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <devices/inputevent.h>
#include <graphics/rastport.h>
#include <intuition/intuition.h>

#include <proto/alib.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include "stepslider.h"
#include "stepslider_private.h"

/* Ticks closer than this many pixels are not drawn, only the two ends */
#define STEPSLIDER_MIN_TICK_GAP 3
/* The minimum width grows with the number of steps up to this many */
#define STEPSLIDER_MAX_SIZED_STEPS 16

/****** Stepslider.mcc/MUIC_Stepslider **************************************
*
*   NAME
*       MUIC_Stepslider
*
*   SUPERCLASS
*       Numeric.mui
*
*   DESCRIPTION
*       A horizontal slider over a few ordered steps, such as the compression
*       levels of an archiver. A tick marks every step. The knob only rests on
*       steps: a click or a drag moves it to the step nearest to the pointer.
*
*       The steps are the values from MUIA_Numeric_Min to MUIA_Numeric_Max,
*       so the mouse, the cursor keys and the wheel all choose from the same
*       values. Keys, wheel, range, default value and notifications are
*       Numeric's own.
*
*   NOTES
*       Stepslider adds no class-specific public attributes or methods. Listen
*       for MUIA_Numeric_Value. The slider shows no text: the program shows
*       the name of the chosen step beside it.
*
*       The groove is filled from the Min end of the scale to the knob.
*       MUIA_Numeric_Reverse puts Min on the right. Sizes follow the object's
*       font, and the colours are the pens of the user's preferences.
*
*       Ticks are drawn while they are at least three pixels apart; in a
*       longer range only the two ends are marked.
*
*/

/* Sizes from the font, so the slider grows with it */
static LONG Stepslider_KnobWidth(Object *obj)
{
    return _font(obj)->tf_YSize / 2 + 4;
}

static LONG Stepslider_KnobHeight(Object *obj)
{
    return _font(obj)->tf_YSize + 2;
}

static LONG Stepslider_TickHeight(Object *obj)
{
    return _font(obj)->tf_YSize / 4 + 2;
}

/* Number of steps minus one, never negative */
static LONG Stepslider_Range(Object *obj)
{
    LONG min = (LONG)XGET(obj, MUIA_Numeric_Min);
    LONG max = (LONG)XGET(obj, MUIA_Numeric_Max);

    return max > min ? max - min : 0;
}

/* Position of the current value on the scale, 0 at the left end */
static LONG Stepslider_Position(Object *obj)
{
    LONG range = Stepslider_Range(obj);
    LONG pos = (LONG)XGET(obj, MUIA_Numeric_Value)
        - (LONG)XGET(obj, MUIA_Numeric_Min);

    if (pos < 0)
        pos = 0;
    if (pos > range)
        pos = range;
    if (XGET(obj, MUIA_Numeric_Reverse))
        pos = range - pos;
    return pos;
}

/* Left edge of the knob at a scale position */
static LONG Stepslider_KnobLeft(Object *obj, LONG pos)
{
    LONG range = Stepslider_Range(obj);
    LONG span = _mwidth(obj) - Stepslider_KnobWidth(obj);

    if (range == 0 || span <= 0)
        return _mleft(obj);
    return _mleft(obj) + (LONG)((QUAD)span * pos / range);
}

/* The value of the step nearest to a pointer position */
static LONG Stepslider_ValueAt(Object *obj, LONG x)
{
    LONG range = Stepslider_Range(obj);
    LONG span = _mwidth(obj) - Stepslider_KnobWidth(obj);
    LONG min = (LONG)XGET(obj, MUIA_Numeric_Min);
    LONG pos;

    if (range == 0 || span <= 0)
        return min;

    x -= _mleft(obj) + Stepslider_KnobWidth(obj) / 2;
    if (x < 0)
        x = 0;
    if (x > span)
        x = span;
    pos = (LONG)((2 * (QUAD)x * range + span) / (2 * (QUAD)span));
    if (XGET(obj, MUIA_Numeric_Reverse))
        pos = range - pos;
    return min + pos;
}

static void Stepslider_Bevel(struct RastPort *rp, LONG left, LONG top,
    LONG right, LONG bottom, ULONG lefttop, ULONG rightbottom)
{
    SetAPen(rp, lefttop);
    Move(rp, left, bottom);
    Draw(rp, left, top);
    Draw(rp, right, top);
    SetAPen(rp, rightbottom);
    Draw(rp, right, bottom);
    Draw(rp, left + 1, bottom);
}

static void Stepslider_Tick(Object *obj, LONG pos)
{
    struct RastPort *rp = _rp(obj);
    LONG x = Stepslider_KnobLeft(obj, pos) + Stepslider_KnobWidth(obj) / 2;
    LONG top = _mtop(obj) + Stepslider_KnobHeight(obj) + 1;

    Move(rp, x, top);
    Draw(rp, x, top + Stepslider_TickHeight(obj) - 1);
}

static void Stepslider_Render(Object *obj, struct Stepslider_DATA *data)
{
    struct RastPort *rp = _rp(obj);
    LONG kw = Stepslider_KnobWidth(obj);
    LONG kh = Stepslider_KnobHeight(obj);
    LONG top = _mtop(obj);
    LONG cy = top + kh / 2;
    LONG gl = _mleft(obj) + kw / 2;
    LONG gr = _mright(obj) - kw / 2;
    LONG range = Stepslider_Range(obj);
    LONG span = _mwidth(obj) - kw;
    LONG kx = Stepslider_KnobLeft(obj, Stepslider_Position(obj));
    LONG i;

    /* Groove: recessed, filled like a gauge from the Min end to the knob */
    Stepslider_Bevel(rp, gl, cy - 2, gr, cy + 2, _pens(obj)[MPEN_SHADOW],
        _pens(obj)[MPEN_SHINE]);
    SetAPen(rp, _pens(obj)[MPEN_HALFSHADOW]);
    RectFill(rp, gl + 1, cy - 1, gr - 1, cy + 1);
    SetAPen(rp, _pens(obj)[MPEN_FILL]);
    if (XGET(obj, MUIA_Numeric_Reverse))
    {
        if (kx + kw / 2 < gr - 1)
            RectFill(rp, kx + kw / 2, cy - 1, gr - 1, cy + 1);
    }
    else if (kx + kw / 2 > gl + 1)
    {
        RectFill(rp, gl + 1, cy - 1, kx + kw / 2, cy + 1);
    }

    /* A tick under every step, or only under the ends of a long range */
    SetAPen(rp, _pens(obj)[MPEN_SHADOW]);
    if (range > 0 && span / range >= STEPSLIDER_MIN_TICK_GAP)
    {
        for (i = 0; i <= range; i++)
            Stepslider_Tick(obj, i);
    }
    else
    {
        Stepslider_Tick(obj, 0);
        if (range > 0)
            Stepslider_Tick(obj, range);
    }

    /* Knob: raised, sunken while it is held, with a grip line */
    SetAPen(rp, _pens(obj)[data->ss_Dragging ? MPEN_HALFSHADOW
        : MPEN_HALFSHINE]);
    RectFill(rp, kx + 1, top + 1, kx + kw - 2, top + kh - 2);
    Stepslider_Bevel(rp, kx, top, kx + kw - 1, top + kh - 1,
        _pens(obj)[data->ss_Dragging ? MPEN_SHADOW : MPEN_SHINE],
        _pens(obj)[data->ss_Dragging ? MPEN_SHINE : MPEN_SHADOW]);
    SetAPen(rp, _pens(obj)[MPEN_SHADOW]);
    Move(rp, kx + kw / 2, top + 3);
    Draw(rp, kx + kw / 2, top + kh - 4);
}

/*
 * A drag ends when the button is released. A hidden or disabled object gets
 * no input, so that release can be missed: the drag also ends when the
 * object is hidden or disabled, and on a move without the button held.
 */
static void Stepslider_EndDrag(Object *obj, struct Stepslider_DATA *data,
    BOOL redraw)
{
    if (!data->ss_Dragging)
        return;

    data->ss_Dragging = FALSE;
    if (redraw)
        MUI_Redraw(obj, MADF_DRAWUPDATE);
}

IPTR Stepslider__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    return DoSuperNewTags(cl, obj, NULL,
        MUIA_FillArea, TRUE,
        TAG_MORE, (IPTR)msg->ops_AttrList);
}

IPTR Stepslider__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Stepslider_DATA *data = INST_DATA(cl, obj);
    IPTR retval = DoSuperMethodA(cl, obj, (Msg)msg);

    if (GetTagData(MUIA_Disabled, FALSE, msg->ops_AttrList))
        Stepslider_EndDrag(obj, data, FALSE);

    return retval;
}

IPTR Stepslider__MUIM_Setup(struct IClass *cl, Object *obj,
    struct MUIP_Setup *msg)
{
    struct Stepslider_DATA *data = INST_DATA(cl, obj);

    if (!DoSuperMethodA(cl, obj, (Msg)msg))
        return FALSE;

    data->ss_Ehn.ehn_Events = IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE;
    data->ss_Ehn.ehn_Priority = 1;
    data->ss_Ehn.ehn_Flags = MUI_EHF_GUIMODE;
    data->ss_Ehn.ehn_Object = obj;
    data->ss_Ehn.ehn_Class = cl;
    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ss_Ehn);
    data->ss_HandlerAdded = TRUE;

    return TRUE;
}

IPTR Stepslider__MUIM_Cleanup(struct IClass *cl, Object *obj,
    struct MUIP_Cleanup *msg)
{
    struct Stepslider_DATA *data = INST_DATA(cl, obj);

    if (data->ss_HandlerAdded)
    {
        DoMethod(_win(obj), MUIM_Window_RemEventHandler,
            (IPTR)&data->ss_Ehn);
        data->ss_HandlerAdded = FALSE;
    }
    data->ss_Dragging = FALSE;

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR Stepslider__MUIM_Hide(struct IClass *cl, Object *obj,
    struct MUIP_Hide *msg)
{
    Stepslider_EndDrag(obj, INST_DATA(cl, obj), FALSE);

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR Stepslider__MUIM_AskMinMax(struct IClass *cl, Object *obj,
    struct MUIP_AskMinMax *msg)
{
    IPTR retval = DoSuperMethodA(cl, obj, (Msg)msg);
    LONG fonth = _font(obj)->tf_YSize;
    LONG height = Stepslider_KnobHeight(obj) + 1 + Stepslider_TickHeight(obj);
    LONG steps = Stepslider_Range(obj) + 1;
    LONG minwidth;
    LONG defwidth;

    if (steps > STEPSLIDER_MAX_SIZED_STEPS)
        steps = STEPSLIDER_MAX_SIZED_STEPS;
    minwidth = steps * (Stepslider_KnobWidth(obj) + 4);
    if (minwidth < fonth * 4)
        minwidth = fonth * 4;
    defwidth = fonth * 12;
    if (defwidth < minwidth)
        defwidth = minwidth;

    msg->MinMaxInfo->MinWidth += minwidth;
    msg->MinMaxInfo->DefWidth += defwidth;
    msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;
    msg->MinMaxInfo->MinHeight += height;
    msg->MinMaxInfo->DefHeight += height;
    msg->MinMaxInfo->MaxHeight += height;

    return retval;
}

IPTR Stepslider__MUIM_Draw(struct IClass *cl, Object *obj,
    struct MUIP_Draw *msg)
{
    IPTR retval = DoSuperMethodA(cl, obj, (Msg)msg);

    if (!(msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE)))
        return retval;

    /* An update clears the old knob; a full draw has the background */
    if (msg->flags & MADF_DRAWUPDATE)
    {
        DoMethod(obj, MUIM_DrawBackground, _mleft(obj), _mtop(obj),
            _mwidth(obj), _mheight(obj), _mleft(obj), _mtop(obj), 0);
    }
    Stepslider_Render(obj, INST_DATA(cl, obj));

    return retval;
}

/*
 * Only this class's mouse handler arrives here: Numeric registers its key
 * handler with its own class, so keys and the wheel never reach this method.
 */
IPTR Stepslider__MUIM_HandleEvent(struct IClass *cl, Object *obj,
    struct MUIP_HandleEvent *msg)
{
    struct Stepslider_DATA *data = INST_DATA(cl, obj);
    LONG x, y, value;

    if (!msg->imsg)
        return 0;

    x = msg->imsg->MouseX;
    y = msg->imsg->MouseY;

    if (msg->imsg->Class == IDCMP_MOUSEMOVE)
    {
        if (!data->ss_Dragging)
            return 0;

        if (!(msg->imsg->Qualifier & IEQUALIFIER_LEFTBUTTON))
        {
            Stepslider_EndDrag(obj, data, TRUE);
            return 0;
        }

        value = Stepslider_ValueAt(obj, x);
        if (value != (LONG)XGET(obj, MUIA_Numeric_Value))
            set(obj, MUIA_Numeric_Value, value);
        return MUI_EventHandlerRC_Eat;
    }

    if (msg->imsg->Class != IDCMP_MOUSEBUTTONS)
        return 0;

    if (msg->imsg->Code == SELECTDOWN && !XGET(obj, MUIA_Disabled)
        && x >= _mleft(obj) && x <= _mright(obj)
        && y >= _mtop(obj) && y <= _mbottom(obj))
    {
        data->ss_Dragging = TRUE;
        value = Stepslider_ValueAt(obj, x);
        if (value != (LONG)XGET(obj, MUIA_Numeric_Value))
            set(obj, MUIA_Numeric_Value, value);
        else
            MUI_Redraw(obj, MADF_DRAWUPDATE);
        return MUI_EventHandlerRC_Eat;
    }

    if (msg->imsg->Code == SELECTUP && data->ss_Dragging)
    {
        Stepslider_EndDrag(obj, data, TRUE);
        return MUI_EventHandlerRC_Eat;
    }

    return 0;
}
