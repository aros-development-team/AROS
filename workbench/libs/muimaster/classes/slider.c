/*
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
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

/*  #define MYDEBUG 1 */
#include "debug.h"
#include "support.h"
#include "mui.h"
#include "muimaster_intern.h"
#include "prefs.h"
#include "imspec.h"

extern struct Library *MUIMasterBase;

struct MUI_SliderData
{
    ULONG flags;
    struct MUI_EventHandlerNode ehn;
    struct ZuneFrameGfx *knob_frame;
    struct MUI_ImageSpec_intern *knob_bg;
    LONG knob_left;
    LONG knob_top;
    LONG knob_width;
    LONG knob_height;
    LONG knob_click;
    LONG last_val;
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
	MUIA_Font, MUIV_Font_Knob,
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
    data->knob_bg = zune_imspec_setup(MUII_SliderKnob, muiRenderInfo(obj));

    InitRastPort(&rp);
    SetFont(&rp,_font(obj));

    width = 0;

    get(obj,MUIA_Numeric_Min,&min);
    get(obj,MUIA_Numeric_Max,&max);

    if ((max - min) > MUI_MAXMAX)
    {
	min = 0;
	max = MUI_MAXMAX;
    }

    /* Determine the width of the knob */
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

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
#ifdef __AROS__
    DeinitRastPort(&rp);
#endif
    return TRUE;
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
static ULONG Slider_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_SliderData *data = INST_DATA(cl, obj);

    if (data->knob_bg)
    {
	zune_imspec_cleanup(data->knob_bg);
	data->knob_bg = NULL;
    }
    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
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
	msg->MinMaxInfo->MinWidth  += data->knob_width + 1;
	msg->MinMaxInfo->MinHeight += data->knob_height;
	msg->MinMaxInfo->DefWidth  += data->knob_width + (max - min) + 20;
	msg->MinMaxInfo->DefHeight += data->knob_height;
	msg->MinMaxInfo->MaxWidth   = MUI_MAXMAX;
	msg->MinMaxInfo->MaxHeight += data->knob_height;
    }
    else
    {
	msg->MinMaxInfo->MinWidth  += data->knob_width;
	msg->MinMaxInfo->MinHeight += data->knob_height + 1;
	msg->MinMaxInfo->DefWidth  += data->knob_width;
	msg->MinMaxInfo->DefHeight += data->knob_height + (max - min) + 20;
	msg->MinMaxInfo->MaxWidth  += data->knob_width;
	msg->MinMaxInfo->MaxHeight  = MUI_MAXMAX;
    }

    return TRUE;
}

/**************************************************************************
 MUIM_Show
**************************************************************************/
static IPTR Slider_Show(struct IClass *cl, Object *obj, struct MUIP_Show *msg)
{
    struct MUI_SliderData *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl,obj,(Msg)msg);
    if (data->knob_bg)
	zune_imspec_show(data->knob_bg, obj);
    return 1;
}

/**************************************************************************
 MUIM_Hide
**************************************************************************/
static IPTR Slider_Hide(struct IClass *cl, Object *obj,struct MUIP_Hide *msg)
{
    struct MUI_SliderData *data = INST_DATA(cl, obj);

    if (data->knob_bg)
	zune_imspec_hide(data->knob_bg);
    return DoSuperMethodA(cl,obj,(Msg)msg);
}


/**************************************************************************
 MUIM_Draw
**************************************************************************/
static ULONG Slider_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    LONG val;
    char *buf;
    int width;

    struct MUI_SliderData *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl,obj,(Msg)msg);

    if (!(msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE)))
        return FALSE;

    if (data->flags & SLIDER_HORIZ)
    {
	data->knob_top  = _mtop(obj);
        data->knob_left = DoSuperMethod(cl, obj,MUIM_Numeric_ValueToScale, 0, _mwidth(obj) - data->knob_width) + _mleft(obj);
    }
    else
    {
        data->knob_top  = (_mheight(obj) - data->knob_height - DoSuperMethod(cl, obj,MUIM_Numeric_ValueToScale, 0, _mheight(obj) - data->knob_height)) + _mtop(obj);
	data->knob_left = _mleft(obj);
    }

    DoMethod(obj,MUIM_DrawBackground,_mleft(obj),_mtop(obj),_mwidth(obj),_mheight(obj));

    zune_imspec_draw(data->knob_bg, muiRenderInfo(obj),
		     data->knob_left, data->knob_top, data->knob_width, data->knob_height,
		     0, 0, 0);

    data->knob_frame->draw[data->state](muiRenderInfo(obj), data->knob_left, data->knob_top, data->knob_width, data->knob_height);

    get(obj, MUIA_Numeric_Value, &val);

    SetFont(_rp(obj),_font(obj));
    SetABPenDrMd(_rp(obj),_pens(obj)[MPEN_TEXT],_pens(obj)[MPEN_BACKGROUND],JAM1);
    buf = (char*)DoMethod(obj,MUIM_Numeric_Stringify,val);
    width = TextLength(_rp(obj),buf,strlen(buf));
    
    Move(_rp(obj), data->knob_left + (data->knob_width - width)/2, data->knob_top + _font(obj)->tf_Baseline + data->knob_frame->ythickness);
    Text(_rp(obj), buf, strlen(buf));

    return TRUE;
}

