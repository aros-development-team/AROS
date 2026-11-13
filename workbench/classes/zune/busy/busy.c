/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <utility/date.h>

#include <aros/debug.h>
#include <aros/asmcall.h>

#include <proto/alib.h>
#include <proto/muimaster.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/timer.h>

#include <string.h>
#include <stdio.h>

#include "Busy_mcc.h"
#include "busy_private.h"

/****** Busy.mcc/MUIC_Busy ************
*
*   NAME
*       MUIC_Busy
*
*   SUPERCLASS
*       Area.mui
*
*   DESCRIPTION
*       Show animated stripes to indicate activity, not progress. Move advances
*       the pattern; an Application timer normally calls it every 50 ms.
*
*   NOTES
*       By default the timer is active from Setup through Cleanup, including
*       while hidden. ShowHideIH ties it to Show/Hide instead. Constructor Speed
*       tags are ignored; set Speed after construction to change the interval.
*
*   BUGS
*       The Show path may register a timer even when superclass Show fails.
*       Complete the matching lifecycle before disposal.
*
*/
/****** Busy.mcc/MUIA_Busy_ShowHideIH ************
*
*   NAME
*       MUIA_Busy_ShowHideIH
*
*   FUNCTION
*       Select when the timer is registered.
*
*   INPUTS
*       Initialize only; BOOL, default FALSE.
*
*   NOTES
*       FALSE: Setup/Cleanup. TRUE: Show/Hide. Manual Move works in both
*       modes and when Speed is Off.
*
*/
/****** Busy.mcc/MUIA_Busy_Speed ************
*
*   NAME
*       MUIA_Busy_Speed
*
*   FUNCTION
*       Control the automatic Move interval.
*
*   INPUTS
*       Set/get; UWORD milliseconds, default 50. Constructor tags are ignored.
*       MUIV_Busy_Speed_Off (0) disables the timer; Speed_User (-1) uses 50 ms.
*
*   NOTES
*       Explicit intervals are 1..65535 ms. Get returns the resolved value.
*       Changing Speed re-registers an active timer but does not redraw or reset
*       the stripe phase. Timer scheduling depends on Application processing.
*
*/
/****** Busy.mcc/MUIM_Busy_Move ************
*
*   NAME
*       MUIM_Busy_Move
*
*   SYNOPSIS
*       DoMethod(obj, MUIM_Busy_Move);
*
*   FUNCTION
*       Advance the stripe pattern and request redraw.
*
*   RESULT
*       Zero. The method also works manually while hidden or Speed_Off.
*
*/

/*** Methods ****************************************************************/
/* Create the timer node with a fixed 50 ms default; ShowHideIH is the only constructor tag processed. */
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

/* Update Speed and re-register an active timer; return the superclass result. */
IPTR Busy__OM_SET(Class *cl, Object *obj, struct opSet *msg)
{
    struct Busy_DATA *data = INST_DATA(cl, obj);
    struct TagItem       *tags  = msg->ops_AttrList;
    struct TagItem       *tag;
    BOOL                  redraw = FALSE;
        
    while ((tag = NextTagItem(&tags)) != NULL)
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


/* Return the stored Speed interval, or delegate unknown attributes. */
IPTR Busy__OM_GET(Class *cl, Object *obj, struct opGet *msg)
{
    struct Busy_DATA *data = INST_DATA(cl, obj);
    IPTR              retval = TRUE;
    
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


/* Register the timer in default lifecycle mode after successful superclass Setup. */
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


/* Remove the default-mode timer; ShowHideIH requires Hide to have removed its timer. */
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

/* In ShowHideIH mode register the timer even if superclass Show reports failure. */
IPTR Busy__MUIM_Show(Class *cl, Object *obj, struct MUIP_Show *msg)
{
    struct Busy_DATA *data = INST_DATA(cl, obj);
    IPTR              retval;
    
    retval = DoSuperMethodA(cl, obj, (Msg)msg);

    if (data->speed && (data->flags & FLG_SHOWHIDEIH))
    {
        data->ihn.ihn_Millis = data->speed;
        DoMethod(_app(obj), MUIM_Application_AddInputHandler, (IPTR) &data->ihn);
    }
    
    data->flags |= FLG_SHOWN;
    
    return retval;
}


/* Remove the ShowHideIH timer; the default-mode timer remains active. */
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

/* Add 50 by 4 pixels to the superclass minimum and default dimensions. */
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


/* Draw alternating eight-pixel stripes at the current phase. */
IPTR Busy__MUIM_Draw(Class *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct Busy_DATA *data = INST_DATA(cl, obj);
    WORD              x;
    IPTR              retval;
    
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


/* Increment the UWORD phase and request an update redraw, regardless of Speed. */
IPTR Busy__MUIM_Busy_Move(Class *cl, Object *obj, Msg msg)
{
    struct Busy_DATA *data;
    
    data = INST_DATA(cl, obj);
    data->pos++;

    MUI_Redraw(obj, MADF_DRAWUPDATE);
    
    return 0;
}

