#include <aros/asmcall.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <libraries/mui.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include "wbobjectclass.h"
#include "wbiconclass.h"
#include "wbwindowclass.h"
#include "wbdrawericonclass.h"

#include <stdio.h>
#include <string.h>

extern struct MUI_CustomClass *WBWindowClass;

IPTR drawerExecute(Class *cl, Object *obj, Msg msg)
{
	// TEMPORARY!!!!
	// A WBA_Object_Application attribute is needed, and
	// this lot will be moved into a library function
	Object *win1;
	Object *parentApp, *parentWin;
	char *directory, *name;
	char *newDirectory;
	IPTR retval=0;

	GetAttr(WBA_Object_Parent, obj, &parentWin);
	GetAttr(WBA_Object_Parent, parentWin, &parentApp);

	GetAttr(WBA_Window_Directory, parentWin, &directory);
	GetAttr(WBA_Icon_Name, obj, &name);
	// temporary
	newDirectory=AllocVec(1000, MEMF_ANY);
	strcpy(newDirectory, directory);
	strcat(newDirectory, name);

	// more temporary hacking
	if(newDirectory[strlen(newDirectory)-1]!='/' && newDirectory[strlen(newDirectory)-1]!=':')
	{
		newDirectory[strlen(newDirectory)+1]=0;
		newDirectory[strlen(newDirectory)]='/';
	}

	win1=(Object*)NewObject(WBWindowClass->mcc_Class, NULL,
		WBA_Window_Directory, newDirectory,
		End;

	DoMethod(parentApp, OM_ADDMEMBER, win1, TAG_END);
	SetAttrs(win1, WBA_Window_Open, TRUE, TAG_DONE);

	return retval;
}

AROS_UFH3(IPTR,wbDrawerIconClassDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
{
	ULONG retval;

	switch(msg->MethodID)
	{
		case WBM_Icon_Execute:
			retval=drawerExecute(cl, obj, msg);
			break;
		default:
			retval=DoSuperMethodA(cl, obj, msg);
			break;
	}

	return retval;
}

