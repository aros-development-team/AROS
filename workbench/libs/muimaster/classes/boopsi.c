/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/memory.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>

#include "debug.h"
#include "mui.h"
#include "support_classes.h"
#include "muimaster_intern.h"
#include "boopsi_private.h"

extern struct Library *MUIMasterBase;

ULONG Boopsi__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Boopsi_DATA *data;
    struct TagItem *tags,*tag;

    obj = (Object *)DoSuperNewTags(cl, obj, NULL,
				   MUIA_FillArea, FALSE,
				   TAG_MORE, (IPTR) msg->ops_AttrList);
    if (!obj)
	return FALSE;

    data = INST_DATA(cl, obj);

    data->boopsi_taglist = CloneTagItems(msg->ops_AttrList);
    data->boopsi_maxwidth = data->boopsi_maxheight = MUI_MAXMAX;

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_Boopsi_Class:
		    data->boopsi_class = (struct IClass*)tag->ti_Data;
	    	    break;

	    case    MUIA_Boopsi_ClassID:
		    data->boopsi_classid = (char*)tag->ti_Data;
	    	    break;

	    case    MUIA_Boopsi_MaxHeight:
		    data->boopsi_minwidth = tag->ti_Data;
		    break;

	    case    MUIA_Boopsi_MaxWidth:
		    data->boopsi_maxwidth = tag->ti_Data;
		    break;

	    case    MUIA_Boopsi_MinHeight:
		    data->boopsi_minheight = tag->ti_Data;
		    break;

	    case    MUIA_Boopsi_MinWidth:
		    data->boopsi_minwidth = tag->ti_Data;
		    break;

	    case    MUIA_Boopsi_Remember:
		    {
			struct TagItem *new_remember;
			if ((new_remember = AllocVec(sizeof(struct TagItem)*(data->remember_len + 2),MEMF_CLEAR))) /* +2 because of the TAG_DONE */
			{
			    if (data->remember) CopyMem(data->remember, new_remember, sizeof(struct TagItem)*data->remember_len);
			    new_remember[data->remember_len].ti_Tag = tag->ti_Data;
			    if (data->remember) FreeVec(data->remember);
			    data->remember = new_remember;
			    data->remember_len++;
			}
		    }
		    break;

	    case    MUIA_Boopsi_Smart:
		    data->boopsi_smart = tag->ti_Data;
		    break;

	    case    MUIA_Boopsi_TagDrawInfo:
		    data->boopsi_tagdrawinfo = tag->ti_Data;
		    break;

	    case    MUIA_Boopsi_TagScreen:
		    data->boopsi_tagscreen = tag->ti_Data;
		    break;

	    case    MUIA_Boopsi_TagWindow:
		    data->boopsi_tagwindow = tag->ti_Data;
		    break;
 
	}
    }

    /* Now fill in the initial remember tag datas in our remember tag list */
    for (tags = data->remember; (tag = NextTagItem(&tags)); )
    {
	struct TagItem *set_tag = FindTagItem(tag->ti_Tag,msg->ops_AttrList);
	if (set_tag) tag->ti_Data = set_tag->ti_Data;
    }

    data->ehn.ehn_Events   = IDCMP_IDCMPUPDATE;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags    = 0;
    data->ehn.ehn_Object   = obj;
    data->ehn.ehn_Class    = cl;

    return (ULONG)obj;
}

ULONG Boopsi__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct Boopsi_DATA *data = INST_DATA(cl, obj);

    if (data->boopsi_taglist) FreeTagItems(data->boopsi_taglist);
    if (data->remember) FreeVec(data->remember);
    return DoSuperMethodA(cl,obj,msg);
}

ULONG Boopsi__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct TagItem *tags,*tag;
    struct Boopsi_DATA *data = INST_DATA(cl, obj);
    int only_trigger = 0;
    int no_notify = 0;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_Boopsi_Class:
		    data->boopsi_class = (struct IClass*)tag->ti_Data;
	    	    break;

	    case    MUIA_Boopsi_ClassID:
		    data->boopsi_classid = (char*)tag->ti_Data;
	    	    break;

	    case    MUIA_Boopsi_MaxHeight:
		    data->boopsi_minwidth = tag->ti_Data;
		    break;

	    case    MUIA_Boopsi_MaxWidth:
		    data->boopsi_maxwidth = tag->ti_Data;
		    break;

	    case    MUIA_Boopsi_MinHeight:
		    data->boopsi_minheight = tag->ti_Data;
		    break;

	    case    MUIA_Boopsi_MinWidth:
		    data->boopsi_minwidth = tag->ti_Data;
		    break;

	    case    MUIA_Boopsi_TagDrawInfo:
		    data->boopsi_tagdrawinfo = tag->ti_Data;
		    break;

	    case    MUIA_Boopsi_TagScreen:
		    data->boopsi_tagscreen = tag->ti_Data;
		    break;

	    case    MUIA_Boopsi_TagWindow:
		    data->boopsi_tagwindow = tag->ti_Data;
		    break;

	    case    MUIA_Boopsi_OnlyTrigger:
	    	    only_trigger = 1;
		    break;

	    case    MUIA_NoNotify:
		    no_notify = tag->ti_Data;
		    break;
	}
    }

    /* Now fill in remember list tag datas in our remember tag list */
    for (tags = data->remember; (tag = NextTagItem(&tags)); )
    {
	struct TagItem *set_tag = FindTagItem(tag->ti_Tag,msg->ops_AttrList);
	if (set_tag) tag->ti_Data = set_tag->ti_Data;
    }

    if (!only_trigger)
    {
	if (data->boopsi_object)
	{
	    /* Rendering will happen here!! This could make problems with virtual groups, forward this to MUIM_Draw??? */
	    if (no_notify) SetAttrs(data->boopsi_object, ICA_TARGET, NULL, TAG_DONE);
	    if (SetGadgetAttrsA((struct Gadget*)data->boopsi_object,_window(obj),NULL,msg->ops_AttrList))
		RefreshGList((struct Gadget*)data->boopsi_object, _window(obj), NULL, 1);
	    if (no_notify) SetAttrs(data->boopsi_object, ICA_TARGET, ICTARGET_IDCMP, TAG_DONE);
        }
    }

    return DoSuperMethodA(cl,obj,(Msg)msg);
}


