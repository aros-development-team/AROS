/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

extern struct Library *MUIMasterBase;

struct MUI_ListviewData
{
    Object *list, *group, *vert;
    struct Hook *layout_hook;
    struct Hook hook;
};

ULONG Listview_Layout_Function(struct Hook *hook, Object *obj, struct MUI_LayoutMsg *lm)
{
    struct MUI_ListviewData *data = (struct MUI_ListviewData *)hook->h_Data;
    switch (lm->lm_Type)
    {
	case    MUILM_MINMAX:
		{
		    /* Calculate the minmax dimension of the group,
		    ** We only have a fixed number of children, so we need no NextObject()
		    */
		    lm->lm_MinMax.MinWidth = _minwidth(data->list) + _minwidth(data->vert);
		    lm->lm_MinMax.DefWidth = _defwidth(data->list) + _defwidth(data->vert);
		    lm->lm_MinMax.MaxWidth = _maxwidth(data->list) + _maxwidth(data->vert);
		    lm->lm_MinMax.MaxWidth = MIN(lm->lm_MinMax.MaxWidth, MUI_MAXMAX);

		    lm->lm_MinMax.MinHeight = MAX(_minheight(data->list), _minheight(data->vert));
		    lm->lm_MinMax.DefHeight = MAX(_defheight(data->list), lm->lm_MinMax.MinHeight);
		    lm->lm_MinMax.MaxHeight = MIN(_maxheight(data->list), _maxheight(data->vert));
		    lm->lm_MinMax.MaxHeight = MIN(lm->lm_MinMax.MaxHeight, MUI_MAXMAX);
		    return 0;
		}

		case MUILM_LAYOUT:
		{
		    /* Now place the objects between
		     * (0, 0, lm->lm_Layout.Width - 1, lm->lm_Layout.Height - 1)
		    */

		    LONG vert_width = _minwidth(data->vert);
		    LONG lay_width = lm->lm_Layout.Width;
		    LONG lay_height = lm->lm_Layout.Height;
		    LONG cont_width;
		    LONG cont_height;

		    /* We need all scrollbars and the button */
		    set(data->vert, MUIA_ShowMe, TRUE); /* We could also overload MUIM_Show... */
		    cont_width = lay_width - vert_width;
		    cont_height = lay_height;

		    MUI_Layout(data->vert, cont_width, 0, vert_width, cont_height,0);

		    /* Layout the group a second time, note that setting _mwidth() and
		       _mheight() should be enough, or we invent a new flag */
		    MUI_Layout(data->list, 0, 0, cont_width, cont_height, 0);
		    return 1;
		}
    }
    return 0;
}

#define PROP_VERT_FIRST   1
#define LIST_VERT_FIRST   4
#define LIST_VERT_VISIBLE 5
#define LIST_VERT_ENTRIES 6

ULONG Listview_Function(struct Hook *hook, APTR dummyobj, void **msg)
{
    struct MUI_ListviewData *data = (struct MUI_ListviewData *)hook->h_Data;
    int type = (int)msg[0];
    LONG val = (LONG)msg[1];

    switch (type)
    {
	case	PROP_VERT_FIRST:
		get(data->vert,MUIA_Prop_First,&val);
		nnset(data->list,MUIA_List_VertProp_First,val);
		break;

	case	LIST_VERT_FIRST:
	    nnset(data->vert, MUIA_Prop_First, val); break;
	case	LIST_VERT_VISIBLE:
	    nnset(data->vert, MUIA_Prop_Visible, val); break;
	case	LIST_VERT_ENTRIES:
	    nnset(data->vert, MUIA_Prop_Entries, val); break;
    }
    return 0;
}

