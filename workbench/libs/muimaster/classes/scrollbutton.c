/*
    Copyright � 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <intuition/intuitionbase.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

extern struct Library *MUIMasterBase;

struct Scrollbutton_Data
{
  WORD mx,my;
  WORD cx,cy;
  ULONG pos;
  Object *horiz_prop;
  Object *vert_prop;
  struct MUI_EventHandlerNode ehn;
};

static ULONG Scrollbutton_New(struct IClass * cl, Object * o, struct opSet * msg)
{
  return (ULONG) DoSuperNew(cl, o,
  	ButtonFrame,
  	MUIA_InputMode, MUIV_InputMode_RelVerify,
  	MUIA_Background, MUII_ButtonBack,
	TAG_MORE, msg->ops_AttrList);
}

static ULONG Scrollbutton_Get(struct IClass * cl, Object * o, struct opGet * msg)
{
    struct Scrollbutton_Data *data = (struct Scrollbutton_Data *) INST_DATA(cl, o);
    switch (msg->opg_AttrID)
    {
	case	MUIA_Scrollbutton_NewPosition:
		*msg->opg_Storage = data->pos;
		return TRUE;

	case	MUIA_Scrollbutton_HorizProp:
		*msg->opg_Storage = (ULONG)data->horiz_prop;
		return TRUE;

	case	MUIA_Scrollbutton_VertProp:
		*msg->opg_Storage = (ULONG)data->horiz_prop;
		return TRUE;

        default:
		return DoSuperMethodA(cl, o, (Msg) msg);
    }
}

static ULONG Scrollbutton_Set(struct IClass * cl, Object * o, struct opSet * msg)
{
    struct Scrollbutton_Data *data = (struct Scrollbutton_Data *) INST_DATA(cl, o);
    struct TagItem *tl = msg->ops_AttrList;
    struct TagItem *ti;

    while ((ti = NextTagItem(&tl)))
    {
	switch (ti->ti_Tag)
	{
	    case    MUIA_Scrollbutton_Horiz:
		    data->cx = ti->ti_Data;
		    break;

	    case    MUIA_Scrollbutton_Vert:
		    data->cy = ti->ti_Data;
		    break;
	}
    }
    return DoSuperMethodA(cl, o, (Msg) msg);
}

static ULONG Scrollbutton_AskMinMax(struct IClass *cl, Object *o, struct MUIP_AskMinMax *msg)
{
    DoSuperMethodA(cl, o, (Msg) msg);

    msg->MinMaxInfo->MinWidth += 2;
    msg->MinMaxInfo->DefWidth += 2;
    msg->MinMaxInfo->MaxWidth += MUI_MAXMAX;

    msg->MinMaxInfo->MinHeight += 2;
    msg->MinMaxInfo->DefHeight += 2;
    msg->MinMaxInfo->MaxHeight += MUI_MAXMAX;
    return 0;
}

static ULONG Scrollbutton_Setup(struct IClass * cl, Object * o, Msg msg)
{
    struct Scrollbutton_Data *data = (struct Scrollbutton_Data *) INST_DATA(cl, o);
    if (!DoSuperMethodA(cl, o, msg))
	return FALSE;

    data->ehn.ehn_Events   = IDCMP_MOUSEBUTTONS;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags    = 0;
    data->ehn.ehn_Object   = o;
    data->ehn.ehn_Class    = cl;

    DoMethod(_win(o), MUIM_Window_AddEventHandler, &data->ehn);

    return TRUE;
}

static ULONG Scrollbutton_Cleanup(struct IClass * cl, Object * o, Msg msg)
{
    struct Scrollbutton_Data *data = (struct Scrollbutton_Data *) INST_DATA(cl, o);
    DoMethod(_win(o), MUIM_Window_RemEventHandler, &data->ehn);
    DoSuperMethodA(cl, o, msg);
    return 0;
}

static ULONG Scrollbutton_HandleEvent(struct IClass * cl, Object * o, struct MUIP_HandleEvent * msg)
{
    struct Scrollbutton_Data *data = (struct Scrollbutton_Data *) INST_DATA(cl, o);
    if (msg->imsg)
    {
	switch (msg->imsg->Class)
	{
	    case    IDCMP_MOUSEBUTTONS:
		    if (msg->imsg->Code == SELECTDOWN)
		    {
			if (msg->imsg->MouseX >= _left(o) && msg->imsg->MouseX <= _right(o) && msg->imsg->MouseY >= _top(o) && msg->imsg->MouseY <= _bottom(o))
			{
			    DoMethod(_win(o), MUIM_Window_RemEventHandler, &data->ehn);
			    data->ehn.ehn_Events |= IDCMP_MOUSEMOVE;
			    DoMethod(_win(o), MUIM_Window_AddEventHandler, &data->ehn);

			    set(o,MUIA_Selected, TRUE);
			    data->mx = msg->imsg->MouseX;
			    data->my = msg->imsg->MouseY;

			}
		    } else
		    {
		    	if (data->ehn.ehn_Events & IDCMP_MOUSEMOVE)
		    	{
			    DoMethod(_win(o), MUIM_Window_RemEventHandler, &data->ehn);
			    data->ehn.ehn_Events &= ~IDCMP_MOUSEMOVE;
			    DoMethod(_win(o), MUIM_Window_AddEventHandler, &data->ehn);

			    set(o,MUIA_Selected, FALSE);
			}
		    }
		    break;

	    case    IDCMP_MOUSEMOVE:
		    {
		    	int sel;
		    	get(o,MUIA_Selected, &sel);
		    	if (sel)
		    	{
			    UWORD x = (msg->imsg->MouseX - data->mx) + data->cx;
			    UWORD y = (msg->imsg->MouseY - data->my) + data->cy;
			    ULONG pos = x << 16 | y;

		            data->pos = pos;
		            set(o,MUIA_Scrollbutton_NewPosition,pos);
		        }
		        break;
		    }
        }
    }
    return 0;
}

#ifndef _AROS
__asm IPTR Scrollbutton_Dispatcher( register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR,Scrollbutton_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif

{
  switch (msg->MethodID)
  {
    case  OM_NEW: return Scrollbutton_New(cl, obj, (struct opSet *) msg);
    case  OM_GET: return Scrollbutton_Get(cl, obj, (struct opGet *) msg);
    case  OM_SET: return Scrollbutton_Set(cl, obj, (struct opSet *) msg);
    case  MUIM_AskMinMax: return Scrollbutton_AskMinMax(cl, obj, (struct MUIP_AskMinMax *) msg);
    case  MUIM_Setup: return Scrollbutton_Setup(cl, obj, msg);
    case  MUIM_Cleanup: return Scrollbutton_Cleanup(cl, obj, msg);
    case  MUIM_HandleEvent: return Scrollbutton_HandleEvent(cl, obj, (struct MUIP_HandleEvent *) msg);
  }
  return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Scrollbutton_desc = { 
    MUIC_Scrollbutton, 
    MUIC_Area, 
    sizeof(struct Scrollbutton_Data),
    (void*)Scrollbutton_Dispatcher 
};
