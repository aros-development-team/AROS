#include <exec/memory.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/graphics.h>

#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

extern struct Library *MUIMasterBase;

struct MUI_VirtgroupData
{
    struct MUI_EventHandlerNode   ehn;
};

/**************************************************************************
 OM_NEW
**************************************************************************/
static ULONG Virtgroup_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_VirtgroupData *data;
    int i;

    obj = (Object *)DoSuperNew(cl, obj, TAG_MORE, msg->ops_AttrList);
    if (!obj) return NULL;

    data = INST_DATA(cl, obj);

    data->ehn.ehn_Events   = IDCMP_MOUSEBUTTONS;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags    = 0;
    data->ehn.ehn_Object   = obj;
    data->ehn.ehn_Class    = cl;
    
    return (ULONG)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
static ULONG Virtgroup_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_VirtgroupData *data = INST_DATA(cl, obj);
    return DoSuperMethodA(cl, obj, msg);
}

/**************************************************************************
 OM_SET
**************************************************************************/
static IPTR Virtgroup_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_VirtgroupData *data = INST_DATA(cl, obj);
    IPTR    	    	     retval;
    
    retval = DoSuperMethodA(cl, obj, (Msg)msg);
    
    return retval;
}

/**************************************************************************
 MUIM_AskMinMax
**************************************************************************/
static ULONG Virtgroup_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
#if 0
    struct MUI_VirtgroupData *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg)msg);
    
    if (data->width > msg->MinMaxInfo->MinWidth)
    	msg->MinMaxInfo->MinWidth = data->width;
	
    if (data->width > msg->MinMaxInfo->MaxWidth)
    	msg->MinMaxInfo->MaxWidth = data->width;
	
    if (data->width > msg->MinMaxInfo->DefWidth)
    	msg->MinMaxInfo->DefWidth = data->width;
	
    msg->MinMaxInfo->MinHeight += data->height;
    msg->MinMaxInfo->MaxHeight += data->height;
    msg->MinMaxInfo->DefHeight += data->height;
#endif

    DoSuperMethodA(cl, obj, (Msg)msg);
    return TRUE;
}

/**************************************************************************
 MUIM_Layout
**************************************************************************/
static ULONG Virtgroup_Layout(struct IClass *cl, Object *obj, struct MUIP_Layout *msg)
{
#if 0
    struct MUI_VirtgroupData *data = INST_DATA(cl, obj);
    ULONG retval;
    
    DoSuperMethodA(cl, obj, (Msg)msg);
    
    data->left        = _left(obj);
    data->top         = _top(obj);
    data->framewidth  = _width(obj);
    data->frameheight = _height(obj) - data->height;
#endif

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/**************************************************************************
 MUIM_Draw
**************************************************************************/
static ULONG Virtgroup_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct MUI_VirtgroupData *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl,obj,(Msg)msg);
    return TRUE;
}

/**************************************************************************
 MUIM_HandleEvent
**************************************************************************/
static ULONG Virtgroup_HandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
#if 0
    struct MUI_VirtgroupData *data = INST_DATA(cl, obj);
    WORD i, x, y;
    
    if (msg->imsg)
    {
	if ((msg->imsg->Class == IDCMP_MOUSEBUTTONS) &&
	    (msg->imsg->Code == SELECTDOWN))
	{
	    x = msg->imsg->MouseX - data->left;
	    y = msg->imsg->MouseY - data->top;

	    if (_between(0, x, data->width) &&
		_between(0, y, data->height))
	    {
		for(i = 0; i < data->numitems; i++)
		{
		    if (_between(data->items[i].x1, x, data->items[i].x2) &&
			_between(data->items[i].y1, y, data->items[i].y2))
		    {
			if (data->active != i)
			{
			    set(obj, MUIA_Group_ActivePage, i);
			}
			break;
		    }

		}
	    }
	    	
	}
    }
#endif

    return 0;
}

#ifndef _AROS
__asm IPTR Virtgroup_Dispatcher( register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR,Virtgroup_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Virtgroup_New(cl, obj, (struct opSet *)msg);
	case OM_DISPOSE: return Virtgroup_Dispose(cl, obj, msg);
	case OM_SET: return Virtgroup_Set(cl, obj, (struct opSet *)msg);
	case MUIM_AskMinMax: return Virtgroup_AskMinMax(cl, obj, (struct MUIP_AskMinMax *)msg);
    	case MUIM_Layout: return Virtgroup_Layout(cl, obj, (struct MUIP_Layout *)msg);
	case MUIM_Draw: return Virtgroup_Draw(cl, obj, (struct MUIP_Draw *)msg);
	case MUIM_HandleEvent: return Virtgroup_HandleEvent(cl, obj, (struct MUIP_HandleEvent *)msg);
    }
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Virtgroup_desc = { 
    MUIC_Virtgroup, 
    MUIC_Group, 
    sizeof(struct MUI_VirtgroupData), 
    Virtgroup_Dispatcher 
};
