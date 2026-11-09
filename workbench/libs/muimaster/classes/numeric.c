/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#include <devices/rawkeycodes.h>
#include <stdio.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/muimaster.h>

#include "debug.h"
#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

struct MUI_NumericData
{
    STRPTR format;
    LONG defvalue;
    LONG max;
    LONG min;
    LONG value;
    ULONG flags;
    struct MUI_EventHandlerNode ehn;
    char buf[50];
};

enum numeric_flags
{
    NUMERIC_REVERSE = (1 << 0),
    NUMERIC_REVLEFTRIGHT = (1 << 1),
    NUMERIC_REVUPDOWN = (1 << 2),
    NUMERIC_CHECKALLSIZES = (1 << 3),
};

/****** Numeric.mui/MUIC_Numeric ************
*
*   NAME
*       MUIC_Numeric
*
*   SUPERCLASS
*       Area.mui
*
*   DESCRIPTION
*       Maintain a signed LONG value, bounds, default and formatting policy for
*       numeric subclasses. Defaults are Min zero, Max 100, Default zero, Format
*       %ld and all flags FALSE. Construction uses the explicit Value or
*       Default, clamped after all initialization tags. Keep Min no greater than
*       Max and all arithmetic representable; inverted bounds and signed
*       overflow are not rejected.
*       Format is borrowed; keep it valid until replacement or disposal.
*       Stringify returns a per-instance buffer overwritten by the next call.
*       Numeric owns no separately allocated formatter string.
*
*   METHODS
*       Setup calls Area then registers a raw-key handler; Cleanup removes it
*       before Area Cleanup. Keyboard arrows change by one, page/word keys by
*       ten (one when the range span is below ten), endpoint keys select bounds,
*       Toggle selects Default, and Press/Release are consumed. Reverse and axis
*       reversal flags affect direction. WordLeft/WordRight use the vertical
*       reversal branch in this implementation. Wheel up/down inside the object
*       change by one and consult only Reverse. HandleEvent does not delegate to
*       Area; when muikey is NONE it dereferences imsg without a NULL check.
*       Export stores one LONG under nonzero ObjectID; Import reads a matching
*       LONG without checking stored length and invokes the ordinary Value
*       setter. Both return zero, ignore delegated results and do not call Area
*       serialization.
*
*   NOTES
*       The setter processes tags in order. Each bound change immediately clamps
*       Value; a later bound can therefore change the effect of earlier Value
*       tags. Value tags are rewritten to the clamped value, or TAG_IGNORE if
*       unchanged. A value change caused only by Min/Max does not synthesize a
*       Value notification.
*       Changed Min, Max or Format pointer requests parent Group
*       InitChange/ExitChange before superclass setting, then redraws. Mutating
*       a format string in place does not change that pointer. CheckAllSizes and
*       reversal changes alone do not request redraw or parent relayout. Numeric
*       stores CheckAllSizes but does not calculate rendered sizes itself.
*
*/
/****** Numeric.mui/MUIA_Numeric_Value ************
*
*   NAME
*       MUIA_Numeric_Value
*
*   FUNCTION
*       Store the current value, clamped to the current Min/Max.
*
*   INPUTS
*       LONG; initialize, set and get.
*
*   NOTES
*       Construction defaults to Default. Runtime setter rewrites the tag to its
*       clamp result, or TAG_IGNORE when unchanged. Tag storage must be
*       writable. Bound changes clamp internally without synthesizing a Value
*       tag. Getter returns stored state; arithmetic and inverted bounds are not
*       validated.
*
*/
/****** Numeric.mui/MUIA_Numeric_Min ************
*
*   NAME
*       MUIA_Numeric_Min
*
*   FUNCTION
*       Store the lower bound. Default zero.
*
*   INPUTS
*       LONG; initialize, set and get.
*
*   NOTES
*       Keep Min <= Max. Construction clamps Value after parsing all tags;
*       runtime setting clamps immediately at this tag. Changed bounds request
*       parent relayout and redraw, but do not directly invoke a subclass size
*       calculator. Getter returns stored bound; no range-error status exists.
*
*/
/****** Numeric.mui/MUIA_Numeric_Max ************
*
*   NAME
*       MUIA_Numeric_Max
*
*   FUNCTION
*       Store the upper bound. Default 100.
*
*   INPUTS
*       LONG; initialize, set and get.
*
*   NOTES
*       Keep Min <= Max. Construction clamps Value after parsing all tags;
*       runtime setting clamps immediately at this tag. Changed bounds request
*       parent relayout and redraw, but do not directly invoke a subclass size
*       calculator. Getter returns stored bound; no range-error status exists.
*
*/
/****** Numeric.mui/MUIA_Numeric_Default ************
*
*   NAME
*       MUIA_Numeric_Default
*
*   FUNCTION
*       Store the value used by SetDefault and by construction without explicit
*       Value. Default zero.
*
*   INPUTS
*       LONG; initialize, set and get.
*
*   NOTES
*       Stored without clamping; SetDefault clamps when applying it. Setting
*       Default alone does not change Value. Getter can therefore return a value
*       outside current bounds.
*
*/
/****** Numeric.mui/MUIA_Numeric_Format ************
*
*   NAME
*       MUIA_Numeric_Format
*
*   FUNCTION
*       Choose the snprintf format used by Stringify. Default %ld.
*
*   INPUTS
*       Borrowed string pointer; initialize, set and get.
*
*   NOTES
*       Supply a non-NULL valid format accepting exactly the supplied long
*       argument, such as %ld. The string is not copied or freed. A changed
*       pointer requests parent relayout and redraw; in-place edits do not.
*       Getter returns the borrowed pointer. Formatting is bounded, but
*       incompatible conversions or missing arguments are not validated.
*
*/
/****** Numeric.mui/MUIA_Numeric_CheckAllSizes ************
*
*   NAME
*       MUIA_Numeric_CheckAllSizes
*
*   FUNCTION
*       Store a size-checking policy flag. Default FALSE.
*
*   INPUTS
*       BOOL; initialize, set and get.
*
*   NOTES
*       Getter normalizes to TRUE/FALSE. Numeric itself does not consume the
*       flag for size calculation. The AROS Slider implementation scans its
*       selected range regardless of this flag; changing it alone does not
*       request relayout.
*
*/
/****** Numeric.mui/MUIA_Numeric_Reverse ************
*
*   NAME
*       MUIA_Numeric_Reverse
*
*   FUNCTION
*       Reverse scale mapping and Numeric input direction. Default FALSE.
*
*   INPUTS
*       BOOL; initialize, set and get.
*
*   NOTES
*       Getter normalizes the flag. Direct Increase/Decrease retain their
*       arithmetic meaning. Reversal alone does not redraw or invalidate Slider
*       cached position; arrange a subsequent valid redraw/recalculation when
*       changing it live.
*
*/
/****** Numeric.mui/MUIA_Numeric_RevLeftRight ************
*
*   NAME
*       MUIA_Numeric_RevLeftRight
*
*   FUNCTION
*       Reverse the horizontal branch of Numeric key handling. Default FALSE.
*
*   INPUTS
*       BOOL; initialize, set and get.
*
*   NOTES
*       Combines with Reverse, so two reversals cancel. Left/Right and endpoint
*       keys use the horizontal branch; Up/Down, page and word keys use the
*       vertical branch. Wheel handling ignores these axis flags. Scale methods
*       also ignore them. Getter returns normalized state; setting alone does
*       not redraw.
*
*/
/****** Numeric.mui/MUIA_Numeric_RevUpDown ************
*
*   NAME
*       MUIA_Numeric_RevUpDown
*
*   FUNCTION
*       Reverse the vertical branch of Numeric key handling. Default FALSE.
*
*   INPUTS
*       BOOL; initialize, set and get.
*
*   NOTES
*       Combines with Reverse, so two reversals cancel. Left/Right and endpoint
*       keys use the horizontal branch; Up/Down, page and word keys use the
*       vertical branch. Wheel handling ignores these axis flags. Scale methods
*       also ignore them. Getter returns normalized state; setting alone does
*       not redraw.
*
*/
/****** Numeric.mui/MUIM_Numeric_Increase ************
*
*   NAME
*       MUIM_Numeric_Increase
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Numeric_Increase, amount);
*
*   FUNCTION
*       Compute Value + amount, clamp to Min/Max and set Value if changed.
*
*   INPUTS
*       amount - LONG; signed arithmetic must remain representable.
*
*   RESULT
*       Always one; setter status is ignored.
*
*   NOTES
*       Negative amounts reverse the arithmetic direction. Reverse flags do not
*       alter this method. Ordinary Value notifications remain enabled. Overflow
*       happens before clamping and is not prevented by it.
*
*/
/****** Numeric.mui/MUIM_Numeric_Decrease ************
*
*   NAME
*       MUIM_Numeric_Decrease
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Numeric_Decrease, amount);
*
*   FUNCTION
*       Compute Value - amount, clamp to Min/Max and set Value if changed.
*
*   INPUTS
*       amount - LONG; signed arithmetic must remain representable.
*
*   RESULT
*       Always one; setter status is ignored.
*
*   NOTES
*       Negative amounts reverse the arithmetic direction. Reverse flags do not
*       alter this method. Ordinary Value notifications remain enabled. Overflow
*       happens before clamping and is not prevented by it.
*
*/
/****** Numeric.mui/MUIM_Numeric_SetDefault ************
*
*   NAME
*       MUIM_Numeric_SetDefault
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Numeric_SetDefault);
*
*   FUNCTION
*       Apply Default through the ordinary Value setter after clamping to
*       Min/Max.
*
*   INPUTS
*       No arguments beyond MethodID. Valid ordered bounds required.
*
*   RESULT
*       Always zero; setter result is ignored.
*
*   NOTES
*       Notifications follow Value setter behavior, including suppression when
*       unchanged. Default itself remains unclamped.
*
*/
/****** Numeric.mui/MUIM_Numeric_Stringify ************
*
*   NAME
*       MUIM_Numeric_Stringify
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Numeric_Stringify, value);
*
*   FUNCTION
*       Format the supplied value using Format into a per-instance character
*       buffer.
*
*   INPUTS
*       value - LONG; not clamped to Numeric bounds. Format must accept the
*       single long argument.
*
*   RESULT
*       Borrowed string pointer, returned as IPTR.
*
*   NOTES
*       snprintf receives size 49, so at most 48 output characters plus
*       terminator are produced; byte 49 is also cleared. Formatting status is
*       ignored. Copy before another Stringify on the same object, and before
*       disposal. Do not free the returned pointer. Subclasses may override this
*       conversion.
*
*/
/****** Numeric.mui/MUIM_Numeric_ScaleToValue ************
*
*   NAME
*       MUIM_Numeric_ScaleToValue
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Numeric_ScaleToValue, scalemin,
*       scalemax, scale);
*
*   FUNCTION
*       Calculate a numeric value from an external scale, swapping numeric
*       endpoints when Reverse is set.
*
*   INPUTS
*       scalemin, scalemax, scale - LONG values. Use ordered endpoints and
*       representable differences/products.
*
*   RESULT
*       Calculated LONG carried as IPTR; Value is not changed.
*
*   NOTES
*       The implementation clamps (scale - scalemin) between scalemin and
*       scalemax, then multiplies by the numeric span, divides by the scale span
*       when nonzero, and adds the numeric start. This is not a correct general
*       normalization for nonzero scalemin; a zero-based scale avoids that
*       particular defect. A zero scale span skips division, not the other
*       arithmetic. There is no final numeric clamp or overflow protection.
*
*/
/****** Numeric.mui/MUIM_Numeric_ValueToScale ************
*
*   NAME
*       MUIM_Numeric_ValueToScale
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Numeric_ValueToScale, scalemin,
*       scalemax);
*
*   FUNCTION
*       Map the stored Value to an external scale without changing Value.
*
*   INPUTS
*       scalemin, scalemax - Ordered LONG output bounds; differences/products
*       must be representable.
*
*   RESULT
*       Calculated scale position, clamped to the supplied scale bounds.
*
*   NOTES
*       Reverse swaps output endpoints. For nonzero numeric span, calculation
*       adds half the numeric span before integer division; reversed/negative
*       cases are not guaranteed symmetric rounding. Equal numeric bounds return
*       the selected starting endpoint. No overflow checking. Use
*       ValueToScaleExt to map a value other than stored Value.
*
*/
/****** Numeric.mui/MUIM_Numeric_ValueToScaleExt ************
*
*   NAME
*       MUIM_Numeric_ValueToScaleExt
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Numeric_ValueToScaleExt, value,
*       scalemin, scalemax);
*
*   FUNCTION
*       Map an explicit value to an external scale without changing Value.
*
*   INPUTS
*       value - LONG, clamped to Numeric bounds.
*       scalemin, scalemax - Ordered LONG output bounds; differences/products
*       must be representable.
*
*   RESULT
*       Calculated scale position, clamped to the supplied scale bounds.
*
*   NOTES
*       Reverse swaps output endpoints. For nonzero numeric span, calculation
*       adds half the numeric span before integer division; reversed/negative
*       cases are not guaranteed symmetric rounding. Equal numeric bounds return
*       the selected starting endpoint. No overflow checking. This is the public
*       Zune extension declared in numeric.h.
*
*/

