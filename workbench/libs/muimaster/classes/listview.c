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

#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

extern struct Library *MUIMasterBase;

struct MUI_ListviewData
{
    Object *list, *group, *vert, *horiz, *button;
    struct Hook *layout_hook;
    struct Hook hook;
};

#ifndef _AROS
__asm ULONG Listview_Layout_Function(register __a0 struct Hook *hook, register __a2 Object *obj, register __a1 struct MUI_LayoutMsg *lm)
#else
AROS_UFH3(ULONG,Listview_Layout_Function,
	AROS_UFHA(struct Hook *, hook,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(struct MUI_LayoutMsg *, lm,  A1))
#endif
{
    struct MUI_ListviewData *data = (struct MUI_ListviewData *)hook->h_Data;
    switch (lm->lm_Type)
    {
	case    MUILM_MINMAX:
		{
		    /* Calulate the minmax dimension of the group,
		    ** We only have a fixed number of children, so we need no NextObject()
		    */
		    WORD maxxxxwidth = 0;
		    WORD maxxxxheight = 0;

		    maxxxxwidth = _minwidth(data->list) + _minwidth(data->vert);
		    if (_minwidth(data->horiz) + _minwidth(data->vert) > maxxxxwidth) maxxxxwidth = _minwidth(data->horiz) + _minwidth(data->vert);
		    lm->lm_MinMax.MinWidth = maxxxxwidth;

		    maxxxxheight = _minheight(data->list) + _minheight(data->horiz);
		    if (_minheight(data->vert) + _minheight(data->horiz) > maxxxxheight) maxxxxheight = _minheight(data->vert) + _minheight(data->horiz);
		    lm->lm_MinMax.MinHeight = maxxxxheight;

		    maxxxxwidth = _defwidth(data->list) + _defwidth(data->vert);
		    if (_defwidth(data->horiz) > maxxxxwidth) maxxxxwidth = _defwidth(data->horiz);
		    lm->lm_MinMax.DefWidth = maxxxxwidth;

		    maxxxxheight = _defheight(data->list) + _defheight(data->horiz);
		    if (_defheight(data->vert) > maxxxxheight) maxxxxheight = _defheight(data->vert);
		    lm->lm_MinMax.DefHeight = maxxxxheight;

		    lm->lm_MinMax.MaxWidth  = MUI_MAXMAX;
		    lm->lm_MinMax.MaxHeight = MUI_MAXMAX;
		    return 0;
		}

		case MUILM_LAYOUT:
		{
		    /* Now place the objects between (0,0,lm->lm_Layout.Width-1,lm->lm_Layout.Height-1)
		    */

		    LONG vert_width = _minwidth(data->vert);
		    LONG horiz_height = _minheight(data->horiz);
		    LONG lay_width = lm->lm_Layout.Width;
		    LONG lay_height = lm->lm_Layout.Height;
		    LONG cont_width;
		    LONG cont_height;

		    /* We need all scrollbars and the button */
		    set(data->vert, MUIA_ShowMe, TRUE); /* We could also overload MUIM_Show... */
		    set(data->horiz, MUIA_ShowMe, TRUE);
		    set(data->button, MUIA_ShowMe, TRUE);
		    cont_width = lay_width - vert_width;
		    cont_height = lay_height - horiz_height;

		    MUI_Layout(data->vert, cont_width, 0, vert_width, cont_height,0);
		    MUI_Layout(data->horiz, 0, cont_height, cont_width, horiz_height, 0);
		    MUI_Layout(data->button, cont_width, cont_height, vert_width, horiz_height, 0);

		    /* Layout the group a second time, note that setting _mwidth() and _mheight() should be enough, or we invent a new flag */
		    MUI_Layout(data->list,0,0,cont_width,cont_height,0);
		    return 1;
		}
    }
    return 0;
}

#define PROP_VERT_FIRST   1
#define LIST_VERT_FIRST   4
#define LIST_VERT_VISIBLE 5
#define LIST_VERT_ENTRIES 6

#ifndef _AROS
__asm ULONG Listview_Function(register __a0 struct Hook *hook, register __a1 void **msg)
#else
AROS_UFH3(ULONG,Listview_Function,
	AROS_UFHA(struct Hook *, hook,  A0),
	AROS_UFHA(APTR, dummy, A2),
	AROS_UFHA(void **, msg,  A1))
#endif
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

	case	2:
		{
		    get(data->horiz,MUIA_Prop_First,&val);
//		    SetAttrs(data->contents,MUIA_Virtgroup_Left, val, MUIA_NoNotify, TRUE, MUIA_Group_Forward, FALSE, TAG_DONE);
		    break;
		}
	case	3: nnset(data->horiz, MUIA_Prop_First, val); break;

	case	LIST_VERT_FIRST: nnset(data->vert, MUIA_Prop_First, val); break;
	case	LIST_VERT_VISIBLE: nnset(data->vert, MUIA_Prop_Visible, val); break;
	case	LIST_VERT_ENTRIES: nnset(data->vert, MUIA_Prop_Entries, val); break;
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
    Object *group, *vert, *horiz, *button;
    Object *list = (Object*)GetTagData(MUIA_Listview_List, NULL, msg->ops_AttrList);
    LONG entries,first,visible;
    if (!list) return NULL;

    layout_hook = mui_alloc_struct(struct Hook);
    if (!layout_hook) return NULL;

    layout_hook->h_Entry = (HOOKFUNC)Listview_Layout_Function;

    obj = (Object *)DoSuperNew(cl, obj,
	MUIA_Group_Horiz, FALSE,
	Child, group = GroupObject,
	    MUIA_Group_LayoutHook, layout_hook,
	    Child, list,
	    Child, vert = ScrollbarObject, MUIA_Group_Horiz, FALSE, End,
	    Child, horiz = ScrollbarObject, MUIA_Group_Horiz, TRUE, End,
	    Child, button = ScrollbuttonObject, End,
	    End,
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
    data->horiz = horiz;
    data->button = button;
    data->group = group;
    data->layout_hook = layout_hook;

    data->hook.h_Entry = (HOOKFUNC)Listview_Function;
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

    DoMethod(vert, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime, obj, 4, MUIM_CallHook, &data->hook, PROP_VERT_FIRST, MUIV_TriggerValue);
    DoMethod(list, MUIM_Notify, MUIA_List_VertProp_First, MUIV_EveryTime, obj, 4, MUIM_CallHook, &data->hook, LIST_VERT_FIRST, MUIV_TriggerValue);
    DoMethod(list, MUIM_Notify, MUIA_List_VertProp_Visible, MUIV_EveryTime, obj, 4, MUIM_CallHook, &data->hook, LIST_VERT_VISIBLE, MUIV_TriggerValue);
    DoMethod(list, MUIM_Notify, MUIA_List_VertProp_Entries, MUIV_EveryTime, obj, 4, MUIM_CallHook, &data->hook, LIST_VERT_ENTRIES, MUIV_TriggerValue);

    return (IPTR)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
static IPTR Listview_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_ListviewData *data = INST_DATA(cl, obj);
    mui_free(data->layout_hook); /* is always here */
    return 0;
}

#ifndef _AROS
__asm IPTR Listview_Dispatcher( register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR,Listview_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Listview_New(cl, obj, (struct opSet *)msg);
	case OM_DISPOSE: return Listview_Dispose(cl,obj,msg);
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Listview_desc = { 
    MUIC_Listview, 
    MUIC_Group, 
    sizeof(struct MUI_ListviewData), 
    (void*)Listview_Dispatcher 
};

