#include <aros/asmcall.h>
#include <exec/types.h>

#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include "tooliconclass.h"
#include "toolicondefaultpresclass.h"
#include "wbwindowclass.h"

extern struct MUI_CustomClass *ToolIconDefaultPresentationClass;

Object* CreatePresentationObject(Object *target)
{
/*
	Object *presentation;
	Object *realWin, *win;

	presentation=(Object*)NewObject(ToolIconDefaultPresentationClass->mcc_Class, NULL,
		ButtonFrame,
		WBA_ToolIconDefaultPres_Target, target,
		MUIA_FixWidth, 40,
		MUIA_FixHeight, 40,
		TAG_END);

	GetAttr(WBA_ToolIcon_ParentWindow, target, (ULONG*)&win);
	if(win)
	{
		GetAttr(WBA_Window_Window, win, (ULONG*)&realWin);
		if(realWin)
			DoMethod(realWin, OM_ADDMEMBER, presentation);
	}

	return presentation;
*/
	return NULL;
}

ULONG toolIconNew(Class *cl, Object *obj, struct opSet *ops)
{
	ULONG retval=0;
/*
	TagItem *tag;
	char *namepart, *pathpart;
	Object *parentIcon, *parentWindow;
	ToolIconClassData *data;
	BOOL selected;

	tag=FindTagItem(WBA_ToolIcon_NamePart, ops->ops_AttrList);
	if(tag)
	{
		tag->ti_Tag=TAG_IGNORE;
		namepart=(char*)tag->ti_Data;
	}

	tag=FindTagItem(WBA_ToolIcon_PathPart, ops->ops_AttrList);
	if(tag)
	{
		tag->ti_Tag=TAG_IGNORE;
		pathpart=(char*)tag->ti_Data;
	}

	tag=FindTagItem(WBA_ToolIcon_Selected, ops->ops_AttrList);
	if(tag)
	{
		tag->ti_Tag=TAG_IGNORE;
		selected=(BOOL)tag->ti_Data;
	}

	tag=FindTagItem(WBA_ToolIcon_ParentIcon, ops->ops_AttrList);
	if(tag)
	{
		tag->ti_Tag=TAG_IGNORE;
		parentIcon=(Object*)tag->ti_Data;
	}

	tag=FindTagItem(WBA_ToolIcon_ParentWindow, ops->ops_AttrList);
	if(tag)
	{
		tag->ti_Tag=TAG_IGNORE;
		parentWindow=(Object*)tag->ti_Data;
	}

	retval=DoSuperMethodA(cl, obj, (Msg)ops);
	if(retval)
	{
		obj=(Object*)retval;

		data=(ToolIconClassData*)INST_DATA(cl, obj);
		data->namepart=namepart;
		data->pathpart=pathpart;
		data->parentIcon=parentIcon;
		data->parentWindow=parentWindow;
		data->selected=selected;
	}

	data->presentation=CreatePresentationObject(obj);

	return (ULONG)data->presentation;
*/

	return retval;
}

ULONG toolIconGet(Class *cl, Object *obj, struct opGet *opg)
{
	ULONG retval=0;
/*
	ToolIconClassData *data;

	data=(ToolIconClassData*)INST_DATA(cl, obj);

	switch(opg->opg_AttrID)
	{
		case WBA_ToolIcon_NamePart:
			*opg->opg_Storage=(ULONG)data->namepart;
			break;
		case WBA_ToolIcon_PathPart:
			*opg->opg_Storage=(ULONG)data->pathpart;
			break;
		case WBA_ToolIcon_Selected:
			*opg->opg_Storage=(ULONG)data->selected;
			break;
		case WBA_ToolIcon_ParentIcon:
			*opg->opg_Storage=(ULONG)data->parentIcon;
			break;
		case WBA_ToolIcon_ParentWindow:
			*opg->opg_Storage=(ULONG)data->parentWindow;
			break;
	}
*/

	return retval;
}

ULONG toolIconSet(Class *cl, Object *obj, struct opSet *ops)
{
	ULONG retval=0;
/*
	TagItem *tstate, *tag;
	ToolIconClassData *data;

	data=(ToolIconClassData*)INST_DATA(cl, obj);

	tstate=ops->ops_AttrList;
	while(tag=(NextTagItem(&tstate)))
	{
		switch(tag->ti_Tag)
		{
			case WBA_ToolIcon_NamePart:
				tag->ti_Tag=TAG_IGNORE;
				data->namepart=(char*)tag->ti_Data;
				break;
			case WBA_ToolIcon_PathPart:
				tag->ti_Tag=TAG_IGNORE;
				data->pathpart=(char*)tag->ti_Data;
				break;
			case WBA_ToolIcon_Selected:
				tag->ti_Tag=TAG_IGNORE;
				data->selected=(BOOL)tag->ti_Data;
				break;
			case WBA_ToolIcon_ParentIcon:
				tag->ti_Tag=TAG_IGNORE;
				data->parentIcon=(Object*)tag->ti_Data;
				break;
			case WBA_ToolIcon_ParentWindow:
				tag->ti_Tag=TAG_IGNORE;
				data->parentWindow=(Object*)tag->ti_Data;
				break;
		}
	}

	retval=DoSuperMethodA(cl, obj, (Msg)ops);
*/

	return retval;
}

AROS_UFH3(IPTR,toolIconDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
{
	ULONG retval;

	switch(msg->MethodID)
	{
		case OM_NEW:
			retval=toolIconNew(cl, obj, (struct opSet*)msg);
			break;
		case OM_GET:
			retval=toolIconGet(cl, obj, (struct opGet*)msg);
			break;
		case OM_SET:
			retval=toolIconSet(cl, obj, (struct opSet*)msg);
			break;
		default:
			retval=DoSuperMethodA(cl, obj, msg);
			break;
	}

	return retval;
}




