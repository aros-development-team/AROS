/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <stdio.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

struct MUI_NumericData
{
    STRPTR format;
    LONG   defvalue;
    LONG   max;
    LONG   min;
    LONG   value;
    ULONG  flags;
    struct MUI_EventHandlerNode ehn;
    char buf[50];
};

enum numeric_flags {
    NUMERIC_REVERSE = (1<<0),
    NUMERIC_REVLEFTRIGHT = (1<<1),
    NUMERIC_REVUPDOWN = (1<<2),
    NUMERIC_CHECKALLSIZES = (1<<3),
};

extern struct Library *MUIMasterBase;

#include <aros/debug.h>

/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR  Numeric_New(struct IClass *cl, Object * obj, struct opSet *msg)
{
    struct MUI_NumericData *data;
    struct TagItem *tags, *tag;
    
    BOOL value_set = FALSE;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj)
	return 0;

    data = INST_DATA(cl, obj);
    data->format = "%ld";
    data->max    = 100;
    data->min    =   0;
    data->flags  =   0;

    for (tags = msg->ops_AttrList; (tag = NextTagItem((const struct TagItem **)&tags));)
    {
	switch (tag->ti_Tag)
	{
	case MUIA_Numeric_CheckAllSizes:
	  _handle_bool_tag(data->flags, tag->ti_Data, NUMERIC_CHECKALLSIZES);
	  break;
	case MUIA_Numeric_Default:
	  /* data->defvalue = CLAMP(tag->ti_Data, data->min, data->max); */
	  data->defvalue = tag->ti_Data;
	  break;
	case MUIA_Numeric_Format:
	  data->format = (STRPTR)tag->ti_Data;
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
	  _handle_bool_tag(data->flags, tag->ti_Data, NUMERIC_REVLEFTRIGHT);
	  break;
	case MUIA_Numeric_RevUpDown:
	  _handle_bool_tag(data->flags, tag->ti_Data, NUMERIC_REVUPDOWN);
	  break;
	case MUIA_Numeric_Value:
	  value_set   = TRUE;
	  data->value = (LONG)tag->ti_Data;
	  break;
	}
    }

    data->value = CLAMP(value_set ? data->value : data->defvalue, data->min, data->max);

    return (ULONG)obj;
}

/**************************************************************************
 OM_SET
**************************************************************************/
static IPTR Numeric_Set(struct IClass *cl, Object * obj, struct opSet *msg)
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

    for (tags = msg->ops_AttrList; (tag = NextTagItem((const struct TagItem **)&tags));)
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Numeric_CheckAllSizes:
		_handle_bool_tag(data->flags, tag->ti_Data, NUMERIC_CHECKALLSIZES);
		break;
	    case MUIA_Numeric_Default:
		/* data->defvalue = CLAMP(tag->ti_Data, data->min, data->max); */
		data->defvalue = tag->ti_Data;
		break;
	    case MUIA_Numeric_Format:
		data->format = (STRPTR)tag->ti_Data;
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
		_handle_bool_tag(data->flags, tag->ti_Data, NUMERIC_REVLEFTRIGHT);
		break;
	    case MUIA_Numeric_RevUpDown:
		_handle_bool_tag(data->flags, tag->ti_Data, NUMERIC_REVUPDOWN);
		break;
	    case MUIA_Numeric_Value:
	        tag->ti_Data = CLAMP((LONG)tag->ti_Data, data->min, data->max);
		
		if (data->value == (LONG)tag->ti_Data)
		    tag->ti_Tag = TAG_IGNORE;
		else
		    data->value = (LONG)tag->ti_Data;
		
		break;
	}
    }

    /* If the max, min or format values changed, then the minimum and maximum sizes
       of the string output by MUIM_Numeric_Strigify maye have changed, so
       give the subclass a chance to recalculate them and relayout the group
       accordingly. Basically, the subclass will have to react on changes to
       these values as well (by setting a notification on them, or by overriding
       OM_SET) and then recalculate the minimum and maximum sizes for the object.  */      
    if (data->format != oldfmt || data->min != oldmin || data->max != oldmax)
    {
        values_changed = TRUE;
        DoMethod(_parent(obj), MUIM_Group_InitChange);
        DoMethod(_parent(obj), MUIM_Group_ExitChange);
    }
	

    ret = DoSuperMethodA(cl, obj, (Msg)msg);

    if (data->value != oldval || values_changed)
    {
	MUI_Redraw(obj, MADF_DRAWUPDATE);
    }
    
    return ret;
}

