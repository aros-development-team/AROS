/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <exec/types.h>

#ifdef __AROS__
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#endif

#include <zunepriv.h>
#include <builtin.h>

#include <Area.h>
#include <Numeric.h>
#include <numericdata.h>
#include <muikey.h>
#include <Window.h>
#include <renderinfo.h>
#include <Notify.h>
#include <Dataspace.h>

static ULONG 
mNew(struct IClass *cl, Object * obj, struct opSet *msg)
{
    struct MUI_NumericData *data;
    struct TagItem *tags, *tag;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj)
	return NULL;

    data = INST_DATA(cl, obj);
    data->format = "%d";
    data->max    = 100;
    data->min    =   0;
    data->flags  =   0;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
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
	  data->value = CLAMP((ULONG)tag->ti_Data, data->min, data->max);
	  break;
	}
    }

    return (ULONG)obj;
}


static ULONG 
mSet(struct IClass *cl, Object * obj, struct opSet *msg)
{
    struct MUI_NumericData *data = INST_DATA(cl, obj);
    struct TagItem *tags, *tag;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
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
	  data->value = CLAMP((ULONG)tag->ti_Data, data->min, data->max);
	  MUI_Redraw(obj, MADF_DRAWUPDATE);
	  break;
	}
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

static ULONG 
mGet(struct IClass *cl, Object * obj, struct opGet *msg)
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


static void
setup_control_char (struct MUI_NumericData *data, Object *obj, struct IClass *cl)
{
    data->ccn.ehn_Events = muiAreaData(obj)->mad_ControlChar;
    data->ccn.ehn_Flags = MUIKEY_PRESS;
    data->ccn.ehn_Priority = 0;
    data->ccn.ehn_Object = obj;
    data->ccn.ehn_Class = cl;
    DoMethod(_win(obj), MUIM_Window_AddControlCharHandler, _U(&data->ccn));
}

static void
cleanup_control_char (struct MUI_NumericData *data, Object *obj)
{
    DoMethod(_win(obj), MUIM_Window_RemControlCharHandler, _U(&data->ccn));
}

static ULONG
mSetup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_NumericData *data = INST_DATA(cl, obj);

    if (!DoSuperMethodA(cl,obj,(Msg)msg))
	return FALSE;

#ifdef __AROS__
    data->ehn.ehn_Events = IDCMP_VANILLAKEY;
#else
    data->ehn.ehn_Events = GDK_KEY_PRESS_MASK;
#endif

    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags    = 0;
    data->ehn.ehn_Object   = obj;
    data->ehn.ehn_Class    = cl;
    DoMethod(_win(obj), MUIM_Window_AddEventHandler, _U(&data->ehn));

    setup_control_char (data, obj, cl);

    return TRUE;
}

static ULONG
mCleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_NumericData *data = INST_DATA(cl, obj);

    cleanup_control_char (data, obj);
    DoMethod(_win(obj), MUIM_Window_RemEventHandler, _U(&data->ehn));
    return DoSuperMethodA(cl,obj,(Msg)msg);
}

static ULONG
mHandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
    struct MUI_NumericData *data = INST_DATA(cl, obj);

    if (msg->muikey != MUIKEY_NONE)
    {
	switch(msg->muikey)
	{
	case MUIKEY_PRESS:
	  return MUI_EventHandlerRC_Eat;
	case MUIKEY_TOGGLE:
	  DoMethod(obj, MUIM_Numeric_SetDefault);
	  return MUI_EventHandlerRC_Eat;
	case MUIKEY_RELEASE: /* fake, after a MUIKEY_PRESS */
	  return MUI_EventHandlerRC_Eat;
	case MUIKEY_TOP:
	  if (data->flags & NUMERIC_REVUPDOWN)
	    set(obj, MUIA_Numeric_Value, data->min);
	  else
	    set(obj, MUIA_Numeric_Value, data->max);
	  return MUI_EventHandlerRC_Eat;
	case MUIKEY_BOTTOM:
	  if (data->flags & NUMERIC_REVUPDOWN)
	    set(obj, MUIA_Numeric_Value, data->max);
	  else
	    set(obj, MUIA_Numeric_Value, data->min);
	  return MUI_EventHandlerRC_Eat;
	case MUIKEY_LEFT:
	  if (data->flags & NUMERIC_REVLEFTRIGHT)
	    DoMethod(obj, MUIM_Numeric_Increase, 1);
	  else
	    DoMethod(obj, MUIM_Numeric_Decrease, 1);
	  return MUI_EventHandlerRC_Eat;
	case MUIKEY_RIGHT:
	  if (data->flags & NUMERIC_REVLEFTRIGHT)
	    DoMethod(obj, MUIM_Numeric_Decrease, 1);
	  else
	    DoMethod(obj, MUIM_Numeric_Increase, 1);
	  return MUI_EventHandlerRC_Eat;
	default:
	  return 0;
	}
    }

    return MUI_EventHandlerRC_Eat;
}


/*** MUI_Numeric methods ***/


