/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
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

struct MUI_PropData
{
    ULONG entries;
    ULONG first;
    ULONG visible;
    LONG deltafactor;
    LONG gadgetid;

    int horiz;
    int usewinborder;
    Object *prop_object;
    struct MUI_EventHandlerNode ehn;
};

/**************************************************************************
 OM_NEW
**************************************************************************/
static ULONG Prop_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_PropData *data;
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

    data->ehn.ehn_Events   = IDCMP_IDCMPUPDATE;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags    = 0;
    data->ehn.ehn_Object   = obj;
    data->ehn.ehn_Class    = cl;

    if (data->usewinborder)
	_flags(obj) |= MADF_BORDERGADGET;

    return (ULONG)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
static ULONG Prop_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    //struct MUI_PropData *data = INST_DATA(cl, obj);
    return DoSuperMethodA(cl, obj, msg);
}

/**************************************************************************
 OM_SET
**************************************************************************/
static ULONG Prop_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct TagItem *tags,*tag;
    struct MUI_PropData *data = INST_DATA(cl, obj);
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


/**************************************************************************
 OM_GET
**************************************************************************/
#define STORE *(msg->opg_Storage)
static ULONG Prop_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct MUI_PropData *data = INST_DATA(cl, obj);
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

/**************************************************************************
 MUIM_AskMinMax
**************************************************************************/
static ULONG Prop_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct MUI_PropData *data = INST_DATA(cl, obj);

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

/**************************************************************************
 MUIM_Setup
**************************************************************************/
static ULONG Prop_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_PropData *data = INST_DATA(cl, obj);
    ULONG rc = DoSuperMethodA(cl, obj, (Msg)msg);
    if (!rc) return 0;

    DoMethod(_win(obj),MUIM_Window_AddEventHandler,(IPTR)&data->ehn);

    if (!data->usewinborder)
    {
	data->gadgetid = DoMethod(_win(obj),MUIM_Window_AllocGadgetID);
    }

    return 1;
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
static ULONG Prop_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_PropData *data = INST_DATA(cl, obj);
    if (!data->usewinborder)
    {
	DoMethod(_win(obj),MUIM_Window_FreeGadgetID,data->gadgetid);
    }
    DoMethod(_win(obj),MUIM_Window_RemEventHandler,(IPTR)&data->ehn);
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/**************************************************************************
 MUIM_Show
**************************************************************************/
static ULONG Prop_Show(struct IClass *cl, Object *obj, struct MUIP_Show *msg)
{
    struct MUI_PropData *data = INST_DATA(cl, obj);
    ULONG rc = DoSuperMethodA(cl, obj, (Msg)msg);

    if (!data->usewinborder)
    {
	BOOL isnewlook;

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

/**************************************************************************
 MUIM_Draw
**************************************************************************/
static ULONG Prop_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct MUI_PropData *data = INST_DATA(cl, obj);

    /* No drawings if own border */
    if (data->usewinborder) return 0;

    DoSuperMethodA(cl, obj, (Msg)msg);

    if (!(msg->flags & MADF_DRAWOBJECT)) return 1;
    if (data->prop_object) RefreshGList((struct Gadget*)data->prop_object, _window(obj), NULL, 1);
    return 1;
}

/**************************************************************************
 MUIM_Hide
**************************************************************************/
static ULONG Prop_Hide(struct IClass *cl, Object *obj, struct MUIP_Hide *msg)
{
    struct MUI_PropData *data = INST_DATA(cl, obj);
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

/**************************************************************************
 MUIM_HandleEvent
**************************************************************************/
static ULONG Prop_HandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
    struct MUI_PropData *data = INST_DATA(cl, obj);
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
	    SetAttrs(obj, MUIA_Prop_First, tag->ti_Data, MUIA_Prop_OnlyTrigger, TRUE, TAG_DONE);
	}
    }

    return 0;
}

/**************************************************************************
 MUIM_Prop_Increase
**************************************************************************/
static ULONG Prop_Increase(struct IClass *cl, Object *obj, struct MUIP_Prop_Increase *msg)
{
    struct MUI_PropData *data = INST_DATA(cl, obj);
    LONG newfirst = data->first + msg->amount * data->deltafactor;
    if (newfirst + data->visible > data->entries) newfirst = data->entries - data->visible;
    if (newfirst != data->first) set(obj,MUIA_Prop_First,newfirst);
    return 1;
}

/**************************************************************************
 MUIM_Prop_Decrease
**************************************************************************/
static ULONG Prop_Decrease(struct IClass *cl, Object *obj, struct MUIP_Prop_Decrease *msg)
{
    struct MUI_PropData *data = INST_DATA(cl, obj);

    /* We cannot decrease if if are on the top */
    if (!data->first) return 1;

    if (data->first < msg->amount * data->deltafactor  && data->first != 0) set(obj,MUIA_Prop_First,0);
    else set(obj,MUIA_Prop_First,data->first - msg->amount * data->deltafactor);
    return 1;
}


BOOPSI_DISPATCHER(IPTR, Prop_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Prop_New(cl, obj, (struct opSet *) msg);
	case OM_DISPOSE: return Prop_Dispose(cl, obj, msg);
	case OM_GET: return Prop_Get(cl, obj, (struct opGet *)msg);
	case OM_SET: return Prop_Set(cl, obj, (struct opSet *)msg);
	case MUIM_Setup: return Prop_Setup(cl, obj, (APTR)msg);
	case MUIM_Cleanup: return Prop_Cleanup(cl, obj, (APTR)msg);
	case MUIM_Show: return Prop_Show(cl, obj, (APTR)msg);
	case MUIM_Hide: return Prop_Hide(cl, obj, (APTR)msg);
	case MUIM_AskMinMax: return Prop_AskMinMax(cl, obj, (APTR)msg);
	case MUIM_Draw: return Prop_Draw(cl, obj, (APTR)msg);
	case MUIM_HandleEvent: return Prop_HandleEvent(cl, obj, (APTR)msg);
	case MUIM_Prop_Decrease: return Prop_Decrease(cl, obj, (APTR)msg);
	case MUIM_Prop_Increase: return Prop_Increase(cl, obj, (APTR)msg);

    }
    return DoSuperMethodA(cl, obj, msg);
}


/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Prop_desc = { 
    MUIC_Prop, 
    MUIC_Area, 
    sizeof(struct MUI_PropData), 
    (void*)Prop_Dispatcher 
};