extern struct Library *MUIMasterBase;

/**************************************************************************
 OM_NEW
**************************************************************************/
/*
 * Construct Area, initialize numeric defaults and borrow Format. Parse tags
 * then clamp explicit Value or Default once against final bounds. Return
 * object or zero on Area failure. Requires ordered bounds and valid
 * borrowed format; no extra allocation or format validation.
 */
IPTR Numeric__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_NumericData *data;
    struct TagItem *tags, *tag;

    BOOL value_set = FALSE;

    obj = (Object *) DoSuperMethodA(cl, obj, (Msg) msg);
    if (!obj)
        return 0;

    data = INST_DATA(cl, obj);
    data->format = "%ld";
    data->max = 100;
    data->min = 0;
    data->flags = 0;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Numeric_CheckAllSizes:
            _handle_bool_tag(data->flags, tag->ti_Data,
                NUMERIC_CHECKALLSIZES);
            break;
        case MUIA_Numeric_Default:
            /* data->defvalue = CLAMP(tag->ti_Data, data->min, data->max); */
            data->defvalue = tag->ti_Data;
            break;
        case MUIA_Numeric_Format:
            data->format = (STRPTR) tag->ti_Data;
            break;
        case MUIA_Numeric_Max:
            data->max = tag->ti_Data;
            break;
        case MUIA_Numeric_Min:
            data->min = tag->ti_Data;
            break;
        case MUIA_Numeric_Reverse:
            _handle_bool_tag(data->flags, tag->ti_Data, NUMERIC_REVERSE);
            break;
        case MUIA_Numeric_RevLeftRight:
            _handle_bool_tag(data->flags, tag->ti_Data,
                NUMERIC_REVLEFTRIGHT);
            break;
        case MUIA_Numeric_RevUpDown:
            _handle_bool_tag(data->flags, tag->ti_Data, NUMERIC_REVUPDOWN);
            break;
        case MUIA_Numeric_Value:
            value_set = TRUE;
            data->value = (LONG) tag->ti_Data;
            break;
        }
    }

    data->value =
        CLAMP(value_set ? data->value : data->defvalue, data->min,
        data->max);

    return (IPTR) obj;
}

