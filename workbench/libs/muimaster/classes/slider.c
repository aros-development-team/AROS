/* Zune -- a free Magic User Interface implementation
 * Copyright (C) 1999 David Le Corfec
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <exec/types.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "support.h"
#include "mui.h"
#include "muimaster_intern.h"

extern struct Library *MUIMasterBase;


/*
Slider.mui/MUIA_Slider_Horiz        
Slider.mui/MUIA_Slider_Level        
Slider.mui/MUIA_Slider_Max          
Slider.mui/MUIA_Slider_Min          
Slider.mui/MUIA_Slider_Quiet        
Slider.mui/MUIA_Slider_Reverse      d
*/

/**************************************************************************
 OM_NEW
**************************************************************************/
static ULONG Slider_New(struct IClass *cl, Object * obj, struct opSet *msg)
{
    struct MUI_SliderData *data;
    struct TagItem *tags, *tag;
    ULONG flags = 0;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
	switch (tag->ti_Tag)
	{
	case MUIA_Group_Horiz:
	case MUIA_Slider_Horiz:
	  _handle_bool_tag(flags, tag->ti_Data, SLIDER_HORIZ);
	  break;
	case MUIA_Slider_Quiet:
	  _handle_bool_tag(flags, tag->ti_Data, SLIDER_QUIET);
	  break;
	}
    }

    obj = (Object *)DoSuperNew(cl, obj,
	MUIA_Background, MUII_SliderBack,
	MUIA_Frame, MUIV_Frame_Slider,
	TAG_MORE, msg->ops_AttrList);

    if (!obj)
    {
	return NULL;
    }

    data = INST_DATA(cl, obj);
    data->flags = flags;

    data->ehn.ehn_Events = IDCMP_MOUSEBUTTONS;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags    = 0;
    data->ehn.ehn_Object   = obj;
    data->ehn.ehn_Class    = cl;

    return (ULONG)obj;
}

/*  static ULONG */
/*  mDispose(struct IClass *cl, Object *obj, Msg msg) */
/*  { */
/*      struct MUI_SliderData *data = INST_DATA(cl, obj); */
/*      Object *cont; */

/*  g_print("slider dispose\n"); */
/*      return DoSuperMethodA(cl, obj, msg); */
/*  } */

/**************************************************************************
 MUIM_Setup
**************************************************************************/
static ULONG Slider_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_SliderData *data = INST_DATA(cl, obj);

    if (!DoSuperMethodA(cl,obj,(Msg)msg))
	return FALSE;

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, &data->ehn);

    return TRUE;
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
static ULONG Slider_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_SliderData *data = INST_DATA(cl, obj);

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->ehn);
    return DoSuperMethodA(cl,obj,(Msg)msg);
}

/**************************************************************************
 MUIM_AskMinMax
**************************************************************************/
static ULONG Slider_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct MUI_SliderData *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg)msg);

    if (data->flags & SLIDER_HORIZ)
    {
	msg->MinMaxInfo->MinWidth  += 24;
	msg->MinMaxInfo->MinHeight += 12;
	msg->MinMaxInfo->DefWidth  += 42;
	msg->MinMaxInfo->DefHeight += 12;
	msg->MinMaxInfo->MaxWidth   = MUI_MAXMAX;
	msg->MinMaxInfo->MaxHeight += 12;
    }
    else
    {
	msg->MinMaxInfo->MinWidth  += 12;
	msg->MinMaxInfo->MinHeight += 24;
	msg->MinMaxInfo->DefWidth  += 12;
	msg->MinMaxInfo->DefHeight += 42;
	msg->MinMaxInfo->MaxWidth  += 12;
	msg->MinMaxInfo->MaxHeight  = MUI_MAXMAX;
    }

    return TRUE;
}

/**************************************************************************
 MUIM_Draw
**************************************************************************/
static ULONG Slider_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct MUI_SliderData *data = INST_DATA(cl, obj);
    return TRUE;
}

/**************************************************************************
 MUIM_HandleEvent
**************************************************************************/
static ULONG Slider_HandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
    struct MUI_SliderData *data = INST_DATA(cl, obj);

    if (msg->imsg)
    {
	switch (msg->imsg->Class)
	{
	}
    }

    return 0;
}

#ifndef _AROS
__asm IPTR Slider_Dispatcher(register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR, Slider_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Slider_New(cl, obj, (struct opSet *)msg);
	case MUIM_Setup: return Slider_Setup(cl, obj, (APTR)msg);
	case MUIM_Cleanup: return Slider_Cleanup(cl, obj, (APTR)msg);
	case MUIM_AskMinMax: return Slider_AskMinMax(cl, obj, (APTR)msg);
	case MUIM_Draw: return Slider_Draw(cl, obj, (APTR)msg);
	case MUIM_HandleEvent: return Slider_HandleEvent(cl, obj, (APTR)msg);
    }
    return DoSuperMethodA(cl, obj, msg);
}


/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Slider_desc = { 
    MUIC_Slider, 
    MUIC_Numeric, 
    sizeof(struct MUI_SliderData), 
    Slider_Dispatcher 
};