/**************************************************************************
 OM_GET
**************************************************************************/
static ULONG  Numeric_Get(struct IClass *cl, Object * obj, struct opGet *msg)
{
    struct MUI_NumericData *data = INST_DATA(cl, obj);
    ULONG *store = msg->opg_Storage;
    ULONG    tag = msg->opg_AttrID;

    switch (tag)
    {
    case MUIA_Numeric_CheckAllSizes:
      *store = ((data->flags & NUMERIC_CHECKALLSIZES) != 0);
      return TRUE;
    case MUIA_Numeric_Default:
      *store = data->defvalue;
      return TRUE;
    case MUIA_Numeric_Format:
      *store = (ULONG)data->format;
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

    return DoSuperMethodA(cl, obj, (Msg)msg);
}


/**************************************************************************
 MUIM_Setup
**************************************************************************/
static ULONG Numeric_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_NumericData *data = INST_DATA(cl, obj);

    if (!DoSuperMethodA(cl,obj,(Msg)msg))
	return FALSE;

    data->ehn.ehn_Events = IDCMP_RAWKEY;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags    = 0;
    data->ehn.ehn_Object   = obj;
    data->ehn.ehn_Class    = cl;
    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)(&data->ehn));

    return TRUE;
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
static ULONG Numeric_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_NumericData *data = INST_DATA(cl, obj);

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)(&data->ehn));
    return DoSuperMethodA(cl,obj,(Msg)msg);
}

/**************************************************************************
 MUIM_HandleEvent
**************************************************************************/
static ULONG Numeric_HandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
    struct MUI_NumericData *data = INST_DATA(cl, obj);

    if (msg->muikey != MUIKEY_NONE)
    {
	LONG step;

	if (data->max - data->min < 10)
	    step = 1;
	else
	    step = 10;

	switch(msg->muikey)
	{
	    case    MUIKEY_PRESS:
		    return MUI_EventHandlerRC_Eat;

	    case    MUIKEY_TOGGLE:
		    DoMethod(obj, MUIM_Numeric_SetDefault);
		    return MUI_EventHandlerRC_Eat;

	    case    MUIKEY_RELEASE:
		    return MUI_EventHandlerRC_Eat;

	    case    MUIKEY_BOTTOM:
	    case    MUIKEY_LINEEND:
		    if (data->flags & NUMERIC_REVUPDOWN)
		        set(obj, MUIA_Numeric_Value, data->min);
		    else
			set(obj, MUIA_Numeric_Value, data->max);
		    return MUI_EventHandlerRC_Eat;

	    case    MUIKEY_TOP:
	    case    MUIKEY_LINESTART:
		    if (data->flags & NUMERIC_REVUPDOWN)
			set(obj, MUIA_Numeric_Value, data->max);
		    else
			set(obj, MUIA_Numeric_Value, data->min);
		    return MUI_EventHandlerRC_Eat;

	    case    MUIKEY_LEFT:
		    if (data->flags & NUMERIC_REVLEFTRIGHT)
			DoMethod(obj, MUIM_Numeric_Increase, 1);
		    else
			DoMethod(obj, MUIM_Numeric_Decrease, 1);
		    return MUI_EventHandlerRC_Eat;

	    case    MUIKEY_RIGHT:
		    if (data->flags & NUMERIC_REVLEFTRIGHT)
			DoMethod(obj, MUIM_Numeric_Decrease, 1);
		    else
			DoMethod(obj, MUIM_Numeric_Increase, 1);
		    return MUI_EventHandlerRC_Eat;

	    case    MUIKEY_UP:
		    if (data->flags & NUMERIC_REVUPDOWN)
			DoMethod(obj, MUIM_Numeric_Increase, 1);
		    else
			DoMethod(obj, MUIM_Numeric_Decrease, 1);
		    return MUI_EventHandlerRC_Eat;

	    case    MUIKEY_DOWN:
		    if (data->flags & NUMERIC_REVUPDOWN)
			DoMethod(obj, MUIM_Numeric_Decrease, 1);
		    else
			DoMethod(obj, MUIM_Numeric_Increase, 1);
		    return MUI_EventHandlerRC_Eat;

	    case MUIKEY_PAGEDOWN:
	    case MUIKEY_WORDRIGHT:
		if (data->flags & NUMERIC_REVUPDOWN)
		    DoMethod(obj, MUIM_Numeric_Decrease, step);
		else
		    DoMethod(obj, MUIM_Numeric_Increase, step);
		return MUI_EventHandlerRC_Eat;

	    case MUIKEY_PAGEUP:
	    case MUIKEY_WORDLEFT:
		if (data->flags & NUMERIC_REVUPDOWN)
		    DoMethod(obj, MUIM_Numeric_Increase, step);
		else
		    DoMethod(obj, MUIM_Numeric_Decrease, step);
		return MUI_EventHandlerRC_Eat;

	    default:
		    return 0;
	}
    }

    return 0;
}