/**************************************************************************
 OM_SET
**************************************************************************/
/*
 * Apply tags sequentially, clamping at each bound/Value change. Rewrite
 * Value data or suppress unchanged tag. Changed bounds/format pointer
 * request parent change cycle; superclass handles notifications, then
 * redraw if value or dimensions changed. Return superclass status. Writable
 * tags, valid parent and ordered bounds required; no synthetic Value tag
 * for bound-induced changes.
 */
IPTR Numeric__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_NumericData *data = INST_DATA(cl, obj);
    struct TagItem *tags, *tag;
    LONG oldval, oldmin, oldmax;
    STRPTR oldfmt;
    IPTR ret;
    BOOL values_changed = FALSE;

    oldval = data->value;
    oldfmt = data->format;
    oldmin = data->min;
    oldmax = data->max;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Numeric_CheckAllSizes:
            _handle_bool_tag(data->flags, tag->ti_Data,
                NUMERIC_CHECKALLSIZES);
            break;
        case MUIA_Numeric_Default:
            /* data->defvalue = CLAMP(tag->ti_Data, data->min, data->max); */
            data->defvalue = tag->ti_Data;
            break;
        case MUIA_Numeric_Format:
            data->format = (STRPTR) tag->ti_Data;
            break;
        case MUIA_Numeric_Max:
            data->max = tag->ti_Data;
            data->value = CLAMP(data->value, data->min, data->max);
            break;
        case MUIA_Numeric_Min:
            data->min = tag->ti_Data;
            data->value = CLAMP(data->value, data->min, data->max);
            break;
        case MUIA_Numeric_Reverse:
            _handle_bool_tag(data->flags, tag->ti_Data, NUMERIC_REVERSE);
            break;
        case MUIA_Numeric_RevLeftRight:
            _handle_bool_tag(data->flags, tag->ti_Data,
                NUMERIC_REVLEFTRIGHT);
            break;
        case MUIA_Numeric_RevUpDown:
            _handle_bool_tag(data->flags, tag->ti_Data, NUMERIC_REVUPDOWN);
            break;
        case MUIA_Numeric_Value:
            tag->ti_Data = CLAMP((LONG) tag->ti_Data, data->min, data->max);

            if (data->value == (LONG) tag->ti_Data)
                tag->ti_Tag = TAG_IGNORE;
            else
                data->value = (LONG) tag->ti_Data;

            break;
        }
    }

    /* If the max, min or format values changed, then the minimum and maximum
       sizes of the string output by MUIM_Numeric_Stringify may have changed,
       so give the subclass a chance to recalculate them and relayout the group
       accordingly. Basically, the subclass will have to react on changes to
       these values as well (by setting a notification on them, or by
       overriding OM_SET) and then recalculate the minimum and maximum sizes
       for the object. */
    if (data->format != oldfmt || data->min != oldmin
        || data->max != oldmax)
    {
        values_changed = TRUE;
        Object *parent = _parent(obj);
        if (parent)
        {
            DoMethod(parent, MUIM_Group_InitChange);
            DoMethod(parent, MUIM_Group_ExitChange);
        }
    }

    ret = DoSuperMethodA(cl, obj, (Msg) msg);

    if (data->value != oldval || values_changed)
    {
        MUI_Redraw(obj, MADF_DRAWUPDATE);
    }

    return ret;
}

