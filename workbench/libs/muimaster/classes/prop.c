/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/memory.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "prefs.h"

/*  #define MYDEBUG 1 */
#include "debug.h"

extern struct Library *MUIMasterBase;

struct Prop_DATA
{
    ULONG entries;
    LONG first;
    ULONG visible;
    LONG deltafactor;
    LONG gadgetid;

    int horiz;
    int usewinborder;
    Object *prop_object;
    struct MUI_EventHandlerNode ehn;
};


IPTR Prop__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Prop_DATA *data;
    struct TagItem *tags,*tag;

    obj = (Object *)DoSuperNewTags(cl, obj, NULL, TAG_MORE, (IPTR) msg->ops_AttrList);
    if (!obj)
	return FALSE;

    data = INST_DATA(cl, obj);

    data->deltafactor = 1;
    
    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_Prop_Entries:
		    data->entries = tag->ti_Data;
		    break;
	    case    MUIA_Prop_First:
		    data->first = tag->ti_Data;
		    break;
	    case    MUIA_Prop_Horiz:
		    data->horiz = tag->ti_Data;
		    break;
	    case    MUIA_Prop_Slider:
		    break;
	    case    MUIA_Prop_UseWinBorder:
		    data->usewinborder = tag->ti_Data;
		    break;
	    case    MUIA_Prop_Visible:
		    data->visible = tag->ti_Data;
		    break;
		    
	    case    MUIA_Prop_DeltaFactor:
	    	    data->deltafactor = tag->ti_Data;
		    break;
	}
    }

    if (data->first < 0)
	data->first = 0;

    data->ehn.ehn_Events   = IDCMP_IDCMPUPDATE;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags    = 0;
    data->ehn.ehn_Object   = obj;
    data->ehn.ehn_Class    = cl;

    if (data->usewinborder)
	_flags(obj) |= MADF_BORDERGADGET;

    return (ULONG)obj;
}

IPTR Prop__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    //struct Prop_DATA *data = INST_DATA(cl, obj);
    return DoSuperMethodA(cl, obj, msg);
}

IPTR Prop__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct TagItem *tags,*tag;
    struct Prop_DATA *data = INST_DATA(cl, obj);
    int refresh = 0;
    int only_trigger = 0;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_Prop_Entries:
		    data->entries = tag->ti_Data;
		    refresh = 1;
		    break;

	    case    MUIA_Prop_First:
		    data->first = tag->ti_Data;
		    refresh = 1;
		    break;

	    case    MUIA_Prop_Slider:
		    break;

	    case    MUIA_Prop_Visible:
		    data->visible = tag->ti_Data;
		    refresh = 1;
		    break;

	    case    MUIA_Prop_OnlyTrigger:
		    only_trigger = tag->ti_Data;
		    break;
		    
	    case    MUIA_Prop_DeltaFactor:
	    	    data->deltafactor = tag->ti_Data;
		    break;
	}
    }

    if (data->first < 0)
	data->first = 0;

    if (data->prop_object && refresh && !only_trigger)
    {
	/* Rendering will happen here!! This could make problems with virtual groups, forward this to MUIM_Draw??? */
	SetAttrs(data->prop_object, ICA_TARGET, NULL, TAG_DONE);
	if (SetGadgetAttrs((struct Gadget*)data->prop_object,_window(obj),NULL,
		PGA_Top,data->first,
		PGA_Visible,data->visible,
		PGA_Total,data->entries,
		TAG_DONE))
	    RefreshGList((struct Gadget*)data->prop_object, _window(obj), NULL, 1);
	SetAttrs(data->prop_object, ICA_TARGET, ICTARGET_IDCMP, TAG_DONE);
    }

    return DoSuperMethodA(cl,obj,(Msg)msg);
}

#define STORE *(msg->opg_Storage)
IPTR Prop__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Prop_DATA *data = INST_DATA(cl, obj);
    switch (msg->opg_AttrID)
    {
    	case    MUIA_Prop_First:
		{
		    if (data->prop_object)
		    {
			/* So we can get a more current value */
		        GetAttr(PGA_Top,data->prop_object,&data->first);
		    }
		    STORE = data->first;
		    return 1;
		}
    	case    MUIA_Prop_Entries: STORE = data->entries; return 1;
    	case    MUIA_Prop_Visible: STORE = data->visible; return 1;
    	default:
    	        return DoSuperMethodA(cl,obj,(Msg)msg);
    }

    return 1;
}
#undef STORE

IPTR Prop__MUIM_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct Prop_DATA *data = INST_DATA(cl, obj);

    /*
    ** let our superclass first fill in what it thinks about sizes.
    ** this will e.g. add the size of frame and inner spacing.
    */
    DoSuperMethodA(cl, obj, (Msg)msg);

    if (data->horiz)
    {
	msg->MinMaxInfo->MinWidth += 6;
	msg->MinMaxInfo->DefWidth += 50;
	msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;

	msg->MinMaxInfo->MinHeight += 6;
	msg->MinMaxInfo->DefHeight += 6;
	msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;
    }
    else /* vertical */
    {
	msg->MinMaxInfo->MinWidth += 6;
	msg->MinMaxInfo->DefWidth += 6;
	msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;

	msg->MinMaxInfo->MinHeight += 6;
	msg->MinMaxInfo->DefHeight += 50;
	msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;
    }
    D(bug("Prop %p minheigh=%d\n",
	  obj, msg->MinMaxInfo->MinHeight));

   return TRUE;
}

