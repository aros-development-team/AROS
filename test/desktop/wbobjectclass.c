#include <aros/asmcall.h>
#include <exec/types.h>

#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include "wbobjectclass.h"

ULONG wbObjectAdd(Class *cl, Object *obj, struct opMember *opm)
{
	ULONG retval=0;
	struct WBObjectClassData *data;
	Object *presentation;

	data=(struct WBObjectClassData*)INST_DATA(cl, obj);

	GetAttr(WBA_Object_Presentation, opm->opam_Object, (ULONG*)&presentation);
	SetAttrs(opm->opam_Object, WBA_Object_Parent, obj, TAG_END);
	DoMethod(data->presentation, OM_ADDMEMBER, presentation, TAG_END);

	return retval;
}

ULONG wbObjectNew(Class *cl, Object *obj, struct opSet *ops)
{
	ULONG retval=0;
	struct TagItem *tag;
	struct WBObjectClassData *data;
	Object *parent=NULL, *presentation=NULL;

	tag=FindTagItem(WBA_Object_Parent, ops->ops_AttrList);
	if(tag)
	{
		tag->ti_Tag=TAG_IGNORE;
		parent=(Object*)tag->ti_Data;
	}

	tag=FindTagItem(WBA_Object_Presentation, ops->ops_AttrList);
	if(tag)
	{
		tag->ti_Tag=TAG_IGNORE;
		presentation=(Object*)tag->ti_Data;
	}

	retval=DoSuperMethodA(cl, obj, (Msg)ops);
	if(retval)
	{
		obj=(Object*)retval;

		data=(struct WBObjectClassData*)INST_DATA(cl, obj);

		data->parent=parent;
		data->presentation=presentation;
		data->objectID=GetUniqueID();
	}

	return retval;
}

ULONG wbObjectGet(Class *cl, Object *obj, struct opGet *opg)
{
	ULONG retval=0;
	struct WBObjectClassData *data;

	data=(struct WBObjectClassData*)INST_DATA(cl, obj);

	switch(opg->opg_AttrID)
	{
		case WBA_Object_Parent:
			*opg->opg_Storage=(ULONG)data->parent;
			break;
		case WBA_Object_Presentation:
			*opg->opg_Storage=(ULONG)data->presentation;
			break;
		case WBA_Object_ID:
			*opg->opg_Storage=(ULONG)data->objectID;
			break;
		default:
			retval=DoSuperMethodA(cl, obj, (Msg)opg);
			break;
	}

	return retval;
}

ULONG wbObjectSet(Class *cl, Object *obj, struct opSet *ops)
{
	ULONG retval=0;
	struct WBObjectClassData *data;
	struct TagItem *tag;

	data=(struct WBObjectClassData*)INST_DATA(cl, obj);

	tag=FindTagItem(WBA_Object_Parent, ops->ops_AttrList);
	if(tag)
	{
		tag->ti_Tag=TAG_IGNORE;
		data->parent=(Object*)tag->ti_Data;
	}
	tag=FindTagItem(WBA_Object_Presentation, ops->ops_AttrList);
	if(tag)
	{
		tag->ti_Tag=TAG_IGNORE;
		data->presentation=(Object*)tag->ti_Data;
	}

	retval=DoSuperMethodA(cl, obj, (Msg)ops);

	return retval;
}

AROS_UFH3(IPTR,wbObjectClassDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
{
	ULONG retval;

	switch(msg->MethodID)
	{
		case OM_NEW:
			retval=wbObjectNew(cl, obj, (struct opSet*)msg);
			break;
		case OM_GET:
			retval=wbObjectGet(cl, obj, (struct opGet*)msg);
			break;
		case OM_SET:
			retval=wbObjectSet(cl, obj, (struct opSet*)msg);
			break;
		case OM_ADDMEMBER:
			retval=wbObjectAdd(cl, obj, (struct opMember*)msg);
			break;
		case WBM_Object_Added:
			retval=1;
			break;
		default:
			retval=DoSuperMethodA(cl, obj, msg);
			break;
	}

	return retval;
}