/**************************************************************************
 OM_GET
**************************************************************************/
/*
 * Write stored numeric fields or normalized flag values through IPTR
 * storage, returning TRUE; otherwise delegate to Area. Format pointer is
 * borrowed; getter transfers no ownership.
 */
IPTR Numeric__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct MUI_NumericData *data = INST_DATA(cl, obj);
    IPTR *store = msg->opg_Storage;
    ULONG tag = msg->opg_AttrID;

    switch (tag)
    {
    case MUIA_Numeric_CheckAllSizes:
        *store = ((data->flags & NUMERIC_CHECKALLSIZES) != 0);
        return TRUE;

    case MUIA_Numeric_Default:
        *store = data->defvalue;
        return TRUE;

    case MUIA_Numeric_Format:
        *store = (IPTR) data->format;
        return TRUE;

    case MUIA_Numeric_Max:
        *store = data->max;
        return TRUE;

    case MUIA_Numeric_Min:
        *store = data->min;
        return TRUE;

    case MUIA_Numeric_Reverse:
        *store = ((data->flags & NUMERIC_REVERSE) != 0);
        return TRUE;

    case MUIA_Numeric_RevLeftRight:
        *store = ((data->flags & NUMERIC_REVLEFTRIGHT) != 0);
        return TRUE;

    case MUIA_Numeric_RevUpDown:
        *store = ((data->flags & NUMERIC_REVUPDOWN) != 0);
        return TRUE;

    case MUIA_Numeric_Value:
        *store = data->value;
        return TRUE;
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/**************************************************************************
 MUIM_Setup
**************************************************************************/
/*
 * Set up Area then register raw-key handler with this Numeric class. Return
 * FALSE on Area failure, otherwise TRUE ignoring registration result.
 * Requires connected window and paired Cleanup.
 */
IPTR Numeric__MUIM_Setup(struct IClass *cl, Object *obj,
    struct MUIP_Setup *msg)
{
    struct MUI_NumericData *data = INST_DATA(cl, obj);

    if (!DoSuperMethodA(cl, obj, (Msg) msg))
        return FALSE;

    data->ehn.ehn_Events = IDCMP_RAWKEY;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags = 0;
    data->ehn.ehn_Object = obj;
    data->ehn.ehn_Class = cl;
    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR) (&data->ehn));

    return TRUE;
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
/*
 * Remove Numeric raw-key handler then return Area Cleanup result. Requires
 * matching Setup and live window; no separate format ownership.
 */
