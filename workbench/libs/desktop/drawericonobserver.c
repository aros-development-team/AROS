/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <libraries/gadtools.h>
#include <libraries/mui.h>
#include <libraries/desktop.h>

#include "support.h"
#include "worker.h"
#include "desktop_intern.h"

#include <proto/desktop.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "presentation.h"
#include "observer.h"
#include "iconobserver.h"
#include "drawericonobserver.h"

#include "desktop_intern_protos.h"

#include <string.h>

IPTR drawerIconObserverNew(Class *cl, Object *obj, struct opSet *msg)
{
	IPTR retval=0;
	struct DrawerIconObserverClassData *data;
	struct TagItem *tag;

	retval=DoSuperMethodA(cl, obj, (Msg)msg);
	if(retval)
	{
		obj=(Object*)retval;
		data=INST_DATA(cl, obj);
	}

	return retval;
}

IPTR drawerIconObserverSet(Class *cl, Object *obj, struct opSet *msg)
{
	struct DrawerIconObserverClassData *data;
	IPTR retval=1;
	struct TagItem *tag, *tstate=msg->ops_AttrList;

	data=(struct DrawerIconObserverClassData*)INST_DATA(cl, obj);

	while((tag=NextTagItem(&tstate)))
	{
		switch(tag->ti_Tag)
		{
			default:
				retval=DoSuperMethodA(cl, obj, (Msg)msg);
				break;
		}
	}

	return retval;
}

IPTR drawerIconObserverGet(Class *cl, Object *obj, struct opGet *msg)
{
	IPTR retval=1;
	struct DrawerIconObserverClassData *data;

	data=(struct DrawerIconObserverClassData*)INST_DATA(cl, obj);

	switch(msg->opg_AttrID)
	{
		default:
			retval=DoSuperMethodA(cl, obj, (Msg)msg);
			break;
	}

	return retval;
}

IPTR drawerIconObserverDispose(Class *cl, Object *obj, Msg msg)
{
	IPTR retval;

	retval=DoSuperMethodA(cl, obj, msg);

	return retval;
}

IPTR drawerIconObserverExecute(Class *cl, Object *obj, Msg msg)
{
	IPTR retval;
	UBYTE *name, *directory;
	struct TagItem *icTags;
	UBYTE *newDir;
	Object *horiz, *vert, *dirWindow, *iconcontainer, *strip;
	struct DrawerIconObserverClassData *data;
	struct NewMenu menuDat[]=
	{
		{NM_TITLE, "AROS", 0,0,0,(APTR)1},
		{NM_ITEM, "Quit", "Q",0,0,(APTR)2},
		{NM_TITLE, "Icon", 0,0,0,(APTR)30},
		{NM_ITEM, "Open...", "O",0,0,(APTR)31},
		{NM_END, NULL, 0,0,0,(APTR)0}
	};

	data=(struct DrawerIconObserverClassData*)INST_DATA(cl, obj);
	retval=DoSuperMethodA(cl, obj, msg);

	name=_name(obj);
	directory=_directory(obj);

	newDir=AllocVec(strlen(name)+strlen(directory)+2, MEMF_ANY);
	strcpy(newDir, directory);
	if(directory[strlen(directory)-1]!=':')
		strcat(newDir, "/");
	strcat(newDir, name);

	horiz=PropObject,
		MUIA_Prop_Horiz, TRUE,
		MUIA_Prop_Entries, 0,
		MUIA_Prop_UseWinBorder, MUIV_Prop_UseWinBorder_Bottom,
		End;
	vert=PropObject,
		MUIA_Prop_Horiz, FALSE,
		MUIA_Prop_UseWinBorder, MUIV_Prop_UseWinBorder_Right,
		End;

	icTags=AllocVec(sizeof(struct TagItem)*5, MEMF_ANY);
	icTags[0].ti_Tag=MUIA_FillArea;
	icTags[0].ti_Data=FALSE;
	icTags[1].ti_Tag=ICOA_Directory;
	icTags[1].ti_Data=newDir;
	icTags[2].ti_Tag=ICA_VertScroller;
	icTags[2].ti_Data=vert;
	icTags[3].ti_Tag=ICA_HorizScroller;
	icTags[3].ti_Data=horiz;
	icTags[4].ti_Tag=TAG_END;
	icTags[4].ti_Data=0;

	iconcontainer=CreateDesktopObjectA(CDO_IconContainer, icTags);

// TEMPORARY!!!!! Use CreateDesktopObjectA(CDO_Window.....) instead!
	dirWindow=WindowObject,
		MUIA_Window_Width, 300,
		MUIA_Window_Height, 140,
//		MUIA_Window_Menustrip, strip=MUI_MakeObject(MUIO_MenustripNM, menuDat, 0),
		MUIA_Window_UseBottomBorderScroller, TRUE,
		MUIA_Window_UseRightBorderScroller, TRUE,
		MUIA_Window_EraseArea, FALSE,
//		MUIA_Window_Menustrip, strip=MUI_MakeObject(MUIO_MenustripNM, menuDat, 0),
		MUIA_Window_Title, newDir,
		WindowContents, iconcontainer,
//		End,
	End;

	DoMethod(_app(_presentation(obj)), OM_ADDMEMBER, dirWindow);
	DoMethod(dirWindow, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, dirWindow, 3, MUIM_Set, MUIA_Window_Open, FALSE);
	DoMethod(vert, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime, iconcontainer, 3, MUIM_Set, ICA_ScrollToVert, MUIV_TriggerValue);
	DoMethod(horiz, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime, iconcontainer, 3, MUIM_Set, ICA_ScrollToHoriz, MUIV_TriggerValue);

	SetAttrs(dirWindow, MUIA_Window_Open, TRUE, TAG_END);

	retval=1;

	return retval;
}

AROS_UFH3(IPTR, drawerIconObserverDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
{
	ULONG retval=0;

	switch(msg->MethodID)
	{
		case OM_NEW:
			retval=drawerIconObserverNew(cl, obj, (struct opSet*)msg);
			break;
		case OM_SET:
			retval=drawerIconObserverSet(cl, obj, (struct opSet*)msg);
			break;
		case OM_GET:
			retval=drawerIconObserverGet(cl, obj, (struct opGet*)msg);
			break;
		case OM_DISPOSE:
			retval=drawerIconObserverDispose(cl, obj, msg);
			break;
		case IOM_Execute:
			retval=drawerIconObserverExecute(cl, obj, msg);
			break;
		default:
			retval=DoSuperMethodA(cl, obj, msg);
			break;
	}

	return retval;
}

