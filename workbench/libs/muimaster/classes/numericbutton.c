/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/memory.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/muimaster.h>

#include <string.h>
#include <stdio.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "frame.h"
#include "support_classes.h"
#include "prefs.h"
#include "imspec.h"
#include "debug.h"
#include "numericbutton_private.h"

extern struct Library *MUIMasterBase;

#define longget(obj,attr,var) 	    \
    do      	    	    	    \
    {	    	    	    	    \
    	IPTR _iptr_var = *(var);    \
	get(obj,attr,&_iptr_var);   \
	*var = (LONG)_iptr_var;     \
    } while(0)

IPTR Numericbutton__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    obj = (Object *)DoSuperNewTags
    (
        cl, obj, NULL,
        MUIA_Background, MUII_ButtonBack,
	MUIA_Frame, MUIV_Frame_Button,
        TAG_MORE, (IPTR) msg->ops_AttrList
    );
    
    if (obj)
    {
    	struct Numericbutton_DATA *data = INST_DATA(cl, obj);
	
	data->ehn.ehn_Events = IDCMP_MOUSEBUTTONS;
	data->ehn.ehn_Priority = 0;
	data->ehn.ehn_Flags    = 0;
	data->ehn.ehn_Object   = obj;
	data->ehn.ehn_Class    = cl;
    }
    
    return (IPTR)obj;
}

/**************************************************************************
 OM_SET
**************************************************************************/
static ULONG Numericbutton__OM_SET(struct IClass *cl, Object * obj, struct opSet *msg)
{
    struct Numericbutton_DATA *data = INST_DATA(cl, obj);
    struct TagItem *tags, *tag;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Numeric_Max:
	        if (tag->ti_Data != XGET(obj, MUIA_Numeric_Max))
		    data->needs_to_recalculate_sizes = TRUE;
		break;
	    case MUIA_Numeric_Min:
	        if (tag->ti_Data != XGET(obj, MUIA_Numeric_Min))
		    data->needs_to_recalculate_sizes = TRUE;
		break;
	    case MUIA_Numeric_Format:
	        if (tag->ti_Data != XGET(obj, MUIA_Numeric_Format))
		    data->needs_to_recalculate_sizes = TRUE;
		break;
	}
    }
    
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR Numericbutton__MUIM_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct Numericbutton_DATA *data = INST_DATA(cl, obj);
//  const struct ZuneFrameGfx *knob_frame;
    IPTR    	    	       retval;
    
    retval = DoSuperMethodA(cl, obj, (Msg)msg);
    if (retval)
    {
    	//knob_frame = zune_zframe_get(&muiGlobalInfo(obj)->mgi_Prefs->frames[MUIV_Frame_Knob]);

    	data->knob_bg = zune_imspec_setup(MUII_ButtonBack, muiRenderInfo(obj));

        DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);

	data->needs_to_recalculate_sizes = TRUE;
    }
    
    return retval;
}

IPTR Numericbutton__MUIM_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct Numericbutton_DATA *data = INST_DATA(cl, obj);
    
    if (data->knob_bg)
    {
	zune_imspec_cleanup(data->knob_bg);
	data->knob_bg = NULL;
    }

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/**************************************************************************
 MUIM_AskMinMax
**************************************************************************/
IPTR Numericbutton__MUIM_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct Numericbutton_DATA *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg)msg);

    if (data->needs_to_recalculate_sizes)
    {
        struct RastPort rp;
        LONG min, max, val, width;
	
	InitRastPort(&rp);
	SetFont(&rp,_font(obj));

	width = 0;

	longget(obj, MUIA_Numeric_Min, &min);
	longget(obj, MUIA_Numeric_Max, &max);

	/* Determine the width of the knob */
	for (val=min;val<=max;val++)
	{
	    LONG nw;
	    char *buf;

	    buf = (char*)DoMethod(obj, MUIM_Numeric_Stringify, val);
	    nw  = TextLength(&rp, buf, strlen(buf));
	    
	    if (nw > width)
	        width = nw;
	}
	
    	data->max_text_width = width;
    	data->text_height    = _font(obj)->tf_YSize;
        
	data->needs_to_recalculate_sizes = FALSE;
    }
    
    msg->MinMaxInfo->MinWidth  += data->max_text_width;
    msg->MinMaxInfo->MinHeight += data->text_height;
    msg->MinMaxInfo->DefWidth  += data->max_text_width;
    msg->MinMaxInfo->DefHeight += data->text_height;
    msg->MinMaxInfo->MaxWidth  += data->max_text_width;
    msg->MinMaxInfo->MaxHeight += data->text_height;

    return TRUE;
}