IPTR Numeric__MUIM_Cleanup(struct IClass *cl, Object *obj,
    struct MUIP_Cleanup *msg)
{
    struct MUI_NumericData *data = INST_DATA(cl, obj);

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR) (&data->ehn));
    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/**************************************************************************
 MUIM_HandleEvent
**************************************************************************/
/*
 * Handle Numeric keys and wheel changes using stored bounds/reversal flags,
 * returning Eat for recognized events or zero otherwise. No Area
 * delegation. When muikey is NONE, imsg must be non-NULL; wheel hit testing
 * requires live geometry. Arithmetic can overflow before clamping; word
 * keys use vertical reversal branch.
 */
IPTR Numeric__MUIM_HandleEvent(struct IClass *cl, Object *obj,
    struct MUIP_HandleEvent *msg)
{
    struct MUI_NumericData *data = INST_DATA(cl, obj);
    IPTR result = 0;
    BOOL increase, change;

    if (msg->muikey != MUIKEY_NONE)
    {
        LONG step;
        BOOL use_absolute = FALSE, is_horizontal = FALSE;

        result = MUI_EventHandlerRC_Eat;

        if (data->max - data->min < 10)
            step = 1;
        else
            step = 10;

        switch (msg->muikey)
        {
        case MUIKEY_PRESS:
            return MUI_EventHandlerRC_Eat;

        case MUIKEY_TOGGLE:
            DoMethod(obj, MUIM_Numeric_SetDefault);
            return MUI_EventHandlerRC_Eat;

        case MUIKEY_RELEASE:
            return MUI_EventHandlerRC_Eat;

        case MUIKEY_TOP:
        case MUIKEY_LINESTART:
            use_absolute = TRUE;
            is_horizontal = TRUE;
            step = -1;
            break;

        case MUIKEY_BOTTOM:
        case MUIKEY_LINEEND:
            use_absolute = TRUE;
            is_horizontal = TRUE;
            step = 1;
            break;

        case MUIKEY_LEFT:
            is_horizontal = TRUE;
            step = -1;
            break;

        case MUIKEY_RIGHT:
            is_horizontal = TRUE;
            step = 1;
            break;

        case MUIKEY_UP:
            step = -1;
            break;

        case MUIKEY_DOWN:
            step = 1;
            break;

        case MUIKEY_PAGEDOWN:
        case MUIKEY_WORDRIGHT:
            break;

        case MUIKEY_PAGEUP:
        case MUIKEY_WORDLEFT:
                step = -step;
            break;

        default:
            return 0;
        }

        /* Send gadget in proper direction */
        if (step != 0)
        {
            if (data->flags & NUMERIC_REVERSE)
                step = -step;

            if ((is_horizontal && (data->flags & NUMERIC_REVLEFTRIGHT) != 0)
                || (!is_horizontal && (data->flags & NUMERIC_REVUPDOWN) != 0))
                step = -step;

            if (use_absolute)
            {
                if (step > 0)
                    step = data->max;
                else
                    step = data->min;
                step -= data->value;
            }

            DoMethod(obj, MUIM_Numeric_Increase, step);
        }
    }
    else if (msg->imsg->Class == IDCMP_RAWKEY
        && _isinobject(obj, msg->imsg->MouseX, msg->imsg->MouseY))
    {
        change = TRUE;
        switch (msg->imsg->Code)
        {
        case RAWKEY_NM_WHEEL_UP:
            increase = FALSE;
            break;
        case RAWKEY_NM_WHEEL_DOWN:
            increase = TRUE;
            break;
        default:
            change = FALSE;
        }
        if (change)
        {
            if (data->flags & NUMERIC_REVERSE)
                increase = !increase;
            DoMethod(obj, increase ?
                MUIM_Numeric_Increase : MUIM_Numeric_Decrease, 1);
            result = MUI_EventHandlerRC_Eat;
        }
    }