#define STORE *(msg->opg_Storage)
ULONG Boopsi__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Boopsi_DATA *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
    	case MUIA_Boopsi_Object: STORE = (LONG)data->boopsi_object;

    	default:
    	{
	    struct TagItem *tags,*tag;

    	    /* look in the rember list first */
	    for (tags = data->remember; (tag = NextTagItem(&tags)); )
            {
            	if (tag->ti_Tag == msg->opg_AttrID)
            	{
		    if (data->boopsi_object)
		    {		    	/* Call the get method of the boopsi object and update the remember list */
		    	IPTR val;

		    	if (GetAttr(msg->opg_AttrID,data->boopsi_object,&val))
			    tag->ti_Data = val;
		    }
		    	
		    STORE = tag->ti_Data;
		    return 1;
		}
	    }

	    /* The id is not in the attr list, so we try the boopsi object first (fills in the msg then) */
	    if (data->boopsi_object)
	    {
	    	IPTR val;

	    	if (GetAttr(msg->opg_AttrID,data->boopsi_object,&val))
		{
		    STORE=val;
		    return 1;
		}
	    }

	    /* No success so we try the superclass */
	    return DoSuperMethodA(cl,obj,(Msg)msg);
	}
    }

    return 1;
}
#undef STORE

ULONG Boopsi__MUIM_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct Boopsi_DATA *data = INST_DATA(cl, obj);

    /*
    ** let our superclass first fill in what it thinks about sizes.
    ** this will e.g. add the size of frame and inner spacing.
    */
    DoSuperMethodA(cl, obj, (Msg)msg);

    msg->MinMaxInfo->MinWidth +=  data->boopsi_minwidth;
    msg->MinMaxInfo->MinHeight += data->boopsi_minheight;
    msg->MinMaxInfo->DefWidth +=  data->boopsi_minwidth;
    msg->MinMaxInfo->DefHeight +=  data->boopsi_minheight;
    msg->MinMaxInfo->MaxWidth  += data->boopsi_maxwidth;
    msg->MinMaxInfo->MaxHeight += data->boopsi_maxheight;
    return TRUE;
}

ULONG Boopsi__MUIM_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct Boopsi_DATA *data = INST_DATA(cl, obj);
    ULONG rc = DoSuperMethodA(cl, obj, (Msg)msg);
    if (!rc) return 0;

    DoMethod(_win(obj),MUIM_Window_AddEventHandler,(IPTR)&data->ehn);

    return 1;
}

ULONG Boopsi__MUIM_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct Boopsi_DATA *data = INST_DATA(cl, obj);
    DoMethod(_win(obj),MUIM_Window_RemEventHandler,(IPTR)&data->ehn);
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

ULONG Boopsi__MUIM_Show(struct IClass *cl, Object *obj, struct MUIP_Show *msg)
{
    struct Boopsi_DATA *data = INST_DATA(cl, obj);
    ULONG rc = DoSuperMethodA(cl, obj, (Msg)msg);
    struct TagItem *tag;
    BOOL completely_visible = TRUE;

D(bug("boopsi_show: obj coord %d,%d - %d,%d\n",
_mleft(obj), _mtop(obj), _mright(obj), _mbottom(obj)));
    
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
		    D(bug("=== boopsi object: completely visible FALSE for obj %x at %d,%d - %d,%d\n",
		    	     obj, _mleft(obj), _mtop(obj), _mright(obj), _mbottom(obj)));
		    break;
		}
	    }
	}
    }
    
    if (completely_visible)
    {
	if ((tag = FindTagItem(GA_Left,data->boopsi_taglist))) tag->ti_Data = _mleft(obj);
	if ((tag = FindTagItem(GA_Top,data->boopsi_taglist))) tag->ti_Data = _mtop(obj);
	if ((tag = FindTagItem(GA_Width,data->boopsi_taglist))) tag->ti_Data = _mwidth(obj);
	if ((tag = FindTagItem(GA_Height,data->boopsi_taglist))) tag->ti_Data = _mheight(obj);
	if (data->boopsi_tagscreen && (tag = FindTagItem(data->boopsi_tagscreen,data->boopsi_taglist))) tag->ti_Data = (ULONG)_screen(obj);
	if (data->boopsi_tagwindow && (tag = FindTagItem(data->boopsi_tagwindow,data->boopsi_taglist))) tag->ti_Data = (ULONG)_window(obj);
	if (data->boopsi_tagdrawinfo && (tag = FindTagItem(data->boopsi_tagdrawinfo,data->boopsi_taglist))) tag->ti_Data = (ULONG)_dri(obj);

	if ((data->boopsi_object = NewObjectA(data->boopsi_class, data->boopsi_classid, data->boopsi_taglist)))
	{
    	    SetAttrsA(data->boopsi_object,data->remember);
    	    AddGadget(_window(obj),(struct Gadget*)data->boopsi_object,~0);
	}
    }
    
    return rc;
}