static void DrawKnob(Object *obj, struct Numericbutton_DATA *data, BOOL force)
{
    struct RastPort *rp, *saverp;
    LONG    	     x, val, pixeloffset, textlen, pixellen;
    STRPTR  	     text;
    
    pixeloffset = data->popwin->MouseX - data->pop_innerx - 2 - data->knob_clickoffset_x;
        
    val = DoMethod(obj, MUIM_Numeric_ScaleToValue, 0,
    	    	   data->pop_innerw - data->knob_width, pixeloffset);
    
    data->knob_left = data->pop_innerx + pixeloffset;
    data->knob_top = data->pop_innery;
    data->knob_val = val;

    if (!force && (data->knob_left == data->knob_prev_left))
    {
    	return;
    }
    data->knob_prev_left = data->knob_left;
    
    if (data->knob_left < data->pop_innerx)
    {
    	data->knob_left = data->pop_innerx;
    }
    else if (data->knob_left > data->pop_innerx + data->pop_innerw - data->knob_width)
    {
    	data->knob_left = data->pop_innerx + data->pop_innerw - data->knob_width;
    }
    
    saverp = _rp(obj);
    _rp(obj) = rp = data->popwin->RPort;
    
    SetABPenDrMd(rp, _pens(obj)[MPEN_SHINE], 0, JAM1);
    RectFill(rp, data->knob_left, data->knob_top,
    	    	 data->knob_left, data->knob_top + data->knob_height - 1);
    RectFill(rp, data->knob_left + 1, data->knob_top,
    	    	 data->knob_left + data->knob_width - 1, data->knob_top);  	    	 
    SetAPen(rp, _pens(obj)[MPEN_SHADOW]);
    RectFill(rp, data->knob_left + data->knob_width - 1, data->knob_top + 1,
    	    	 data->knob_left + data->knob_width - 1, data->knob_top + data->knob_height - 1);
    RectFill(rp, data->knob_left + 1, data->knob_top + data->knob_height - 1,
    	    	 data->knob_left + data->knob_width - 2, data->knob_top + data->knob_height - 1);		 

    if (data->knob_bg)
    {
    	#warning "Ugly hack?"
	
	struct IBox old_mad_Box = muiAreaData(obj)->mad_Box;
	
	muiAreaData(obj)->mad_Box.Left   = data->knob_left + 1;
	muiAreaData(obj)->mad_Box.Top    = data->knob_top +  1;
	muiAreaData(obj)->mad_Box.Width  = data->knob_width - 2;
	muiAreaData(obj)->mad_Box.Height = data->knob_height - 2;
	
    	zune_imspec_draw(data->knob_bg, muiRenderInfo(obj),
	    	    	 data->knob_left + 1,
			 data->knob_top + 1,
			 data->knob_width - 2,
			 data->knob_height - 2,
			 0,
			 0,
			 0);
			 
	muiAreaData(obj)->mad_Box = old_mad_Box;
    }
    else
    {
	SetAPen(rp, _pens(obj)[MPEN_BACKGROUND]);
	RectFill(rp, data->knob_left + 1, data->knob_top + 1,
    	    	     data->knob_left + data->knob_width - 2,
		     data->knob_top + data->knob_height - 2);
    }    	    	

    SetFont(rp, _font(obj));

    text = (STRPTR)DoMethod(obj, MUIM_Numeric_Stringify, val);
    textlen = strlen(text);
    pixellen = TextLength(_rp(obj), text, textlen);

    SetAPen(rp, _pens(obj)[MPEN_TEXT]);
    Move(rp, data->knob_left + 2 + (data->knob_width - 4 - pixellen) / 2,
    	     data->knob_top + 1 + rp->TxBaseline);
    Text(rp, text, textlen);
    
    SetAPen(rp, _pens(obj)[MPEN_BACKGROUND]);
    
    if (data->knob_left - 1 >= data->pop_innerx)
    {
    	RectFill(rp, data->pop_innerx, data->pop_innery,
	    	     data->knob_left - 1, data->pop_innery + data->pop_innerh - 1);
    }

    x = data->knob_left + data->knob_width;
    if (x <= data->pop_innerx + data->pop_innerw - 1)
    {
    	RectFill(rp, x, data->pop_innery,
	    	     data->pop_innerx + data->pop_innerw - 1,
		     data->pop_innery + data->pop_innerh - 1);   	
    }
                		  
    _rp(obj) = saverp;
}