    return result;
}


/**************************************************************************
 MUIM_Numeric_Decrease
**************************************************************************/
/*
 * Implement the public Numeric Decrease contract above with the ABI-defined
 * message. Requires ordered numeric/scale bounds and representable
 * arithmetic. No resource ownership is transferred. Return and clamping
 * semantics are specified in that method contract; delegated setter status
 * is not propagated.
 */
IPTR Numeric__MUIM_Decrease(struct IClass *cl, Object *obj,
    struct MUIP_Numeric_Decrease *msg)
{
    struct MUI_NumericData *data = INST_DATA(cl, obj);
    LONG newval = CLAMP(data->value - msg->amount, data->min, data->max);
    if (newval != data->value)
        set(obj, MUIA_Numeric_Value, newval);

    return 1;
}

/**************************************************************************
 MUIM_Numeric_Increase
**************************************************************************/
/*
 * Implement the public Numeric Increase contract above with the ABI-defined
 * message. Requires ordered numeric/scale bounds and representable
 * arithmetic. No resource ownership is transferred. Return and clamping
 * semantics are specified in that method contract; delegated setter status
 * is not propagated.
 */
IPTR Numeric__MUIM_Increase(struct IClass *cl, Object *obj,
    struct MUIP_Numeric_Increase *msg)
{
    struct MUI_NumericData *data = INST_DATA(cl, obj);
    LONG newval = CLAMP(data->value + msg->amount, data->min, data->max);