/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR Listview_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ListviewData   *data;
    struct TagItem *tag, *tags;
    struct Hook *layout_hook;
    Object *group, *vert;
    Object *list = (Object*)GetTagData(MUIA_Listview_List, NULL, msg->ops_AttrList);
    LONG entries,first,visible;
    if (!list) return NULL;

    layout_hook = mui_alloc_struct(struct Hook);
    if (!layout_hook) return NULL;

    layout_hook->h_Entry = HookEntry;
    layout_hook->h_SubEntry = (HOOKFUNC)Listview_Layout_Function;

    obj = (Object *)DoSuperNewTags(cl, obj, NULL,
	MUIA_Group_Horiz, FALSE,
	MUIA_InnerLeft, 0,
	MUIA_InnerRight, 0,
	Child, (IPTR) (group = GroupObject,
	    MUIA_InnerLeft, 0,
	    MUIA_InnerRight, 0,
	    MUIA_Group_LayoutHook, (IPTR) layout_hook,
	    Child, (IPTR) list,
	    Child, (IPTR) (vert = ScrollbarObject, MUIA_Group_Horiz, FALSE, End),
	    End),
	TAG_DONE);

    if (!obj)
    {
	mui_free(layout_hook);
	return NULL;
    }

    data = INST_DATA(cl, obj);
    layout_hook->h_Data = data;
    data->list = list;
    data->vert = vert;
    data->group = group;
    data->layout_hook = layout_hook;

    data->hook.h_Entry = HookEntry;
    data->hook.h_SubEntry = (HOOKFUNC)Listview_Function;
    data->hook.h_Data = data;

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
    	}
    }

    get(list,MUIA_List_VertProp_First,&first);
    get(list,MUIA_List_VertProp_Visible,&visible);
    get(list,MUIA_List_VertProp_Entries,&entries);

    SetAttrs(data->vert,
	MUIA_Prop_First, first,
	MUIA_Prop_Visible, visible,
	MUIA_Prop_Entries, entries,
	TAG_DONE);

    DoMethod(vert, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime, (IPTR)obj, 4,
	     MUIM_CallHook, (IPTR)&data->hook, PROP_VERT_FIRST, MUIV_TriggerValue);
    DoMethod(list, MUIM_Notify, MUIA_List_VertProp_First, MUIV_EveryTime,
	     (IPTR)obj, 4, MUIM_CallHook, (IPTR)&data->hook, LIST_VERT_FIRST,
	     MUIV_TriggerValue);
    DoMethod(list, MUIM_Notify, MUIA_List_VertProp_Visible, MUIV_EveryTime,
	     (IPTR)obj, 4, MUIM_CallHook, (IPTR)&data->hook, LIST_VERT_VISIBLE,
	     MUIV_TriggerValue);
    DoMethod(list, MUIM_Notify, MUIA_List_VertProp_Entries, MUIV_EveryTime,
	     (IPTR)obj, 4, MUIM_CallHook, (IPTR)&data->hook, LIST_VERT_ENTRIES,
	     MUIV_TriggerValue);

    return (IPTR)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
static IPTR Listview_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_ListviewData *data = INST_DATA(cl, obj);

    mui_free(data->layout_hook); /* is always here */
    return DoSuperMethodA(cl, obj, msg);
}

BOOPSI_DISPATCHER(IPTR, Listview_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Listview_New(cl, obj, (struct opSet *)msg);
	case OM_DISPOSE: return Listview_Dispose(cl,obj,msg);
	case MUIM_List_Clear:
	case MUIM_List_CreateImage:
	case MUIM_List_DeleteImage:
	case MUIM_List_Exchange:
	case MUIM_List_GetEntry:
	case MUIM_List_Insert:
	case MUIM_List_InsertSingle:
	case MUIM_List_Jump:
	case MUIM_List_NextSelected:
	case MUIM_List_Redraw:
	case MUIM_List_Remove:
	case MUIM_List_Select:
	case MUIM_List_Sort:
	case MUIM_List_TestPos:
	{
	    struct MUI_ListviewData *data = INST_DATA(cl, obj);
	    
	    return DoMethodA(data->list, msg);
	}
	
    }
    
    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Listview_desc = { 
    MUIC_Listview, 
    MUIC_Group, 
    sizeof(struct MUI_ListviewData), 
    (void*)Listview_Dispatcher 
};

