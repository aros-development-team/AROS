#include <aros/asmcall.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <libraries/mui.h>

#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include <stdio.h>

#include "wbapplicationclass.h"
#include "wbobjectclass.h"
#include "wbwindowclass.h"
#include "wbwindowdefaultpresclass.h"

ULONG wbApplicationNew(Class *cl, Object *obj, struct opSet *ops)
{
	ULONG retval=0;
	struct WBApplicationClassData *data;

	retval=DoSuperMethodA(cl, obj, (Msg)ops);
	if(retval)
	{
		obj=(Object*)retval;
		data=(struct WBApplicationClassData*)INST_DATA(cl, obj);

		data->zuneApplication=ApplicationObject,
			MUIA_Application_Title, "AROS Desktop test program",
		End;

		NewList(&data->windowList);
		data->wbPort=CreatePort(NULL, 0);

		SetAttrs(obj, WBA_Object_Presentation, data->zuneApplication, TAG_END);
	}

	return retval;
}

ULONG wbApplicationDispose(Class *cl, Object *obj, Msg msg)
{
	struct WBApplicationClassData *data;
	ULONG retval;

	data=(struct WBApplicationClassData*)INST_DATA(cl, obj);
	DeletePort(data->wbPort);

	retval=DoSuperMethodA(cl, obj, msg);

	return retval;
}

/* The window class is a special case. Its presentation object
   is not a zune window object, but the highest level group
   object that belongs to the zune window object. So when we
   add windows to this app, the default behaviour would be
   to add the presentation object, but we don't want this, we
   want to add the window-presentation object instead.
*/
ULONG wbApplicationAdd(Class *cl, Object *obj, struct opMember *opm)
{
	ULONG retval=0;
	struct WBApplicationClassData *data;
	Object *zuneWindow;
	Object *winPres;
	struct WindowNode *newWindow;

	data=(struct WBApplicationClassData*)INST_DATA(cl, obj);

	SetAttrs(opm->opam_Object, WBA_Object_Parent, obj, TAG_END);
	GetAttr(WBA_Window_Window, opm->opam_Object, (ULONG*)&zuneWindow);
	DoMethod(data->zuneApplication, OM_ADDMEMBER, zuneWindow, TAG_END);

	newWindow=(struct WindowNode*)AllocVec(sizeof(struct WindowNode), MEMF_ANY);
	newWindow->wn_window=opm->opam_Object;
	AddTail(&data->windowList, (struct Node*)newWindow);

	DoMethod(opm->opam_Object, WBM_Object_Added);

	return retval;
}

ULONG wbApplicationInput(Class *cl, Object *obj, struct WBAppInputData *wbaid)
{
	struct WBApplicationClassData *data;
	ULONG retval;
	ULONG *hackeryRUs;
	struct WBInterMessage *wbim;

	data=(struct WBApplicationClassData*)INST_DATA(cl, obj);

	if(wbaid->breakSig & (1L << data->wbPort->mp_SigBit))
	{
		while(wbim=((struct WBInterMessage*)GetMsg(data->wbPort)))
			DoMethod(wbim->requester, wbim->methodID, wbim->numArgs, wbim->args, TAG_END);
	}
	else
		retval=DoMethod(data->zuneApplication, MUIM_Application_Input, wbaid->signals);

	*wbaid->signals|=(1L << data->wbPort->mp_SigBit);

//printf("26\n");
//Delay(200);


	return retval;
}

ULONG wbApplicationGet(Class *cl, Object *obj, struct opGet *opg)
{
	ULONG retval=0;
	struct WBApplicationClassData *data;

	data=(struct WBApplicationClassData*)INST_DATA(cl, obj);

	switch(opg->opg_AttrID)
	{
		case WBA_Application_MsgPort:
			*opg->opg_Storage=(ULONG)data->wbPort;
			retval=1;
			break;
		default:
			retval=DoSuperMethodA(cl, obj, (Msg)opg);
			break;
	}

	return retval;
}

AROS_UFH3(IPTR,wbApplicationDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
{
	ULONG retval=0;

	switch(msg->MethodID)
	{
		case WBM_Application_Input:
			retval=wbApplicationInput(cl, obj, (struct WBAppInputData*)msg);
			break;
		case OM_ADDMEMBER:
			retval=wbApplicationAdd(cl, obj, (struct opMember*)msg);
			break;
		case OM_NEW:
			retval=wbApplicationNew(cl, obj, (struct opSet*)msg);
			break;
		case OM_GET:
			retval=wbApplicationGet(cl, obj, (struct opGet*)msg);
			break;
		case OM_DISPOSE:
			retval=wbApplicationDispose(cl, obj, msg);
			break;
		default:
			retval=DoSuperMethodA(cl, obj, msg);
			break;
	}

	return retval;
}