static void KillPopupWin(Object *obj, struct Numericbutton_DATA *data)
{
    if (data->popwin)
    {
    	CloseWindow(data->popwin);
    	data->popwin = NULL;
    }
    
    if (data->ehn.ehn_Events & IDCMP_MOUSEMOVE)
    {
    	DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
	data->ehn.ehn_Events &= ~IDCMP_MOUSEMOVE;
	DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);    	
    }
    
}

static BOOL MakePopupWin(Object *obj, struct Numericbutton_DATA *data)
{
    const struct ZuneFrameGfx 	*zframe;
    struct RastPort 	    	*rp, *saverp;
    LONG    	    	    	 winx, winy, winw, winh;
    LONG    	    	    	 framew, frameh;
    LONG    	    	    	 min, max;
    
    zframe = zune_zframe_get_with_state(&muiGlobalInfo(obj)->mgi_Prefs->frames[MUIV_Frame_Slider],
    	    	    	    	    	muiGlobalInfo(obj)->mgi_Prefs->frames[MUIV_Frame_Slider].state);
    
    data->pop_innerx = zframe->ileft;			
    data->pop_innery = zframe->itop;
    
    data->knob_width = data->max_text_width + 4;
    data->knob_height = data->text_height + 2;
    
    framew = data->pop_innerx + zframe->iright;
    frameh = data->pop_innery + zframe->ibottom;

    longget(obj, MUIA_Numeric_Min, &min);
    longget(obj, MUIA_Numeric_Max, &max);
    
    winw = max - min + data->knob_width + framew;
    winh = data->knob_height + frameh;
    
    if (winw > _screen(obj)->Width)
    {
    	winw = _screen(obj)->Width;
    }
    
    if ((winw < data->knob_width + framew) || (winh > _screen(obj)->Height))
    {
    	return FALSE;
    }

    data->pop_innerw = winw - framew;
    data->pop_innerh = winh - frameh;

    data->knob_left = DoMethod(obj, MUIM_Numeric_ValueToScale, 0, data->pop_innerw - data->knob_width);
    
    winx = _window(obj)->LeftEdge + _mleft(obj) - 
 	   data->pop_innerx - 2 -
	   data->knob_left;
    winy = _window(obj)->TopEdge + _mtop(obj) - 1-
	   data->pop_innery;
    
    data->popwin = OpenWindowTags(NULL, WA_CustomScreen, (IPTR)_screen(obj),
    	    	    	    	    	WA_Left, winx,
					WA_Top, winy,
					WA_Width, winw,
					WA_Height, winh,
					WA_AutoAdjust, TRUE,
					WA_Borderless, TRUE,
					WA_Activate, FALSE,
					WA_BackFill, (IPTR)LAYERS_NOBACKFILL,
					TAG_DONE);
					
    if (!data->popwin)
    {
    	return FALSE;
    }
    
    rp = data->popwin->RPort;

    saverp = _rp(obj);
    _rp(obj) = rp;
    zframe->draw(muiRenderInfo(obj), 0, 0, winw, winh);    

    DrawKnob(obj, data, TRUE);
    

    _rp(obj) = saverp;
        
    return TRUE;				    
    	    	    	    	    	
}

/**************************************************************************
 MUIM_Show
**************************************************************************/
IPTR Numericbutton__MUIM_Show(struct IClass *cl, Object *obj, struct MUIP_Show *msg)
{
    struct Numericbutton_DATA *data = INST_DATA(cl, obj);
    IPTR    	    	       retval;
    
    retval = DoSuperMethodA(cl, obj, (Msg)msg);
    
    if (data->knob_bg)
	zune_imspec_show(data->knob_bg, obj);
	
    return retval;
}

/**************************************************************************
 MUIM_Hide
**************************************************************************/
IPTR Numericbutton__MUIM_Hide(struct IClass *cl, Object *obj, struct MUIP_Hide *msg)
{
    struct Numericbutton_DATA *data = INST_DATA(cl, obj);

    if (data->popwin)
    {
    	KillPopupWin(obj, data);
    }

    if (data->knob_bg)
	zune_imspec_hide(data->knob_bg);
        
    return DoSuperMethodA(cl, obj, (Msg)msg);
}


