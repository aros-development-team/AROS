#include <aros/asmcall.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <libraries/gadtools.h>
#include <libraries/mui.h>

#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include <stdio.h>

#include "wbiconclass.h"
#include "wbwindowclass.h"
#include "tooliconclass.h"
#include "wbobjectclass.h"
#include "wbapplicationclass.h"
#include "wbwindowdefaultpresclass.h"

extern struct MUI_CustomClass *WBToolIconClass;
extern struct MUI_CustomClass *WBWindowDefaultPresentationClass;
extern struct MUI_CustomClass *WBDrawerIconClass;

/*** TEMPORARY ***/
void scanDirectory(char *directoryPath, Object *obj, struct MsgPort *deliverResults)
{
	char *buffer;
	BPTR dir;
	struct ExAllControl *eac;
	BOOL more;
	struct ExAllData *ead;
	struct WBInterMessage *wbim;

	// hack
	buffer=(STRPTR)AllocVec(100000, MEMF_ANY);
	dir=Lock(directoryPath, ACCESS_READ);
	eac=(struct ExAllControl*)AllocDosObject(DOS_EXALLCONTROL, NULL);
	eac->eac_LastKey=0;

	do
	{
		more=ExAll(dir, (struct ExAllData*)buffer, 100000, ED_OWNER, eac);
		ead=(struct ExAllData*)buffer;

		do
		{
			wbim=(struct WBInterMessage*)AllocVec(sizeof(struct WBInterMessage), MEMF_ANY);
			wbim->im_message.mn_Node.ln_Type=NT_MESSAGE;
			wbim->im_message.mn_ReplyPort=NULL;
			wbim->im_message.mn_Length=sizeof(struct WBInterMessage);
			wbim->requester=obj;
			wbim->methodID=WBM_Window_AddFile;
			wbim->numArgs=1;
			wbim->args=(ULONG*)AllocVec(sizeof(ULONG), MEMF_ANY);
			*wbim->args=(ULONG)ead;

			PutMsg(deliverResults, (struct Message*)wbim);

			ead=ead->ed_Next;
		}
		while(ead);
	}
	while(more);

	FreeDosObject(DOS_EXALLCONTROL, eac);
	UnLock(dir);
}
/*** END TEMPORARY  ***/

