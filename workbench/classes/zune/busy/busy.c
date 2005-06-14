/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: clock.c 20066 2003-11-08 21:43:55Z stegerg $
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <utility/date.h>

#include <aros/debug.h>
#include <aros/asmcall.h>

#include <proto/muimaster.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/timer.h>

#include <string.h>
#include <stdio.h>

#include "Busy_mcc.h"
#include "busy_private.h"

/*** Methods ****************************************************************/
IPTR Busy__OM_NEW(Class *cl, Object *obj, struct opSet *msg)
{
    struct Busy_DATA *data;
    
    obj = (Object *) DoSuperMethodA(cl, obj, (Msg)msg);	
    if (!obj) return 0;
    
    data = INST_DATA(cl, obj);
    if (GetTagData(MUIA_Busy_ShowHideIH, FALSE, msg->ops_AttrList))
    {
    	data->flags |= FLG_SHOWHIDEIH;
    }
    data->speed = 50;
    
    data->ihn.ihn_Flags  = MUIIHNF_TIMER;
    data->ihn.ihn_Method = MUIM_Busy_Move;
    data->ihn.ihn_Object = obj;
    data->ihn.ihn_Millis = 0;
    
    return (IPTR)obj;
}

IPTR Busy__OM_SET(Class *cl, Object *obj, struct opSet *msg)
{
    struct Busy_DATA *data = INST_DATA(cl, obj);
    struct TagItem   	 *tags  = msg->ops_AttrList;
    struct TagItem   	 *tag;
    BOOL    	      	  redraw = FALSE;
        
    while ((tag = NextTagItem((const struct TagItem **)&tags)) != NULL)
    {
    	switch(tag->ti_Tag)
	{
    	    case MUIA_Busy_Speed:
		if ((LONG)tag->ti_Data == MUIV_Busy_Speed_User)
		{
		    data->speed = 50;
		}
		else
		{
		    data->speed = tag->ti_Data;
		}

	    	if ((data->flags & FLG_SHOWN) ||
		    ((data->flags & FLG_SETUP) && !(data->flags & FLG_SHOWHIDEIH)))
		{
		    if (data->ihn.ihn_Millis)
		    {
    	    	    	DoMethod(_app(obj), MUIM_Application_RemInputHandler, (IPTR) &data->ihn);
		    }
		    
		    data->ihn.ihn_Millis = data->speed;
		    
		    if (data->ihn.ihn_Millis)
		    {
    	    	        DoMethod(_app(obj), MUIM_Application_AddInputHandler, (IPTR) &data->ihn);
		    }		    		    
		}
		break;
		
	} /* switch(tag->ti_Tag) */
	
    } /* while ((tag = NextTagItem(&tags)) != NULL) */
    
    if (redraw)
    {
    	 MUI_Redraw(obj, MADF_DRAWUPDATE);
    }
    
    return DoSuperMethodA(cl, obj, (Msg)msg);
}


IPTR Busy__OM_GET(Class *cl, Object *obj, struct opGet *msg)
{
    struct Busy_DATA *data = INST_DATA(cl, obj);
    IPTR    	      retval = TRUE;
    
    switch(msg->opg_AttrID)
    {
    	case MUIA_Busy_Speed:
	    *msg->opg_Storage = data->speed;
	    break;
	    
	default:
	    retval = DoSuperMethodA(cl, obj, (Msg)msg);
	    break;
    }
    
    return retval;
}


IPTR Busy__MUIM_Setup(Class *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct Busy_DATA *data = INST_DATA(cl, obj);
    
    if (!DoSuperMethodA(cl, obj, (Msg)msg)) return FALSE;

    if (data->speed && !(data->flags & FLG_SHOWHIDEIH))
    {
    	data->ihn.ihn_Millis = data->speed;
        DoMethod(_app(obj), MUIM_Application_AddInputHandler, (IPTR) &data->ihn);
    }
    
    data->flags |= FLG_SETUP;
    
    return TRUE;
}


IPTR Busy__MUIM_Cleanup(Class *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct Busy_DATA *data = INST_DATA(cl, obj);
 
    if (data->ihn.ihn_Millis && !(data->flags & FLG_SHOWHIDEIH))
    {
   	DoMethod(_app(obj), MUIM_Application_RemInputHandler, (IPTR) &data->ihn);
	data->ihn.ihn_Millis = 0;
    }
    
    data->flags &= ~FLG_SETUP;
    
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR Busy__MUIM_Show(Class *cl, Object *obj, struct MUIP_Show *msg)
{
    struct Busy_DATA *data = INST_DATA(cl, obj);
    IPTR    	      retval;
    
    retval = DoSuperMethodA(cl, obj, (Msg)msg);

    if (data->speed && (data->flags & FLG_SHOWHIDEIH))
    {
    	data->ihn.ihn_Millis = data->speed;
        DoMethod(_app(obj), MUIM_Application_AddInputHandler, (IPTR) &data->ihn);
    }
    
    data->flags |= FLG_SHOWN;
    
    return retval;
}


IPTR Busy__MUIM_Hide(Class *cl, Object *obj, struct MUIP_Hide *msg)
{
    struct Busy_DATA *data = INST_DATA(cl, obj);
 
    if (data->ihn.ihn_Millis && (data->flags & FLG_SHOWHIDEIH))
    {
    	DoMethod(_app(obj), MUIM_Application_RemInputHandler, (IPTR) &data->ihn);
	data->ihn.ihn_Millis = 0;
    }
    
    data->flags &= ~FLG_SHOWN;
    
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR Busy__MUIM_AskMinMax(Class *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    DoSuperMethodA(cl, obj, (Msg)msg);
    
    msg->MinMaxInfo->MinWidth  += 50;
    msg->MinMaxInfo->MinHeight += 4;
    msg->MinMaxInfo->DefWidth  += 50;
    msg->MinMaxInfo->DefHeight += 4;
    msg->MinMaxInfo->MaxWidth   = MUI_MAXMAX;
    msg->MinMaxInfo->MaxHeight  = MUI_MAXMAX;

    return TRUE;
}


IPTR Busy__MUIM_Draw(Class *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct Busy_DATA *data = INST_DATA(cl, obj);
    WORD    	      x;
    IPTR    	      retval;
    
    retval = DoSuperMethodA(cl, obj, (Msg)msg);
    
    if (!(msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE))) return 0;
        
    for(x = 0; x < _mwidth(obj); x++)
    {
    	WORD col;
	
    	col = ((x + data->pos) / 8) % 2;
	SetAPen(_rp(obj), _pens(obj)[col ? MPEN_SHADOW : MPEN_SHINE]);
	RectFill(_rp(obj), _mleft(obj) + x, _mtop(obj), _mleft(obj) + x, _mbottom(obj));
    }
    
    return retval;
}


IPTR Busy__MUIM_Busy_Move(Class *cl, Object *obj, Msg msg)
{
    struct Busy_DATA *data;
    
    data = INST_DATA(cl, obj);
    data->pos++;

    MUI_Redraw(obj, MADF_DRAWUPDATE);
    
    return 0;
}