    if (newval != data->value)
        set(obj, MUIA_Numeric_Value, newval);

    return 1;
}


/**************************************************************************
 MUIM_Numeric_ScaleToValue
**************************************************************************/
/*
 * Implement the public Numeric ScaleToValue contract above with the ABI-
 * defined message. Requires ordered numeric/scale bounds and representable
 * arithmetic. No resource ownership is transferred. Return and clamping
 * semantics are specified in that method contract; delegated setter status
 * is not propagated.
 */
IPTR Numeric__MUIM_ScaleToValue(struct IClass *cl, Object *obj,
    struct MUIP_Numeric_ScaleToValue *msg)
{
    struct MUI_NumericData *data = INST_DATA(cl, obj);
    LONG min, max;
    LONG val;
    LONG d;

    min = (data->flags & NUMERIC_REVERSE) ? data->max : data->min;
    max = (data->flags & NUMERIC_REVERSE) ? data->min : data->max;

    val = CLAMP(msg->scale - msg->scalemin, msg->scalemin, msg->scalemax);
    d = msg->scalemax - msg->scalemin;

    // FIXME: watch out for overflow here.
    val = val * (max - min);

    if (d)
        val /= d;

    val += min;

    return val;
}

/**************************************************************************
 MUIM_Numeric_SetDefault
**************************************************************************/
/*
 * Implement the public Numeric SetDefault contract above with the ABI-
 * defined message. Requires ordered numeric/scale bounds and representable
 * arithmetic. No resource ownership is transferred. Return and clamping
 * semantics are specified in that method contract; delegated setter status
 * is not propagated.
 */
IPTR Numeric__MUIM_SetDefault(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_NumericData *data = INST_DATA(cl, obj);

    set(obj, MUIA_Numeric_Value, CLAMP(data->defvalue, data->min,
            data->max));

    return 0;
}

/**************************************************************************
 MUIM_Numeric_Stringify
**************************************************************************/
/*
 * Implement the public Numeric Stringify contract above with the ABI-
 * defined message. Requires ordered numeric/scale bounds and representable
 * arithmetic. Returns a borrowed per-instance buffer overwritten on the
 * next call; do not free it.
 */
IPTR Numeric__MUIM_Stringify(struct IClass *cl, Object *obj,
    struct MUIP_Numeric_Stringify *msg)
{
    struct MUI_NumericData *data = INST_DATA(cl, obj);

    snprintf(data->buf, 49, data->format, (long)msg->value);
    data->buf[49] = 0;

    return (IPTR) data->buf;
}

/**************************************************************************
 MUIM_Numeric_ValueToScale
**************************************************************************/
/*
 * Implement the public Numeric ValueToScale contract above with the ABI-
 * defined message. Requires ordered numeric/scale bounds and representable
 * arithmetic. No resource ownership is transferred. Return and clamping
 * semantics are specified in that method contract; delegated setter status
 * is not propagated.
 */
IPTR Numeric__MUIM_ValueToScale(struct IClass *cl, Object *obj,
    struct MUIP_Numeric_ValueToScale *msg)
{
    LONG val;
    struct MUI_NumericData *data = INST_DATA(cl, obj);
    LONG min, max;

    min = (data->flags & NUMERIC_REVERSE) ? msg->scalemax : msg->scalemin;
    max = (data->flags & NUMERIC_REVERSE) ? msg->scalemin : msg->scalemax;

    if (data->max != data->min)
    {
        val =
            min + ((data->value - data->min) * (max - min) + (data->max -
                data->min) / 2) / (data->max - data->min);
    }
    else
    {
        val = min;
    }

    val = CLAMP(val, msg->scalemin, msg->scalemax);

    return val;
}

/**************************************************************************
 MUIM_Numeric_ValueToScaleExt
**************************************************************************/
/*
 * Implement the public Numeric ValueToScaleExt contract above with the ABI-
 * defined message. Requires ordered numeric/scale bounds and representable
 * arithmetic. No resource ownership is transferred. Return and clamping
 * semantics are specified in that method contract; delegated setter status
 * is not propagated.
 */
IPTR Numeric__MUIM_ValueToScaleExt(struct IClass *cl, Object *obj,
    struct MUIP_Numeric_ValueToScaleExt *msg)
{
    LONG scale;
    LONG value;
    struct MUI_NumericData *data = INST_DATA(cl, obj);
    LONG min, max;

