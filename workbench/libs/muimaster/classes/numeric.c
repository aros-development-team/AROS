/*
    Copyright � 2002-2013, The AROS Development Team. All rights reserved.
    $Id$
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

extern struct Library *MUIMasterBase;

/**************************************************************************
 OM_NEW
**************************************************************************/
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
IPTR Numeric__MUIM_Stringify(struct IClass *cl, Object *obj,
    struct MUIP_Numeric_Stringify *msg)
{
    struct MUI_NumericData *data = INST_DATA(cl, obj);

    /* TODO: use RawDoFmt() and buffer overrun */
    snprintf(data->buf, 49, data->format, (long)msg->value);
    data->buf[49] = 0;

    return (IPTR) data->buf;
}

/**************************************************************************
 MUIM_Numeric_ValueToScale
**************************************************************************/
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