ULONG wbWindowNew(Class *cl, Object *obj, struct opSet *ops)
{
	ULONG retval=0;
	struct TagItem *tag;
	struct WBWindowClassData *data;
	char *directoryPath;
	Object *strip, *presentation;
	Object *horiz, *vert;
	struct NewMenu menuDat[]=
	{
		{NM_TITLE, "AROS", 0,0,0,(APTR)1},
		{NM_ITEM, "Quit", "Q",0,0,(APTR)2},
		{NM_END, NULL, 0,0,0,(APTR)0}
	};

	tag=FindTagItem(WBA_Window_Directory, ops->ops_AttrList);
	if(tag)
	{
		tag->ti_Tag=TAG_IGNORE;
		directoryPath=(char*)tag->ti_Data;
	}

	retval=DoSuperMethodA(cl, obj, (Msg)ops);
	if(retval)
	{
		obj=(Object*)retval;

		data=(struct WBWindowClassData*)INST_DATA(cl, obj);

		horiz=PropObject,
			MUIA_Prop_Horiz, TRUE,
			MUIA_Prop_Entries, 0,
			MUIA_Prop_UseWinBorder, MUIV_Prop_UseWinBorder_Bottom,
			End,
		vert=PropObject,
			MUIA_Prop_Horiz, FALSE,
			MUIA_Prop_UseWinBorder, MUIV_Prop_UseWinBorder_Right,
			End,

		data->zuneWindow=(Object*)WindowObject,
			MUIA_Window_Menustrip, strip=MUI_MakeObject(MUIO_MenustripNM, menuDat, 0),
			MUIA_Window_Title, directoryPath,
			MUIA_Window_UseBottomBorderScroller, TRUE,
			MUIA_Window_UseRightBorderScroller, TRUE,
			WindowContents, presentation=(Object*)NewObject(WBWindowDefaultPresentationClass->mcc_Class, NULL,
//				Child, vert,
//				Child, horiz,
				WBA_WinPresentation_HorizSize, horiz,
				WBA_WinPresentation_VertSize, vert,
				WBA_WinPresentation_SemanticObject, obj,
				End,
			End;

		SetAttrs(obj, WBA_Object_Presentation, presentation, TAG_END);

//		DoMethod(horiz, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime, presentation, 3, MUIM_Set, WBA_WinPresentation_ViewPointX, MUIV_TriggerValue);
//		DoMethod(vert, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime, presentation, 3, MUIM_Set, WBA_WinPresentation_ViewPointY, MUIV_TriggerValue);

		data->open=FALSE;
		data->directoryPath=directoryPath;
		data->multiSelectMode=FALSE;
		NewList(&data->selectionList);
		NewList(&data->iconList);
	}

	return retval;
}

ULONG wbWindowGet(Class *cl, Object *obj, struct opGet *opg)
{
	ULONG retval=0;
	struct WBWindowClassData *data;

	data=(struct WBWindowClassData*)INST_DATA(cl, obj);

	switch(opg->opg_AttrID)
	{
		case WBA_Window_Open:
			*opg->opg_Storage=(ULONG)data->open;
			break;
		case WBA_Window_Directory:
			*opg->opg_Storage=(ULONG)data->directoryPath;
			break;
		case WBA_Window_Window:
			*opg->opg_Storage=(ULONG)data->zuneWindow;
			break;
		default:
			retval=DoSuperMethodA(cl, obj, (Msg)opg);
			break;
	}

	return retval;
}

ULONG wbWindowSet(Class *cl, Object *obj, struct opSet *ops)
{
	ULONG retval=0;
	struct WBWindowClassData *data;
	struct TagItem *tag;
	struct MsgPort *wbPort;
	ULONG thisID=0;
	BOOL changedParent=FALSE;
	Object *parent;

	data=(struct WBWindowClassData*)INST_DATA(cl, obj);

	tag=FindTagItem(WBA_Window_Open, ops->ops_AttrList);
	if(tag)
	{
		tag->ti_Tag=TAG_IGNORE;
		data->open=(BOOL)tag->ti_Data;

		SetAttrs(data->zuneWindow, MUIA_Window_Open, data->open, TAG_END);
	}

	tag=FindTagItem(WBA_Object_Parent, ops->ops_AttrList);
	if(tag)
	{
		changedParent=TRUE;
	}

	retval=DoSuperMethodA(cl, obj, (Msg)ops);

	if(changedParent)
	{
		/* this window has been added to an application.. */
		/* request to scan the directory  */
		GetAttr(WBA_Object_Parent, obj, (ULONG*)&parent);
		GetAttr(WBA_Application_MsgPort, parent, (ULONG*)&wbPort);

//		GetAttr(WBA_Object_ID, obj, &thisID);
		scanDirectory(data->directoryPath, obj, wbPort);
	}

	return retval;
}

ULONG wbWindowDispose(Class *cl, Object *obj, Msg msg)
{
	struct WBWindowClassData *data;
	ULONG retval;

	data=(struct WBWindowClassData*)INST_DATA(cl, obj);

	retval=DoSuperMethodA(cl, obj, msg);

	return retval;
}

Object* createIcon(struct ExAllData *exall)
{
	Object *icon=NULL;

	// hmmmmmm... what do we do about tools & projects???
	switch(exall->ed_Type)
	{
		case 2:
			icon=(Object*)NewObject(WBDrawerIconClass->mcc_Class, NULL,
				WBA_Icon_Name, exall->ed_Name,
				WBA_Icon_Type, exall->ed_Type,
				End;
			break;
		default:
			icon=(Object*)NewObject(WBToolIconClass->mcc_Class, NULL,
				WBA_Icon_Name, exall->ed_Name,
				WBA_Icon_Type, exall->ed_Type,
				End;
			break;
	}

	return icon;
}

ULONG wbWindowAddFile(Class *cl, Object *obj, struct WBAddFileMethodData* msg)
{
	ULONG retval=0;
	Object *icon;

	icon=createIcon(*msg->args);
	DoMethod(obj, OM_ADDMEMBER, icon, TAG_END);

	return retval;
}

ULONG wbWindowMSet(Class *cl, Object *obj, struct MUIP_Set *msg)
{
	return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR wbWindowAdd(Class *cl, Object *obj, struct opMember *msg)
{
	IPTR retval=0;
	Object *thisPres, *pres;
	struct WBWindowClassData *data;
	struct IconNode *iconNode;

	retval=DoSuperMethodA(cl, obj, (Msg)msg);

	data=(struct WBWindowClassData*)INST_DATA(cl, obj);

	iconNode=AllocVec(sizeof(struct IconNode), MEMF_ANY);
	iconNode->i_member=msg->opam_Object;

	AddTail(&data->iconList, (struct Node*)iconNode);

	DoMethod(msg->opam_Object, WBM_Object_Added);
//	DoMethod(msg->opam_Object, MUIM_Notify, WBA_Icon_Selected, TRUE, obj, 2, WBM_Window_ProcessIconSelection, msg->opam_Object);

	return retval;
}

IPTR wbWindowAdded(Class *cl, Object *obj, Msg msg)
{
	IPTR retval=0;
	Object *pres;

	retval=DoSuperMethodA(cl, obj, msg);

	GetAttr(WBA_Object_Presentation, obj, &pres);
	DoMethod(pres, MUIM_Notify, WBA_WinPresentation_Clicked, MUIV_EveryTime, obj, 2, WBM_Window_HandleInput, MUIV_TriggerValue);

	return retval;
}

/*
This function is a bit crap, it is too centralized, however my excuse
is that it is temporary.  The IO will be farmed out to the icons
which should make things a bit more extensible - if a little slower.
And this will function will disappear.
*/

IPTR wbWindowHandleInput(Class *cl, Object *obj, struct HandleInputMethodData *msg)
{
	IPTR retval=0;
	BOOL done=FALSE;
	struct IconNode *in;
	struct IconSelectedNode *oldMember, *member;
	struct WBWindowClassData *data;
	ULONG iconReturn;

	data=(struct WBWindowClassData*)INST_DATA(cl, obj);

	in=data->iconList.lh_Head;
	while(in->i_Node.ln_Succ && !done)
	{
		iconReturn=DoMethod(in->i_member, WBM_Icon_HandleInput, msg->imsg);
		if(iconReturn)
		{
			if(!data->multiSelectMode)
			{
				while(!IsListEmpty(&data->selectionList))
				{
					oldMember=(struct MemberNode*)data->selectionList.lh_Head;
					if(oldMember->is_member!=in->i_member)
						SetAttrs(oldMember->is_member, WBA_Icon_Selected, FALSE, TAG_END);
					Remove((struct Node*)oldMember);
				}
			}

			member=AllocVec(sizeof(struct IconSelectedNode), MEMF_ANY);
			member->is_member=in->i_member;

			AddTail(&data->selectionList, (struct Node*)member);

			done=TRUE;
		}
		else
			in=in->i_Node.ln_Succ;
	}

	if(done==FALSE)
	{
		while(!IsListEmpty(&data->selectionList))
		{
			oldMember=(struct MemberNode*)data->selectionList.lh_Head;
			SetAttrs(oldMember->is_member, WBA_Icon_Selected, FALSE, TAG_END);
			Remove((struct Node*)oldMember);
		}

	}

	return retval;
}

AROS_UFH3(IPTR,wbWindowDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
{
	ULONG retval;

	switch(msg->MethodID)
	{
//		case MUIM_Set:
//			retval=wbWindowMSet(cl, obj, (MUIP_Set*)msg);
//			break;
		case WBM_Window_AddFile:
			retval=wbWindowAddFile(cl, obj, (struct WBAddFileMethodData*)msg);
			break;
		case OM_ADDMEMBER:
			retval=wbWindowAdd(cl, obj, (struct opMember*)msg);
			break;
		case OM_NEW:
			retval=wbWindowNew(cl, obj, (struct opSet*)msg);
			break;
		case OM_GET:
			retval=wbWindowGet(cl, obj, (struct opGet*)msg);
			break;
		case OM_SET:
			retval=wbWindowSet(cl, obj, (struct opSet*)msg);
			break;
		case OM_DISPOSE:
			retval=wbWindowDispose(cl, obj, msg);
			break;
		case WBM_Object_Added:
			retval=wbWindowAdded(cl, obj, msg);
			break;
		case WBM_Window_HandleInput:
			retval=wbWindowHandleInput(cl, obj, (struct HandleInputMethodData*)msg);
			break;
		default:
			retval=DoSuperMethodA(cl, obj, msg);
			break;
	}

	return retval;
}