    value = CLAMP(msg->value, data->min, data->max);
    min = (data->flags & NUMERIC_REVERSE) ? msg->scalemax : msg->scalemin;
    max = (data->flags & NUMERIC_REVERSE) ? msg->scalemin : msg->scalemax;

    if (data->max != data->min)
    {
        scale =
            min + ((value - data->min) * (max - min) + (data->max -
                data->min) / 2) / (data->max - data->min);
    }
    else
    {
        scale = min;
    }

    scale = CLAMP(scale, msg->scalemin, msg->scalemax);

    return scale;
}

/**************************************************************************
 MUIM_Export - to export an object's "contents" to a dataspace object.
**************************************************************************/
/*
 * For nonzero ObjectID, pass a stack LONG Value to Dataspace_Add, which
 * copies it. Ignore result and return zero. Requires valid borrowed
 * Dataspace; no Area export and no ownership transfer of stack storage.
 */
IPTR Numeric__MUIM_Export(struct IClass *cl, Object *obj,
    struct MUIP_Export *msg)
{
    struct MUI_NumericData *data = INST_DATA(cl, obj);
    ULONG id;

    if ((id = muiNotifyData(obj)->mnd_ObjectID))
    {
        LONG value = data->value;
        DoMethod(msg->dataspace, MUIM_Dataspace_Add,
            (IPTR) & value, sizeof(value), (IPTR) id);
    }
    return 0;
}

/**************************************************************************
 MUIM_Import - to import an object's "contents" from a dataspace object.
**************************************************************************/
/*
 * For nonzero ObjectID, find borrowed Dataspace bytes and read a LONG
 * without size validation, then set Value normally. Return zero and ignore
 * setter status. Requires matching stored representation; no Area import.
 */
IPTR Numeric__MUIM_Import(struct IClass *cl, Object *obj,
    struct MUIP_Import *msg)
{
    ULONG id;
    LONG *s;

    if ((id = muiNotifyData(obj)->mnd_ObjectID))
    {
        if ((s = (LONG *) DoMethod(msg->dataspace, MUIM_Dataspace_Find,
                    (IPTR) id)))
        {
            set(obj, MUIA_Numeric_Value, *s);
        }
    }

    return 0;
}


/*
 * Dispatch numeric methods, lifecycle, accessors and serialization;
 * delegate others to Area. Return chosen handler result. Requires ABI-
 * correct message and live object except construction.
 */
BOOPSI_DISPATCHER(IPTR, Numeric_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Numeric__OM_NEW(cl, obj, (APTR) msg);
    case OM_SET:
        return Numeric__OM_SET(cl, obj, (APTR) msg);
    case OM_GET:
        return Numeric__OM_GET(cl, obj, (APTR) msg);

    case MUIM_Setup:
        return Numeric__MUIM_Setup(cl, obj, (APTR) msg);
    case MUIM_Cleanup:
        return Numeric__MUIM_Cleanup(cl, obj, (APTR) msg);
    case MUIM_HandleEvent:
        return Numeric__MUIM_HandleEvent(cl, obj, (APTR) msg);
    case MUIM_Numeric_Decrease:
        return Numeric__MUIM_Decrease(cl, obj, (APTR) msg);
    case MUIM_Numeric_Increase:
        return Numeric__MUIM_Increase(cl, obj, (APTR) msg);
    case MUIM_Numeric_ScaleToValue:
        return Numeric__MUIM_ScaleToValue(cl, obj, (APTR) msg);
    case MUIM_Numeric_SetDefault:
        return Numeric__MUIM_SetDefault(cl, obj, (APTR) msg);
    case MUIM_Numeric_Stringify:
        return Numeric__MUIM_Stringify(cl, obj, (APTR) msg);
    case MUIM_Numeric_ValueToScale:
        return Numeric__MUIM_ValueToScale(cl, obj, (APTR) msg);
    case MUIM_Numeric_ValueToScaleExt:
        return Numeric__MUIM_ValueToScaleExt(cl, obj, (APTR) msg);
    case MUIM_Export:
        return Numeric__MUIM_Export(cl, obj, (APTR) msg);
    case MUIM_Import:
        return Numeric__MUIM_Import(cl, obj, (APTR) msg);
    }

    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Numeric_desc =
{
    MUIC_Numeric,
    MUIC_Area,
    sizeof(struct MUI_NumericData),
    (void *) Numeric_Dispatcher
};
