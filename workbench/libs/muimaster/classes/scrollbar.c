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

struct MUI_ScrollbarData
{
    int dummy;
};

/**************************************************************************
 OM_NEW
**************************************************************************/
static ULONG Scrollbar_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_PropData *data;
    struct TagItem *tags,*tag;
    int horiz = GetTagData(MUIA_Group_Horiz, 0, msg->ops_AttrList);
    Object *prop = MUI_NewObject(MUIC_Prop,MUIA_Prop_Horiz, horiz, TAG_MORE, msg->ops_AttrList);
    Object *but;

    obj = (Object *)DoSuperNew(cl, obj, MUIA_Group_Spacing, 0, MUIA_Group_Child, prop, TAG_MORE, msg->ops_AttrList);
    if (!obj)
	return FALSE;

    data = INST_DATA(cl, obj);

    but = TextObject, MUIA_Weight, 0, ButtonFrame, MUIA_InputMode, MUIV_InputMode_RelVerify, MUIA_Text_PreParse, "\33c", MUIA_Text_Contents, horiz?"L":"U", End;
    if (but)
    {
	DoMethod(but, MUIM_Notify, MUIA_Timer, MUIV_EveryTime, prop, 2, MUIM_Prop_Decrease, 1);
	DoMethod(obj, OM_ADDMEMBER, but);
    }

    but = TextObject, MUIA_Weight, 0, ButtonFrame, MUIA_InputMode, MUIV_InputMode_RelVerify, MUIA_Text_PreParse, "\33c", MUIA_Text_Contents, horiz?"R":"D", End;
    if (but)
    {
	DoMethod(but, MUIM_Notify, MUIA_Timer, MUIV_EveryTime, prop, 2, MUIM_Prop_Increase, 1);
	DoMethod(obj, OM_ADDMEMBER, but);
    }

    return (ULONG)obj;
}

#ifndef _AROS
__asm IPTR Scrollbar_Dispatcher( register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR,Scrollbar_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Scrollbar_New(cl, obj, (struct opSet *) msg);
    }
    return DoSuperMethodA(cl, obj, msg);
}


/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Scrollbar_desc = { 
    MUIC_Scrollbar, 
    MUIC_Group, 
    sizeof(struct MUI_ScrollbarData), 
    Scrollbar_Dispatcher 
};