static ULONG 
mDecrease(struct IClass *cl, Object * obj, struct MUIP_Numeric_Decrease *msg)
{
    struct MUI_NumericData *data = INST_DATA(cl, obj);

    data->value = CLAMP(data->value - msg->amount, data->min, data->max);
    MUI_Redraw(obj, MADF_DRAWUPDATE);
    return 0;
}


static ULONG 
mIncrease(struct IClass *cl, Object * obj, struct MUIP_Numeric_Increase *msg)
{
    struct MUI_NumericData *data = INST_DATA(cl, obj);

    data->value = CLAMP(data->value + msg->amount, data->min, data->max);
    MUI_Redraw(obj, MADF_DRAWUPDATE);
    return 0;
}


static ULONG 
mScaleToValue(struct IClass *cl, Object * obj,
	      struct MUIP_Numeric_ScaleToValue *msg)
{
    DOUBLE val;
    struct MUI_NumericData *data = INST_DATA(cl, obj);
    LONG min, max;

    min = (data->flags & NUMERIC_REVERSE) ? data->max : data->min;
    max = (data->flags & NUMERIC_REVERSE) ? data->min : data->max;

    val = min + msg->scale
	* (max - min) / (DOUBLE)(msg->scalemax - msg->scalemin);
    val = CLAMP(val, data->min, data->max);
    return (LONG)(val + 0.5);
}


static ULONG 
mSetDefault(struct IClass *cl, Object * obj, Msg msg)
{
    struct MUI_NumericData *data = INST_DATA(cl, obj);

    data->value = CLAMP(data->defvalue, data->min, data->max);
    MUI_Redraw(obj, MADF_DRAWUPDATE);
    return 0;
}


static ULONG 
mStringify(struct IClass *cl, Object * obj, struct MUIP_Numeric_Stringify *msg)
{
    struct MUI_NumericData *data = INST_DATA(cl, obj);
    static char buf[50];

    g_snprintf(buf, 50, data->format, msg->value);
    return (ULONG)buf;
}


static ULONG 
mValueToScale(struct IClass *cl, Object * obj, struct MUIP_Numeric_ValueToScale *msg)
{
    DOUBLE val;
    struct MUI_NumericData *data = INST_DATA(cl, obj);
    LONG min, max;

    min = (data->flags & NUMERIC_REVERSE) ? msg->scalemax : msg->scalemin;
    max = (data->flags & NUMERIC_REVERSE) ? msg->scalemin : msg->scalemax;

    val = min + data->value
	* (max - min) / (DOUBLE)(data->max - data->min);
    val = CLAMP(val, min, max);
    return (LONG)(val + 0.5);
}

/*
 * MUIM_Export : to export an objects "contents" to a dataspace object.
 */
static ULONG
mExport(struct IClass *cl, Object *obj, struct MUIP_Export *msg)
{
    struct MUI_NumericData *data = INST_DATA(cl, obj);
    STRPTR id;

    if ((id = muiNotifyData(obj)->mnd_ObjectID))
    {
	DoMethod(msg->dataspace, MUIM_Dataspace_AddInt,
		 _U(id), _U("value"), _U(&data->value));
    }
    return 0;
}


/*
 * MUIM_Import : to import an objects "contents" from a dataspace object.
 */
static ULONG
mImport(struct IClass *cl, Object *obj, struct MUIP_Import *msg)
{
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
    return 0;
}


/* static ULONG  */
/* Numeric_Dispatcher(struct IClass *cl, Object * obj, Msg msg) */
AROS_UFH3S(IPTR, Numeric_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
{
    switch (msg->MethodID)
    {
    case OM_NEW:
      return mNew(cl, obj, (APTR)msg);
    case OM_SET:
      return mSet(cl, obj, (APTR)msg);
    case OM_GET:
      return mGet(cl, obj, (APTR)msg);
    case MUIM_Setup:
      return mSetup(cl, obj, (APTR)msg);
    case MUIM_Cleanup :
      return mCleanup(cl, obj, (APTR)msg);
    case MUIM_HandleEvent:
      return mHandleEvent(cl, obj, (APTR)msg);
    case MUIM_Numeric_Decrease:
      return mDecrease(cl, obj, (APTR)msg);
    case MUIM_Numeric_Increase:
      return mIncrease(cl, obj, (APTR)msg);
    case MUIM_Numeric_ScaleToValue:
      return mScaleToValue(cl, obj, (APTR)msg);
    case MUIM_Numeric_SetDefault:
      return mSetDefault(cl, obj, (APTR)msg);
    case MUIM_Numeric_Stringify:
      return mStringify(cl, obj, (APTR)msg);
    case MUIM_Numeric_ValueToScale:
      return mValueToScale(cl, obj, (APTR)msg);
    case MUIM_Export :
      return mExport(cl, obj, (APTR)msg);
    case MUIM_Import :
      return mImport(cl, obj, (APTR)msg);	
    }

    return DoSuperMethodA(cl, obj, msg);
}


/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Numeric_desc = { 
    MUIC_Numeric, 
    MUIC_Area, 
    sizeof(struct MUI_NumericData), 
    Numeric_Dispatcher 
};

/*** EOF ***/
