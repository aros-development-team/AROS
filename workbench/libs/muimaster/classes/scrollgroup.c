#include <exec/memory.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

extern struct Library *MUIMasterBase;

struct MUI_ScrollgroupData
{
    Object *contents;
    Object *vert, *horiz, *button;
    struct Hook hook;
};


#ifndef _AROS
__asm ULONG Scrollgroup_Function(register __a0 struct Hook *hook, register __a1 void **msg)
#else
AROS_UFH3(ULONG,Scrollgroup_Function,
	AROS_UFHA(struct Hook *, hook,  A0),
	AROS_UFHA(APTR, dummy, A2),
	AROS_UFHA(void **, msg,  A1))
#endif
{
    struct MUI_ScrollgroupData *data = (struct MUI_ScrollgroupData *)hook->h_Data;
    int type = (int)msg[0];
    LONG val = (LONG)msg[1];

    switch (type)
    {
	case	1: SetAttrs(data->contents,MUIA_Virtgroup_Top, val, MUIA_NoNotify, TRUE, MUIA_Group_Forward, FALSE, TAG_DONE); break;
	case	2: SetAttrs(data->contents,MUIA_Virtgroup_Left, val, MUIA_NoNotify, TRUE, MUIA_Group_Forward, FALSE, TAG_DONE); break;
	case	3: nnset(data->horiz, MUIA_Prop_First, val); break;
	case	4: nnset(data->vert, MUIA_Prop_First, val); break;
    }
    return 0;
}

/**************************************************************************
 OM_NEW
**************************************************************************/
static ULONG Scrollgroup_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ScrollgroupData *data;
    struct TagItem *tags,*tag;
    Object *contents = (Object*)GetTagData(MUIA_Scrollgroup_Contents, NULL, msg->ops_AttrList);
    Object *vert,*horiz, *button;

    obj = (Object *)DoSuperNew(cl, obj,
    	MUIA_Group_Horiz, FALSE,
    	Child, ColGroup(2),
	    Child, contents = HGroupV, /* Saves one variable */
		VirtualFrame,
		Child, contents,
		End,
	    Child, vert = ScrollbarObject, MUIA_Group_Horiz, FALSE, End,

	    Child, horiz = ScrollbarObject, MUIA_Group_Horiz, TRUE, End,
	    Child, button = ScrollbuttonObject, End,
	    End,
	TAG_DONE);

    if (!obj)
	return FALSE;

    data = INST_DATA(cl, obj);
    data->vert = vert;
    data->horiz = horiz;
    data->button = button;
    data->contents = contents;

    data->hook.h_Entry = (HOOKFUNC)Scrollgroup_Function;
    data->hook.h_Data = data;

    DoMethod(vert, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime, obj, 4, MUIM_CallHook, &data->hook, 1, MUIV_TriggerValue);
    DoMethod(horiz, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime, obj, 4, MUIM_CallHook, &data->hook, 2, MUIV_TriggerValue);
    DoMethod(contents, MUIM_Notify, MUIA_Virtgroup_Left, MUIV_EveryTime, obj, 4, MUIM_CallHook, &data->hook, 3, MUIV_TriggerValue);
    DoMethod(contents, MUIM_Notify, MUIA_Virtgroup_Top, MUIV_EveryTime, obj, 4, MUIM_CallHook, &data->hook, 4, MUIV_TriggerValue);

    return (ULONG)obj;
}

/**************************************************************************
 MUIM_Show
**************************************************************************/
static ULONG Scrollgroup_Show(struct IClass *cl, Object *obj, struct MUIP_Show *msg)
{
    struct MUI_ScrollgroupData *data = INST_DATA(cl, obj);
    LONG top,left,width,height;

    get(data->contents, MUIA_Virtgroup_Left, &left);
    get(data->contents, MUIA_Virtgroup_Top, &top);
    get(data->contents, MUIA_Virtgroup_Width, &width);
    get(data->contents, MUIA_Virtgroup_Height, &height);

    SetAttrs(data->horiz, MUIA_Prop_First, left,
			  MUIA_Prop_Entries, width,
			  MUIA_Prop_Visible, _mwidth(data->contents));


    SetAttrs(data->vert,  MUIA_Prop_First, top,
			  MUIA_Prop_Entries, height,
			  MUIA_Prop_Visible, _mheight(data->contents));

    return DoSuperMethodA(cl,obj,(Msg)msg);
}

#ifndef _AROS
__asm IPTR Scrollgroup_Dispatcher( register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR,Scrollgroup_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Scrollgroup_New(cl, obj, (struct opSet *) msg);
	case MUIM_Show: return Scrollgroup_Show(cl, obj, (struct MUIP_Show*)msg);
    }
    return DoSuperMethodA(cl, obj, msg);
}


/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Scrollgroup_desc = { 
    MUIC_Scrollgroup, 
    MUIC_Group, 
    sizeof(struct MUI_ScrollgroupData), 
    Scrollgroup_Dispatcher 
};