/**************************************************************************
 MUIM_Numeric_Decrease
**************************************************************************/
static ULONG  Numeric_Decrease(struct IClass *cl, Object * obj, struct MUIP_Numeric_Decrease *msg)
{
    struct MUI_NumericData *data = INST_DATA(cl, obj);
    LONG newval = CLAMP(data->value - msg->amount, data->min, data->max);
    if (newval != data->value) set(obj,MUIA_Numeric_Value, newval);
    return 1;
}

/**************************************************************************
 MUIM_Numeric_Increase
**************************************************************************/
static ULONG Numeric_Increase(struct IClass *cl, Object * obj, struct MUIP_Numeric_Increase *msg)
{
    struct MUI_NumericData *data = INST_DATA(cl, obj);
    LONG newval = CLAMP(data->value + msg->amount, data->min, data->max);

    if (newval != data->value) set(obj,MUIA_Numeric_Value, newval);
    return 1;
}


/**************************************************************************
 MUIM_Numeric_ScaleToValue
**************************************************************************/
static ULONG Numeric_ScaleToValue(struct IClass *cl, Object * obj, struct MUIP_Numeric_ScaleToValue *msg)
{
    struct MUI_NumericData *data = INST_DATA(cl, obj);
    LONG min, max;
    LONG val;
    LONG d;

    min = (data->flags & NUMERIC_REVERSE) ? data->max : data->min;
    max = (data->flags & NUMERIC_REVERSE) ? data->min : data->max;

    val  = CLAMP(msg->scale - msg->scalemin, msg->scalemin, msg->scalemax);
    d    = msg->scalemax -  msg->scalemin;

#warning FIXME: watch out for overflow here.
    val  = val * (max - min);
    
    if (d)
        val /= d;
    
    val += min;

    return val;
}

/**************************************************************************
 MUIM_Numeric_SetDefault
**************************************************************************/
static IPTR Numeric_SetDefault(struct IClass *cl, Object * obj, Msg msg)
{
    struct MUI_NumericData *data = INST_DATA(cl, obj);

    set(obj, MUIA_Numeric_Value, CLAMP(data->defvalue, data->min, data->max));
    
    return 0;
}

/**************************************************************************
 MUIM_Numeric_Stringify
**************************************************************************/
static IPTR Numeric_Stringify(struct IClass *cl, Object * obj, struct MUIP_Numeric_Stringify *msg)
{
    struct MUI_NumericData *data = INST_DATA(cl, obj);

    /* TODO: use RawDoFmt() and buffer overrun */
    snprintf(data->buf, 49, data->format, msg->value);
    data->buf[49] = 0;
    
    return (IPTR)data->buf;
}

/**************************************************************************
 MUIM_Numeric_ValueToScale
**************************************************************************/
static LONG  Numeric_ValueToScale(struct IClass *cl, Object * obj, struct MUIP_Numeric_ValueToScale *msg)
{
    LONG val;
    struct MUI_NumericData *data = INST_DATA(cl, obj);
    LONG min, max;

    min = (data->flags & NUMERIC_REVERSE) ? msg->scalemax : msg->scalemin;
    max = (data->flags & NUMERIC_REVERSE) ? msg->scalemin : msg->scalemax;

    if (data->max != data->min)
    {
	val = min + ((data->value - data->min) * (max - min) +  (data->max - data->min)/2) / (data->max - data->min);
    }
    else
    {
	val = min;
    }
    
    val = CLAMP(val, min, max);

    return val;
}

