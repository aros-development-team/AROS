#include <aros/asmcall.h>
#include <exec/types.h>
#include <intuition/classusr.h>
#include <libraries/mui.h>

#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include <stdio.h>

#include "icondefpres.h"

#define DEBUG 1
#include <aros/debug.h>

ULONG iconPresNew(Class *cl, Object *obj, struct opSet *ops)
{
	ULONG retval=0;
	struct TagItem *tag;
	Object *target=NULL;
	BOOL border=FALSE, doubleClicked=FALSE;
	char *label=NULL;
	struct IconDefPresClassData *data;

	tag=FindTagItem(WBA_IconPresentation_Target, ops->ops_AttrList);
	if(tag)
	{
		tag->ti_Tag=TAG_IGNORE;
		target=(Object*)tag->ti_Data;
	}

	tag=FindTagItem(WBA_IconPresentation_Border, ops->ops_AttrList);
	if(tag)
	{
		tag->ti_Tag=TAG_IGNORE;
		border=(BOOL)tag->ti_Data;
	}

	tag=FindTagItem(WBA_IconPresentation_Label, ops->ops_AttrList);
	if(tag)
	{
		tag->ti_Tag=TAG_IGNORE;
		label=(char*)tag->ti_Data;
	}

	retval=DoSuperMethodA(cl, obj, (Msg)ops);
	if(retval)
	{
		obj=(Object*)retval;

		data=(struct IconDefPresClassData*)INST_DATA(cl, obj);
		data->target=target;
		data->border=border;
		data->label=label;
		data->placed=FALSE;
		data->doubleClicked=FALSE;

		data->imageObject=RectangleObject,
//			MUIA_InputMode, MUIV_InputMode_Immediate,
			MUIA_Frame, MUIV_Frame_Button,
			MUIA_FixWidth, 32,
			MUIA_FixHeight, 32,
			End;

		data->labelObject=TextObject,
			MUIA_Text_Contents, data->label,
			End;

		DoMethod(obj, OM_ADDMEMBER, data->imageObject, TAG_END);
		DoMethod(obj, OM_ADDMEMBER, data->labelObject, TAG_END);
	}

	return retval;
}

ULONG iconPresGet(Class *cl, Object *obj, struct opGet *opg)
{
	ULONG retval=0;
	struct IconDefPresClassData *data;

	data=(struct IconDefPresClassData*)INST_DATA(cl, obj);

	switch(opg->opg_AttrID)
	{
		case WBA_IconPresentation_Placed:
			*opg->opg_Storage=(ULONG)data->placed;
			break;
		case WBA_IconPresentation_Target:
			*opg->opg_Storage=(ULONG)data->target;
			break;
		case WBA_IconPresentation_Border:
			*opg->opg_Storage=(ULONG)data->border;
			break;
		case WBA_IconPresentation_Label:
			*opg->opg_Storage=(ULONG)data->label;
			break;
		case WBA_IconPresentation_DoubleClicked:
			retval=1;
			*opg->opg_Storage=(ULONG)data->doubleClicked;
			break;
		default:
			retval=DoSuperMethodA(cl, obj,(Msg)opg);
			break;
	}

	return retval;
}

ULONG iconPresSet(Class *cl, Object *obj, struct opSet *ops)
{
	ULONG retval=0;
	struct TagItem *tstate, *tag;
	struct IconDefPresClassData *data;

	data=(struct IconDefPresClassData*)INST_DATA(cl, obj);

	tstate=ops->ops_AttrList;
	while(tag=(NextTagItem(&tstate)))
	{
		switch(tag->ti_Tag)
		{
			case WBA_IconPresentation_Target:
				tag->ti_Tag=TAG_IGNORE;
				data->target=(Object*)tag->ti_Data;
				break;
			case WBA_IconPresentation_Border:
				tag->ti_Tag=TAG_IGNORE;
				data->border=(BOOL)tag->ti_Data;
				break;
			case WBA_IconPresentation_Label:
				tag->ti_Tag=TAG_IGNORE;
				data->label=(char*)tag->ti_Data;
				break;
			case WBA_IconPresentation_Placed:
				tag->ti_Tag=TAG_IGNORE;
				data->placed=(BOOL)tag->ti_Data;
				break;
			case WBA_IconPresentation_DoubleClicked:
				data->doubleClicked=(BOOL)tag->ti_Data;
				break;
		}
	}

	retval=DoSuperMethodA(cl, obj, (Msg)ops);

	return retval;
}

ULONG iconPresDraw(Class *cl, Object *obj, struct MUIP_Draw *msg)
{
	ULONG retval=0;



	return retval;
}

ULONG iconPresHandleInput(Class *cl, Object *obj, struct MUIP_HandleInput *msg)
{
	ULONG retval;
	struct IconDefPresClassData *data;

	data=(struct IconDefPresClassData*)INST_DATA(cl, obj);

kprintf("icon/handleinput\n");

//	DoSuperMethodA(cl, obj, (Msg)msg);

	if(msg->imsg->Class==IDCMP_MOUSEBUTTONS)
	{
		if(msg->imsg->Code==IECODE_LBUTTON)
		{
			ULONG selected;

			if(msg->imsg->MouseX >= _left(obj) && msg->imsg->MouseX <= _right(obj))
			{
				if(msg->imsg->MouseY >= _top(obj) && msg->imsg->MouseY <= _bottom(obj))
				{
					GetAttr(MUIA_Selected, data->imageObject, &selected);
					if(selected)
						SetAttrs(obj, WBA_IconPresentation_DoubleClicked, TRUE, TAG_END);
					else
						SetAttrs(data->imageObject, MUIA_Selected, TRUE, TAG_END);
				}
			}

		}
	}

	return retval;
}

IPTR iconPresSetup(Class *cl, Object *obj, struct MUIP_Setup *msg)
{
	IPTR retval=0;

	retval=DoSuperMethodA(cl, obj, (Msg)msg);

	MUI_RequestIDCMP(obj, IDCMP_MOUSEBUTTONS);

	return retval;
}

IPTR iconPresCleanup(Class *cl, Object *obj, struct MUIP_Cleanup *msg)
{
	IPTR retval=0;

	MUI_RejectIDCMP(obj, IDCMP_MOUSEBUTTONS);

	retval=DoSuperMethodA(cl, obj, (Msg)msg);

	return retval;
}

AROS_UFH3(IPTR,iconClassDefPresDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
{
	ULONG retval=0;

	switch(msg->MethodID)
	{
		case OM_NEW:
			retval=iconPresNew(cl, obj, (struct opSet*)msg);
			break;
		case OM_GET:
			retval=iconPresGet(cl, obj, (struct opGet*)msg);
			break;
		case OM_SET:
			retval=iconPresSet(cl, obj, (struct opSet*)msg);
			break;
//		case MUIM_Setup:
//			retval=iconPresSetup(cl, obj, (struct MUIP_Setup*)msg);
//			break;
//		case MUIM_Cleanup:
//			retval=iconPresCleanup(cl, obj, (struct MUIP_Cleanup*)msg);
//			break;
//		case MUIM_HandleInput:
//			retval=iconPresHandleInput(cl, obj, (struct MUIP_HandleInput*)msg);
//			break;
//		case MUIM_Draw:
//			printf("Icon Draw\n");
//			retval=iconPresDraw(cl, obj, (struct MUIP_Draw*)msg);
//			break;
		default:
			retval=DoSuperMethodA(cl, obj, msg);
			break;
	}

	return retval;
}

