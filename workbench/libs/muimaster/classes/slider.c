/*
    Copyright � 1999, David Le Corfec.
    Copyright � 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <string.h>

#include <clib/alib_protos.h>
#include <clib/macros.h>

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

struct MUI_SliderData
{
    ULONG flags;
    struct MUI_EventHandlerNode ehn;
    struct ZuneFrameGfx *knob_frame;
    LONG knob_width;
    LONG knob_height;
    LONG knob_pos;
    LONG knob_click;
    LONG state; /* When using mouse */
};


enum slider_flags {
    SLIDER_HORIZ = (1<<0),
    SLIDER_QUIET = (1<<1),
};


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
    ULONG flags = SLIDER_HORIZ;

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

/**************************************************************************
 MUIM_Setup
**************************************************************************/
static ULONG Slider_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_SliderData *data = INST_DATA(cl, obj);
    LONG min;
    LONG max;
    LONG val;
    LONG width;
    struct RastPort rp;

    if (!DoSuperMethodA(cl,obj,(Msg)msg))
	return FALSE;

    data->knob_frame = zune_zframe_get(&muiGlobalInfo(obj)->mgi_Prefs->frames[MUIV_Frame_Knob]);

    InitRastPort(&rp);
    SetFont(&rp,_font(obj));

    width = 0;

    get(obj,MUIA_Numeric_Min,&min);
    get(obj,MUIA_Numeric_Max,&max);