IPTR Prop__MUIM_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct Prop_DATA *data = INST_DATA(cl, obj);
    ULONG rc = DoSuperMethodA(cl, obj, (Msg)msg);
    if (!rc) return 0;

    DoMethod(_win(obj),MUIM_Window_AddEventHandler,(IPTR)&data->ehn);

    if (!data->usewinborder)
    {
	data->gadgetid = DoMethod(_win(obj),MUIM_Window_AllocGadgetID);
    }

    return 1;
}

IPTR Prop__MUIM_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct Prop_DATA *data = INST_DATA(cl, obj);
    if (!data->usewinborder)
    {
	DoMethod(_win(obj),MUIM_Window_FreeGadgetID,data->gadgetid);
    }
    DoMethod(_win(obj),MUIM_Window_RemEventHandler,(IPTR)&data->ehn);
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR Prop__MUIM_Show(struct IClass *cl, Object *obj, struct MUIP_Show *msg)
{
    struct Prop_DATA *data = INST_DATA(cl, obj);
    ULONG rc = DoSuperMethodA(cl, obj, (Msg)msg);

    if (!data->usewinborder)
    {
	BOOL isnewlook, completely_visible = TRUE;;

	if (_flags(obj) & MADF_INVIRTUALGROUP)
	{
    	    Object *wnd, *parent;

    	    get(obj, MUIA_WindowObject,&wnd);
	    parent = obj;
	    while (get(parent,MUIA_Parent,&parent))
	    {
		if (!parent) break;
		if (parent == wnd) break;

		if (_flags(parent) & MADF_ISVIRTUALGROUP)
		{
	    	    if ((_mleft(obj) < _mleft(parent)) ||
			(_mright(obj) > _mright(parent)) ||
			(_mtop(obj) < _mtop(parent)) ||
			(_mbottom(obj) > _mbottom(parent)))
		    {
			completely_visible = FALSE;
			kprintf("=== prop object: completely visible FALSE for obj %x at %d,%d - %d,%d\n",
		    		 obj, _mleft(obj), _mtop(obj), _mright(obj), _mbottom(obj));
			break;
		    }
		}
	    }
	}
    
    	if (completely_visible)
	{
	    if (muiGlobalInfo(obj)->mgi_Prefs->scrollbar_type == SCROLLBAR_TYPE_NEWLOOK)
		isnewlook = TRUE;
	    else
		isnewlook = FALSE;

	    if ((data->prop_object = NewObject(NULL, "propgclass",
			    GA_Left, _mleft(obj),
			    GA_Top, _mtop(obj),
			    GA_Width, _mwidth(obj),
			    GA_Height, _mheight(obj),
			    GA_ID, data->gadgetid,
			    PGA_Freedom, data->horiz?FREEHORIZ:FREEVERT,
			    PGA_Total, data->entries,
			    PGA_Visible, data->visible,
			    PGA_Top, data->first,
    			    PGA_NewLook, isnewlook,
    			    PGA_Borderless, TRUE,
		            ICA_TARGET  , ICTARGET_IDCMP, /* needed for notification */
    			    TAG_DONE)))
	    {
    		AddGadget(_window(obj),(struct Gadget*)data->prop_object,~0);
	    }
	}
    } else
    {
    	switch (data->usewinborder)
    	{
	    case    MUIV_Prop_UseWinBorder_Right:
		    data->prop_object = muiRenderInfo(obj)->mri_VertProp;
		    if (data->prop_object)
		    {
		    	/* Store pointer to this propclass object in propgadget->UserData,
			   so that window class when receiving IDCMP_IDCMUPDATE from
			   arrow gadgets can notify propclass object */
			   
		    	((struct Gadget *)data->prop_object)->UserData = obj;
		    }
		    break;

	    case    MUIV_Prop_UseWinBorder_Bottom:
		    data->prop_object = muiRenderInfo(obj)->mri_HorizProp;
		    if (data->prop_object)
		    {
		    	/* Store pointer to this propclass object in propgadget->UserData,
			   so that window class when receiving IDCMP_IDCMUPDATE from
			   arrow gadgets can notify propclass object */

		    	((struct Gadget *)data->prop_object)->UserData = obj;
		    }
		    break;
    	}
    	if (data->prop_object)
    	{
	    data->gadgetid = ((struct Gadget*)data->prop_object)->GadgetID;

	    SetAttrs(data->prop_object, ICA_TARGET, NULL, TAG_DONE);
	    if (SetGadgetAttrs((struct Gadget*)data->prop_object,_window(obj),NULL,
		PGA_Top,data->first,
		PGA_Visible,data->visible,
		PGA_Total,data->entries,
		TAG_DONE))
	    {
		RefreshGList((struct Gadget*)data->prop_object, _window(obj), NULL, 1);
	    }
	    SetAttrs(data->prop_object, ICA_TARGET, ICTARGET_IDCMP, TAG_DONE);
	}
    }

    return rc;
}