/**************************************************************************
 MUIM_Draw
**************************************************************************/
IPTR Numericbutton__MUIM_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    IPTR  val;
    char *buf;
    int   width;

    DoSuperMethodA(cl,obj,(Msg)msg);

    if (!(msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE)))
        return FALSE;

    DoMethod(obj,MUIM_DrawBackground,_mleft(obj),_mtop(obj),_mwidth(obj),_mheight(obj),
    	    	 _mleft(obj),_mtop(obj),0);

    SetFont(_rp(obj),_font(obj));
    SetABPenDrMd(_rp(obj),_pens(obj)[MPEN_TEXT],_pens(obj)[MPEN_BACKGROUND],JAM1);

    get(obj, MUIA_Numeric_Value, &val);
    buf = (char*)DoMethod(obj,MUIM_Numeric_Stringify,val);
    width = TextLength(_rp(obj),buf,strlen(buf));
    
    Move(_rp(obj), _mleft(obj) + (_mwidth(obj) - width) / 2,	 
	    	   _mtop(obj) + _font(obj)->tf_Baseline);
	 
    Text(_rp(obj), buf, strlen(buf));

    return TRUE;
}

/**************************************************************************
 MUIM_HandleEvent
**************************************************************************/
IPTR Numericbutton__MUIM_HandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
    struct Numericbutton_DATA *data = INST_DATA(cl, obj);

    if (!msg->imsg)
    {
    	return 0;
    }
    
    switch(msg->imsg->Class)
    {
    	case IDCMP_MOUSEBUTTONS:
	    switch(msg->imsg->Code)
	    {
	    	case SELECTDOWN:		
		    if (_between(_left(obj), msg->imsg->MouseX, _right(obj)) &&
	        	_between(_top(obj), msg->imsg->MouseY, _bottom(obj)) &&
			(muiAreaData(obj)->mad_Flags & MADF_CANDRAW) &&
			!data->popwin)
		    {
		    	data->knob_clickoffset_x = msg->imsg->MouseX - _mleft(obj);
			
		     	if (MakePopupWin(obj, data))
		     	{
    			    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
			    data->ehn.ehn_Events |= IDCMP_MOUSEMOVE;
			    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
			    
			    return 0;    	
		     	}
		    }
		    break;
		   
		case SELECTUP:
		case MENUUP:
		case MIDDLEUP:
		default:
		    if (data->popwin)
		    {
		    	KillPopupWin(obj, data);
			if ((msg->imsg->Code == SELECTUP))
			{
			    set(obj, MUIA_Numeric_Value, data->knob_val);
			}
		    	return 0;
		    }
		    break;
		    
		    
	    } /* switch(msg->imsg->Code) */
	    break;
	    
	case IDCMP_MOUSEMOVE:
	    if (data->popwin)
	    {
		DrawKnob(obj, data, FALSE);
		
	    	return 0;
	    }
	    break;
	    
    } /* switch(msg->imsg->Class) */
       
    return 0;
}


#if ZUNE_BUILTIN_NUMERICBUTTON
BOOPSI_DISPATCHER(IPTR, Numericbutton_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Numericbutton__OM_NEW(cl, obj, (struct opSet *)msg);
	case OM_SET: return Numericbutton__OM_SET(cl, obj, (struct opSet *)msg);
	
	case MUIM_Setup: return Numericbutton__MUIM_Setup(cl, obj, (struct MUIP_Setup *)msg);
	case MUIM_Cleanup: return Numericbutton__MUIM_Cleanup(cl, obj, (struct MUIP_Cleanup *)msg);		
    	case MUIM_Show: return Numericbutton__MUIM_Show(cl, obj, (struct MUIP_Show *)msg);
    	case MUIM_Hide: return Numericbutton__MUIM_Hide(cl, obj, (struct MUIP_Hide *)msg);
	case MUIM_AskMinMax: return Numericbutton__MUIM_AskMinMax(cl, obj, (struct MUIP_AskMinMax *)msg);
	case MUIM_Draw: return Numericbutton__MUIM_Draw(cl, obj, (struct MUIP_Draw *)msg);
	case MUIM_HandleEvent: return Numericbutton__MUIM_HandleEvent(cl, obj, (struct MUIP_HandleEvent *)msg);
        default: return DoSuperMethodA(cl, obj, msg);
    }
}

const struct __MUIBuiltinClass _MUI_Numericbutton_desc =
{ 
    MUIC_Numericbutton, 
    MUIC_Numeric, 
    sizeof(struct Numericbutton_DATA), 
    (void*)Numericbutton_Dispatcher 
};
#endif /* ZUNE_BUILTIN_NUMERICBUTTON */
