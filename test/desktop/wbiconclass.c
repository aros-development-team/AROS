#include <aros/asmcall.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/classusr.h>
#include <libraries/mui.h>

#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include "wbobjectclass.h"
#include "wbiconclass.h"
#include "wbwindowclass.h"
#include "toolicondefaultpresclass.h"
#include "icondefpres.h"

#include <stdio.h>
#include <string.h>

#define DEBUG 1
#include <aros/debug.h>

extern struct MUI_CustomClass *WBToolIconDefaultPresentationClass;
extern struct MUI_CustomClass *WBWindowClass;

Object* createIconPresentationClass(Object *target)
{
	Object *pres=NULL;
	char *name=NULL;
	LONG type=0;
	Object *image, *label;

	GetAttr(WBA_Icon_Name, target, (ULONG*)&name);
	GetAttr(WBA_Icon_Type, target, (ULONG*)&type);

/*
2  : Drawer
-3 : Tool/Project
*/
	switch(type)
	{
// TEMPORARY -- always create a tool icon presentation class, for now...
		default:
			pres=(Object*)NewObject(WBToolIconDefaultPresentationClass->mcc_Class, NULL,
				WBA_IconPresentation_Target, target,
				WBA_IconPresentation_Border, TRUE,
				WBA_IconPresentation_Label, name,
				End;
			break;
	}

	return pres;
}

ULONG wbIconNew(Class *cl, Object *obj, struct opSet *ops)
{
	ULONG retval=0;
	struct TagItem *tag;
	struct WBIconClassData *data;
	BOOL selected=FALSE;
	char *name;
	LONG type;
	Object *presentation;

	tag=FindTagItem(WBA_Icon_Selected, ops->ops_AttrList);
	if(tag)
	{
		tag->ti_Tag=TAG_IGNORE;
		selected=(BOOL)tag->ti_Data;
	}

	tag=FindTagItem(WBA_Icon_Name, ops->ops_AttrList);
	if(tag)
	{
		tag->ti_Tag=TAG_IGNORE;
		name=(char*)tag->ti_Data;
	}
	else
		return 0;

	tag=FindTagItem(WBA_Icon_Type, ops->ops_AttrList);
	if(tag)
	{
		tag->ti_Tag=TAG_IGNORE;
		type=(LONG)tag->ti_Data;
	}
	else
		return 0;

	retval=DoSuperMethodA(cl, obj, (Msg)ops);
	if(retval)
	{
		obj=(Object*)retval;

		data=(struct WBIconClassData*)INST_DATA(cl, obj);

		data->selected=selected;
		data->name=name;
		data->type=type;

		presentation=createIconPresentationClass(obj);

		SetAttrs(obj, WBA_Object_Presentation, presentation, TAG_END);
	}

	return retval;
}

ULONG wbIconGet(Class *cl, Object *obj, struct opGet *opg)
{
	ULONG retval=0;
	struct WBIconClassData *data;

	data=(struct WBIconClassData*)INST_DATA(cl, obj);

	switch(opg->opg_AttrID)
	{
		case WBA_Icon_Selected:
			*opg->opg_Storage=(ULONG)data->selected;
			retval=1;
			break;
		case WBA_Icon_Name:
			*opg->opg_Storage=(ULONG)data->name;
			break;
		default:
			retval=DoSuperMethodA(cl, obj, (Msg)opg);
			break;
	}

	return retval;
}

ULONG wbIconSetup(Class *cl, Object *obj, struct MUIP_Setup *msg)
{
	ULONG retval=0;
	struct WBIconClassData *data;
	Object *pres;

	data=(struct WBIconClassData*)INST_DATA(cl, obj);

	DoSuperMethodA(cl, obj, (Msg)msg);

//	GetAttr(WBA_Object_Presentation, obj, (ULONG*)&pres);

//	DoMethod(pres, MUIM_Notify, MUIA_Selected, TRUE, obj, 3, MUIM_Set, WBA_Icon_Selected, TRUE);

	return retval;
}

IPTR wbIconAdded(Class *cl, Object *obj, Msg msg)
{
	IPTR retval=0;
	Object *pres;
	struct WBIconClassData *data;

	data=(struct WBIconClassData*)INST_DATA(cl, obj);

	retval=DoSuperMethodA(cl, obj, (Msg)msg);

//	GetAttr(WBA_Object_Presentation, obj, (ULONG*)&pres);
//	DoMethod(pres, MUIM_Notify, MUIA_Selected, (ULONG)TRUE, obj, 3, MUIM_Set, WBA_Icon_Selected, (ULONG)TRUE);

//	DoMethod(pres, MUIM_Notify, WBA_IconPresentation_DoubleClicked, TRUE, obj, 3, MUIM_Set, WBA_Icon_DoubleClicked, TRUE);


	return retval;
}

ULONG wbIconSet(Class *cl, Object *obj, struct opSet *ops)
{
	ULONG retval=0;
	struct WBIconClassData *data;
	struct TagItem *tag;
	Object *pres;
	ULONG selected;

	data=(struct WBIconClassData*)INST_DATA(cl, obj);

	tag=FindTagItem(WBA_Icon_Name, ops->ops_AttrList);
	if(tag)
	{
		tag->ti_Tag=TAG_IGNORE;
		data->name=(char*)tag->ti_Data;
	}
	tag=FindTagItem(WBA_Icon_Selected, ops->ops_AttrList);
	if(tag)
	{
		GetAttr(WBA_Object_Presentation, obj, &pres);
		SetAttrs(pres, MUIA_Selected, tag->ti_Data, TAG_END);
		data->selected=tag->ti_Data;
		// until zune is fixed....
		MUI_Redraw(pres, MADF_DRAWOBJECT);
	}
	tag=FindTagItem(WBA_Icon_DoubleClicked, ops->ops_AttrList);
	if(tag)
	{
		DoMethod(obj, WBM_Icon_Execute);
	}
	retval=DoSuperMethodA(cl, obj, (Msg)ops);

	return retval;
}

IPTR wbIconHandleInput(Class *cl, Object *obj, struct IconHandleInputMethodData *msg)
{
	IPTR retval=0;
	Object *pres;
	struct WBIconClassData *data;

	data=(struct WBIconClassData*)INST_DATA(cl, obj);

	GetAttr(WBA_Object_Presentation, obj, &pres);

	if(msg->imsg->MouseX >= _left(pres) && msg->imsg->MouseX <= _right(pres))
	{
		if(msg->imsg->MouseY >= _top(pres) && msg->imsg->MouseY <= _bottom(pres))
		{
			retval=1;
			if(data->selected)
				SetAttrs(obj, WBA_Icon_DoubleClicked, TRUE, TAG_END);
			else
				SetAttrs(obj, WBA_Icon_Selected, TRUE, TAG_END);
		}
	}

	return retval;
}

AROS_UFH3(IPTR,wbIconClassDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
{
	ULONG retval;

	switch(msg->MethodID)
	{
		case OM_NEW:
			retval=wbIconNew(cl, obj, (struct opSet*)msg);
			break;
		case OM_GET:
			retval=wbIconGet(cl, obj, (struct opGet*)msg);
			break;
		case MUIM_Setup:
			retval=wbIconSetup(cl, obj, (struct MUIP_Setup*)msg);
			break;
//		case MUIM_Set:
//			retval=wbIconMSet(cl, obj, (struct MUIP_Set*)msg);
//			break;
		case OM_SET:
			retval=wbIconSet(cl, obj, (struct opSet*)msg);
			break;
		case WBM_Object_Added:
			retval=wbIconAdded(cl, obj, msg);
			break;
		case WBM_Icon_HandleInput:
			retval=wbIconHandleInput(cl, obj, (struct IconHandleInputMethodData*)msg);
			break;
		default:
			retval=DoSuperMethodA(cl, obj, msg);
			break;
	}

	return retval;
}