#   warning this oughta be changed: if max-min is _huge_ it will take a LONG time to complete!

    /* Determine the with of the know */
    for (val=min;val<=max;val++)
    {
	LONG nw;
	char *buf;

	buf = (char*)DoMethod(obj,MUIM_Numeric_Stringify,val);
	nw = TextLength(&rp,buf,strlen(buf));
	if (nw > width) width = nw;
    }
    data->knob_width  = width + 2 * data->knob_frame->xthickness + 2;
    data->knob_height = _font(obj)->tf_YSize + 2 * data->knob_frame->ythickness;

    data->knob_pos    = 0;

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
    LONG min,max;

    DoSuperMethodA(cl, obj, (Msg)msg);

    get(obj,MUIA_Numeric_Min,&min);
    get(obj,MUIA_Numeric_Max,&max);

    if (data->flags & SLIDER_HORIZ)
    {
	msg->MinMaxInfo->MinWidth  += data->knob_width + (max - min);
	msg->MinMaxInfo->MinHeight += data->knob_height;
	msg->MinMaxInfo->DefWidth  += data->knob_width + (max - min) + 20;
	msg->MinMaxInfo->DefHeight += data->knob_height;
	msg->MinMaxInfo->MaxWidth   = MUI_MAXMAX;
	msg->MinMaxInfo->MaxHeight += data->knob_height;
    }
    else
    {
	msg->MinMaxInfo->MinWidth  += data->knob_width;
	msg->MinMaxInfo->MinHeight += data->knob_height + (max - min);
	msg->MinMaxInfo->DefWidth  += data->knob_width;
	msg->MinMaxInfo->DefHeight += data->knob_height + (max - min) + 20;
	msg->MinMaxInfo->MaxWidth  += data->knob_width;
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

    DoSuperMethodA(cl,obj,(Msg)msg);

    if (data->flags & SLIDER_HORIZ)
    {
    	LONG val;
	char *buf;
	int width;

	data->knob_pos = DoMethod(obj,MUIM_Numeric_ValueToScale, 0, _mwidth(obj) - data->knob_width) + _mleft(obj);

	DoMethod(obj,MUIM_DrawBackground,_mleft(obj),_mtop(obj),_mwidth(obj),_mheight(obj));
	data->knob_frame->draw[data->state](muiRenderInfo(obj), data->knob_pos, _mtop(obj), data->knob_width, _mheight(obj));

	get(obj,MUIA_Numeric_Value,&val);

	SetFont(_rp(obj),_font(obj));
	SetABPenDrMd(_rp(obj),_pens(obj)[MPEN_TEXT],_pens(obj)[MPEN_BACKGROUND],JAM1);
	buf = (char*)DoMethod(obj,MUIM_Numeric_Stringify,val);
	width = TextLength(_rp(obj),buf,strlen(buf));

	Move(_rp(obj), data->knob_pos + (data->knob_width - width)/2, _mtop(obj) + _font(obj)->tf_Baseline + data->knob_frame->ythickness);
	Text(_rp(obj),buf,strlen(buf));
    }
    else
    {
    	LONG val;
	char *buf;
	int width;

	data->knob_pos = DoMethod(obj,MUIM_Numeric_ValueToScale, 0, _mheight(obj) - data->knob_height) + _mtop(obj);

	DoMethod(obj,MUIM_DrawBackground,_mleft(obj),_mtop(obj),_mwidth(obj),_mheight(obj));
	data->knob_frame->draw[data->state](muiRenderInfo(obj), _mleft(obj), data->knob_pos, data->knob_width, data->knob_height);

	get(obj,MUIA_Numeric_Value,&val);

	SetFont(_rp(obj),_font(obj));
	SetABPenDrMd(_rp(obj),_pens(obj)[MPEN_TEXT],_pens(obj)[MPEN_BACKGROUND],JAM1);
	buf = (char*)DoMethod(obj,MUIM_Numeric_Stringify,val);
	width = TextLength(_rp(obj),buf,strlen(buf));

	Move(_rp(obj), _mleft(obj) + (data->knob_width - width)/2, data->knob_pos + _font(obj)->tf_Baseline + data->knob_frame->ythickness);
	Text(_rp(obj), buf, strlen(buf));
    }


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
	    case IDCMP_MOUSEBUTTONS:
	        if (msg->imsg->Code == SELECTDOWN)
	        {
	            if (_isinobject(msg->imsg->MouseX, msg->imsg->MouseY))
	            {
		        data->knob_click = (data->flags & SLIDER_HORIZ) ? msg->imsg->MouseX : msg->imsg->MouseY;

		        if
			(
			     ((data->flags & SLIDER_HORIZ) && _between(data->knob_pos, msg->imsg->MouseX, data->knob_pos + data->knob_width  - 1)) ||
			    (!(data->flags & SLIDER_HORIZ) && _between(data->knob_pos, msg->imsg->MouseY, data->knob_pos + data->knob_height - 1))
			)
			{
			    DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->ehn);
			    data->ehn.ehn_Events |= IDCMP_MOUSEMOVE;
			    DoMethod(_win(obj), MUIM_Window_AddEventHandler, &data->ehn);
			    data->state = 1;
			    MUI_Redraw(obj,MADF_DRAWUPDATE);
			}
			else
			{
			    if
			    (
			         ((data->flags & SLIDER_HORIZ) && msg->imsg->MouseX < data->knob_pos) ||
                                (!(data->flags & SLIDER_HORIZ) && msg->imsg->MouseY < data->knob_pos)
			    )
			    {
			        DoMethod(obj, MUIM_Numeric_Decrease, 1);
			    }
			    else
			    {
				DoMethod(obj, MUIM_Numeric_Increase, 1);
			    }
			}
		    }
	        }
		else
   	        {
		    if (data->state)
		    {
			DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->ehn);
			data->ehn.ehn_Events &= ~IDCMP_MOUSEMOVE;
			DoMethod(_win(obj), MUIM_Window_AddEventHandler, &data->ehn);
			data->state = 0;
			MUI_Redraw(obj,MADF_DRAWUPDATE);
	  	    }
		}
		break;

	    case IDCMP_MOUSEMOVE:
	    {
		LONG newval;
		
		if (data->flags & SLIDER_HORIZ)
		    newval =  DoMethod(obj,MUIM_Numeric_ScaleToValue, 0, _mwidth(obj) - data->knob_width, msg->imsg->MouseX - data->knob_click);
		else
		    newval =  DoMethod(obj,MUIM_Numeric_ScaleToValue, 0, _mheight(obj) - data->knob_height, msg->imsg->MouseY - data->knob_click);

		set(obj, MUIA_Numeric_Value, newval);
	    }
	    break;
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
    (void*)Slider_Dispatcher 
};