ULONG Boopsi__MUIM_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct Boopsi_DATA *data = INST_DATA(cl, obj);
    DoSuperMethodA(cl, obj, (Msg)msg);

    if (!(msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE))) return 1;
    if (data->boopsi_object) RefreshGList((struct Gadget*)data->boopsi_object, _window(obj), NULL, 1);
    return 1;
}

ULONG Boopsi__MUIM_Hide(struct IClass *cl, Object *obj, struct MUIP_Hide *msg)
{
    struct Boopsi_DATA *data = INST_DATA(cl, obj);
    if (data->boopsi_object)
    {
        struct TagItem *tags,*tag;

        /* Now fill in the initial remember tag datas in our remember tag list */
	for (tags = data->remember; (tag = NextTagItem(&tags)); )
	{
            GetAttr(tag->ti_Tag, data->boopsi_object, &tag->ti_Data);
	}

    	RemoveGadget(_window(obj),(struct Gadget*)data->boopsi_object);
    	DisposeObject(data->boopsi_object);
    	data->boopsi_object = NULL;
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

ULONG Boopsi__MUIM_HandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
    //struct Boopsi_DATA *data = INST_DATA(cl, obj);
    
    if (msg->imsg)
    {
    	if (msg->imsg->Class == IDCMP_IDCMPUPDATE)
    	{
//	    struct TagItem *tags,*tag;

	    /* Go through the tag list (iaddress) and set all the tags, this is somewhat stupid,
	    ** because all objects which hear on the same notifies will receive the notify
	    ** although their value has not changed really, but this how MUI seems to mange it
	    ** (this could real make big problems if using MUIV_TriggerValue)
	    ** a better idea, is to distinguish this via a allocatable gadgetid, and the
	    ** object checks wheather the gadget id equals to its allocated
	    **/

	    SetAttrs(obj,MUIA_Boopsi_OnlyTrigger,TRUE,TAG_MORE,(IPTR)msg->imsg->IAddress);

//            for (tags = (struct TagItem*)msg->imsg->IAddress; (tag = NextTagItem(&tags)); )
//            {
//	    }
	}
    }

    return 0;
}

BOOPSI_DISPATCHER(IPTR, Boopsi_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW:           return Boopsi__OM_NEW(cl, obj, (struct opSet *) msg);
	case OM_DISPOSE:       return Boopsi__OM_DISPOSE(cl, obj, msg);
	case OM_GET:           return Boopsi__OM_GET(cl, obj, (struct opGet *)msg);
	case OM_SET:           return Boopsi__OM_SET(cl, obj, (struct opSet *)msg);
	case MUIM_Setup:       return Boopsi__MUIM_Setup(cl, obj, (APTR)msg);
	case MUIM_Cleanup:     return Boopsi__MUIM_Cleanup(cl, obj, (APTR)msg);
	case MUIM_Show:        return Boopsi__MUIM_Show(cl, obj, (APTR)msg);
	case MUIM_Hide:        return Boopsi__MUIM_Hide(cl, obj, (APTR)msg);
	case MUIM_AskMinMax:   return Boopsi__MUIM_AskMinMax(cl, obj, (APTR)msg);
	case MUIM_Draw:        return Boopsi__MUIM_Draw(cl, obj, (APTR)msg);
	case MUIM_HandleEvent: return Boopsi__MUIM_HandleEvent(cl, obj, (APTR)msg);
    }
    {
	struct Boopsi_DATA *data = INST_DATA(cl, obj);
	if (((msg->MethodID >> 16) != ((TAG_USER >> 16) | 0x0042)) && data->boopsi_object)
	{
	    return DoMethodA(data->boopsi_object, msg);
	}
	return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

#if ZUNE_BUILTIN_BOOPSI
const struct __MUIBuiltinClass _MUI_Boopsi_desc = { 
    MUIC_Boopsi, 
    MUIC_Area, 
    sizeof(struct Boopsi_DATA), 
    (void*)Boopsi_Dispatcher 
};
#endif /* ZUNE_BUILTIN_BOOPSI */