/**************************************************************************
 MUIM_Numeric_ValueToScaleExt
**************************************************************************/
static LONG  Numeric_ValueToScaleExt(struct IClass *cl, Object * obj, struct MUIP_Numeric_ValueToScaleExt *msg)
{
    LONG scale;
    LONG value;
    struct MUI_NumericData *data = INST_DATA(cl, obj);
    LONG min, max;

    value = CLAMP(msg->value,data->min,data->max);
    min = (data->flags & NUMERIC_REVERSE) ? msg->scalemax : msg->scalemin;
    max = (data->flags & NUMERIC_REVERSE) ? msg->scalemin : msg->scalemax;

    if (data->max != data->min)
    {
	scale = min + ((value - data->min) * (max - min) +  (data->max - data->min)/2) / (data->max - data->min);
    }
    else
    {
	scale = min;
    }
    
    scale = CLAMP(scale, min, max);

    return scale;
}

/**************************************************************************
 MUIM_Export - to export an objects "contents" to a dataspace object.
**************************************************************************/
static ULONG Numeric_Export(struct IClass *cl, Object *obj, struct MUIP_Export *msg)
{
#if 0
    struct MUI_NumericData *data = INST_DATA(cl, obj);
    STRPTR id;

    if ((id = muiNotifyData(obj)->mnd_ObjectID))
    {
	DoMethod(msg->dataspace, MUIM_Dataspace_AddInt,
		 _U(id), _U("value"), _U(&data->value));
    }
#endif
    return 0;
}

/**************************************************************************
 MUIM_Import - to import an objects "contents" from a dataspace object.
**************************************************************************/
static ULONG Numeric_Import(struct IClass *cl, Object *obj, struct MUIP_Import *msg)
{
#if 0
    struct MUI_NumericData *data = INST_DATA(cl, obj);
    STRPTR id;
    LONG val;

    if ((id = muiNotifyData(obj)->mnd_ObjectID))
    {
	if (DoMethod(msg->dataspace, MUIM_Dataspace_FindString, _U(id), _U("value")))
	{
	    DoMethod(msg->dataspace, MUIM_Dataspace_FindInt,
		     _U(id), _U("value"), _U(&val));
	    set(obj, MUIA_Numeric_Value, val);
	}
    }
#endif
    return 0;
}


BOOPSI_DISPATCHER(IPTR, Numeric_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Numeric_New(cl, obj, (APTR)msg);
	case OM_SET: return Numeric_Set(cl, obj, (APTR)msg);
	case OM_GET: return Numeric_Get(cl, obj, (APTR)msg);
	case MUIM_Setup: return Numeric_Setup(cl, obj, (APTR)msg);
	case MUIM_Cleanup: return Numeric_Cleanup(cl, obj, (APTR)msg);
	case MUIM_HandleEvent:  return Numeric_HandleEvent(cl, obj, (APTR)msg);
	case MUIM_Numeric_Decrease: return Numeric_Decrease(cl, obj, (APTR)msg);
	case MUIM_Numeric_Increase: return Numeric_Increase(cl, obj, (APTR)msg);
	case MUIM_Numeric_ScaleToValue: return Numeric_ScaleToValue(cl, obj, (APTR)msg);
	case MUIM_Numeric_SetDefault: return Numeric_SetDefault(cl, obj, (APTR)msg);
	case MUIM_Numeric_Stringify: return Numeric_Stringify(cl, obj, (APTR)msg);
	case MUIM_Numeric_ValueToScale: return (IPTR)Numeric_ValueToScale(cl, obj, (APTR)msg);
	case MUIM_Numeric_ValueToScaleExt: return (IPTR)Numeric_ValueToScaleExt(cl, obj, (APTR)msg);
	case MUIM_Export: return Numeric_Export(cl, obj, (APTR)msg);
	case MUIM_Import: return Numeric_Import(cl, obj, (APTR)msg);	
    }

    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END


/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Numeric_desc = { 
    MUIC_Numeric, 
    MUIC_Area, 
    sizeof(struct MUI_NumericData), 
    (void*)Numeric_Dispatcher 
};