IPTR Prop__MUIM_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct Prop_DATA *data = INST_DATA(cl, obj);

    /* No drawings if own border */
    if (data->usewinborder) return 0;

    DoSuperMethodA(cl, obj, (Msg)msg);

    if (!(msg->flags & MADF_DRAWOBJECT)) return 1;
    if (data->prop_object) RefreshGList((struct Gadget*)data->prop_object, _window(obj), NULL, 1);
    return 1;
}

IPTR Prop__MUIM_Hide(struct IClass *cl, Object *obj, struct MUIP_Hide *msg)
{
    struct Prop_DATA *data = INST_DATA(cl, obj);
    if (data->prop_object)
    {
    	if (!data->usewinborder)
    	{
	    RemoveGadget(_window(obj),(struct Gadget*)data->prop_object);
    	    DisposeObject(data->prop_object);
    	} else
    	{
	    data->gadgetid = 0;
    	}
	data->prop_object = NULL;
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR Prop__MUIM_HandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
    struct Prop_DATA *data = INST_DATA(cl, obj);
    if (msg->imsg)
    {
    	if (msg->imsg->Class == IDCMP_IDCMPUPDATE)
    	{
	    struct TagItem *tag;

	    /* Check if we are meant */
	    tag = FindTagItem(GA_ID,(struct TagItem*)msg->imsg->IAddress);
	    if (!tag) return 0;
	    if (tag->ti_Data != data->gadgetid) return 0;

	    /* Check if we PGA_Top has really changed */
	    tag = FindTagItem(PGA_Top,(struct TagItem*)msg->imsg->IAddress);
	    if (!tag) return 0;
	    if (tag->ti_Data == data->first) return 0;
	    data->first = tag->ti_Data;
	    if (data->first < 0)
		data->first = 0;
	    SetAttrs(obj, MUIA_Prop_First, tag->ti_Data, MUIA_Prop_OnlyTrigger, TRUE, TAG_DONE);
	}
    }

    return 0;
}

IPTR Prop__MUIM_Prop_Increase(struct IClass *cl, Object *obj, struct MUIP_Prop_Increase *msg)
{
    struct Prop_DATA *data = INST_DATA(cl, obj);
    LONG newfirst;

    newfirst = data->first + msg->amount * data->deltafactor;

    if (newfirst + data->visible > data->entries)
	newfirst = data->entries - data->visible;
    if (newfirst != data->first)
	set(obj, MUIA_Prop_First, newfirst);
    return 1;
}

IPTR Prop__MUIM_Prop_Decrease(struct IClass *cl, Object *obj, struct MUIP_Prop_Decrease *msg)
{
    struct Prop_DATA *data = INST_DATA(cl, obj);
    LONG newfirst;

    /* We cannot decrease if if are on the top */
    if (data->first <= 0)
	return 1;

    newfirst = data->first - msg->amount * data->deltafactor;

    if (newfirst < 0)
	set(obj, MUIA_Prop_First, 0);
    else if (newfirst != data->first)
	set(obj, MUIA_Prop_First, newfirst);
    return 1;
}


BOOPSI_DISPATCHER(IPTR, Prop_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW:             return Prop__OM_NEW(cl, obj, (struct opSet *) msg);
	case OM_DISPOSE:         return Prop__OM_DISPOSE(cl, obj, msg);
	case OM_GET:             return Prop__OM_GET(cl, obj, (struct opGet *)msg);
	case OM_SET:             return Prop__OM_SET(cl, obj, (struct opSet *)msg);
	case MUIM_Setup:         return Prop__MUIM_Setup(cl, obj, (APTR)msg);
	case MUIM_Cleanup:       return Prop__MUIM_Cleanup(cl, obj, (APTR)msg);
	case MUIM_Show:          return Prop__MUIM_Show(cl, obj, (APTR)msg);
	case MUIM_Hide:          return Prop__MUIM_Hide(cl, obj, (APTR)msg);
	case MUIM_AskMinMax:     return Prop__MUIM_AskMinMax(cl, obj, (APTR)msg);
	case MUIM_Draw:          return Prop__MUIM_Draw(cl, obj, (APTR)msg);
	case MUIM_HandleEvent:   return Prop__MUIM_HandleEvent(cl, obj, (APTR)msg);
	case MUIM_Prop_Decrease: return Prop__MUIM_Prop_Decrease(cl, obj, (APTR)msg);
	case MUIM_Prop_Increase: return Prop__MUIM_Prop_Increase(cl, obj, (APTR)msg);
        default:                 return DoSuperMethodA(cl, obj, msg);

    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Prop_desc =
{ 
    MUIC_Prop, 
    MUIC_Area, 
    sizeof(struct Prop_DATA), 
    (void*)Prop_Dispatcher 
};
