#include <exec/memory.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

extern struct Library *MUIMasterBase;

struct MUI_RegisterData
{
    char **labels;
};

/**************************************************************************
 OM_NEW
**************************************************************************/
static ULONG Register_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_RegisterData *data;
    struct TagItem *tags,*tag;
    Object *but_group, *page_group;
    int i,j,groups,group_act;
    Object **buts;

    group_act = GetTagData(MUIA_Group_ActivePage, 0, msg->ops_AttrList);

    obj = (Object *)DoSuperNew(cl, obj, TAG_DONE); /* We need no tags */ 
    if (!obj) return NULL;

    data = INST_DATA(cl, obj);
    data->labels = (char**)GetTagData(MUIA_Register_Titles, NULL, msg->ops_AttrList);
    if (!data->labels)
    {
	CoerceMethod(cl, obj, OM_DISPOSE);
	return NULL;
    }

    but_group = HGroup, End;
    if (!but_group)
    {
	CoerceMethod(cl, obj, OM_DISPOSE);
	return NULL;
    }

    page_group = MUI_NewObject(MUIC_Group, MUIA_Group_PageMode, TRUE, TAG_MORE, msg->ops_AttrList);
    if (!page_group)
    {
    	MUI_DisposeObject(but_group);
	CoerceMethod(cl, obj, OM_DISPOSE);
	return NULL;
    }

    /* Count the number of groups */
    for (groups=0;data->labels[groups];groups++);

    if (!(buts = (Object**)AllocVec(sizeof(Object *)*groups,0)))
    {
    	MUI_DisposeObject(page_group);
    	MUI_DisposeObject(but_group);
	CoerceMethod(cl, obj, OM_DISPOSE);
	return NULL;
    }

    for (i=0;i<groups;i++)
    {
	buts[i] = TextObject, ButtonFrame, MUIA_InputMode, MUIV_InputMode_Immediate, MUIA_Text_Contents, data->labels[i], End;
	if (!buts[i])
	{
	    FreeVec(buts);
	    MUI_DisposeObject(page_group);
	    MUI_DisposeObject(but_group);
	    CoerceMethod(cl, obj, OM_DISPOSE);
	    return NULL;
	}
	DoMethod(but_group, OM_ADDMEMBER, buts[i]);
    }

    for (i=0;i<groups;i++)
    {
	for (j=0;j<groups;j++)
	{
	   if (i==j) continue;
	   DoMethod(buts[i],MUIM_Notify, MUIA_Selected, TRUE, buts[j], 3, MUIM_Set, MUIA_Selected, FALSE);
	}
	DoMethod(buts[i],MUIM_Notify, MUIA_Selected, TRUE, page_group, 3, MUIM_Set, MUIA_Group_ActivePage, i);
    }

    set(buts[group_act],MUIA_Selected,TRUE);

    DoMethod(obj, OM_ADDMEMBER, but_group);
    DoMethod(obj, OM_ADDMEMBER, page_group);
    FreeVec(buts);
    return obj;
}

#ifndef _AROS
__asm IPTR Register_Dispatcher( register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR,Register_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Register_New(cl, obj, (struct opSet *) msg);
    }
    return DoSuperMethodA(cl, obj, msg);
}


/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Register_desc = { 
    MUIC_Register, 
    MUIC_Group, 
    sizeof(struct MUI_RegisterData), 
    Register_Dispatcher 
};