/**************************************************************************
 MUIM_HandleEvent
**************************************************************************/
static ULONG Slider_HandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
    struct MUI_SliderData *data = INST_DATA(cl, obj);

    if (!msg->imsg)
	return 0;
    switch (msg->imsg->Class)
    {
	case IDCMP_MOUSEBUTTONS:
	    if (msg->imsg->Code == SELECTDOWN)
	    {
		if (_isinobject(msg->imsg->MouseX, msg->imsg->MouseY))
		{
		    if (data->flags & SLIDER_HORIZ)
		    {
			data->knob_click = msg->imsg->MouseX - data->knob_left + _mleft(obj);
		    }
		    else
		    {
			data->knob_click = msg->imsg->MouseY - data->knob_top + _mtop(obj) ;
			D(bug("%p: Y=%ld, mtop=%ld mheight=%ld ktop=%ld kheight=%ld knob_click=%ld\n",
			      obj, msg->imsg->MouseY, _mtop(obj), _mheight(obj),
			      data->knob_top, data->knob_height, data->knob_click));
		    }
		        
		    if (_between(data->knob_left, msg->imsg->MouseX, data->knob_left + data->knob_width)
			&& _between(data->knob_top,  msg->imsg->MouseY, data->knob_top  + data->knob_height))
		    {
			DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
			data->ehn.ehn_Events |= IDCMP_MOUSEMOVE;
			DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
			data->state = 1;
			MUI_Redraw(obj,MADF_DRAWUPDATE);
		    }
		    else if (((data->flags & SLIDER_HORIZ)
			      && msg->imsg->MouseX < data->knob_left)
			     || (!(data->flags & SLIDER_HORIZ)
				 && msg->imsg->MouseY > data->knob_top + data->knob_height))
		    {
			DoSuperMethod(cl, obj, MUIM_Numeric_Decrease, 1);
		    }
		    else
		    {
			DoSuperMethod(cl, obj, MUIM_Numeric_Increase, 1);
		    }
		}
	    }
	    else /* msg->imsg->Code != SELECTDOWN */
	    {
		if (data->state)
		{
		    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
		    data->ehn.ehn_Events &= ~IDCMP_MOUSEMOVE;
		    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
		    data->state = 0;
		    MUI_Redraw(obj,MADF_DRAWUPDATE);
		}
	    } /* if (msg->imsg->Code == SELECTDOWN) */
	    break;

	case IDCMP_MOUSEMOVE:
	{
	    LONG oldval;
	    LONG newval;

	    if (data->flags & SLIDER_HORIZ)
		newval =  DoSuperMethod(cl, obj, MUIM_Numeric_ScaleToValue,
					0, _mwidth(obj) - data->knob_width,
					msg->imsg->MouseX - data->knob_click);
	    else
	    {
		LONG scale;

		scale = _mheight(obj) - data->knob_height + data->knob_click - msg->imsg->MouseY;
		newval =  DoSuperMethod(cl, obj, MUIM_Numeric_ScaleToValue,
					0, _mheight(obj) - data->knob_height,
					scale);
		D(bug("%p: Y=%ld scale=%ld val=%ld\n", obj, msg->imsg->MouseY, scale, newval));
	    }

	    get(obj, MUIA_Numeric_Value, oldval);
	    if (oldval != newval)
	    {
		set(obj, MUIA_Numeric_Value, newval);
	    }
	}
	break;
    }

    return 0;
}

BOOPSI_DISPATCHER(IPTR, Slider_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Slider_New(cl, obj, (struct opSet *)msg);
	case MUIM_Setup: return Slider_Setup(cl, obj, (APTR)msg);
	case MUIM_Cleanup: return Slider_Cleanup(cl, obj, (APTR)msg);
	case MUIM_Show: return Slider_Show(cl, obj, (APTR)msg);
	case MUIM_Hide: return Slider_Hide(cl, obj, (APTR)msg);
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
