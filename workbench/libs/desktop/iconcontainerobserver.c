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
#include <libraries/mui.h>
#include <workbench/workbench.h>

#include <proto/dos.h>
#include <proto/desktop.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "support.h"
#include "worker.h"
#include "desktop_intern.h"
#include "iconcontainerobserver.h"
#include "presentation.h"
#include "iconcontainerclass.h"
#include "iconobserver.h"
#include "observer.h"
#include "iconclass.h"

#include "desktop_intern_protos.h"

IPTR iconConObsNew(Class *cl, Object *obj, struct opSet *msg)
{
	IPTR retval=0;
	struct IconContainerObserverClassData *data;
	struct TagItem *tag;
	UBYTE *directory;

	tag=FindTagItem(ICOA_Directory, msg->ops_AttrList);
	if(tag)
	{
		tag->ti_Tag=TAG_IGNORE;
		directory=(UBYTE*)tag->ti_Data;
	}

	retval=DoSuperMethodA(cl, obj, (Msg)msg);
	if(retval)
	{
		obj=(Object*)retval;
		data=INST_DATA(cl, obj);
		data->directory=directory;
		// this is unlocked by the scanner worker
		data->dirLock=Lock(directory, ACCESS_READ);
	}

	return retval;
}

IPTR iconConObsSet(Class *cl, Object *obj, struct opSet *msg)
{
	struct IconContainerObserverClassData *data;
	IPTR retval=1;
	struct TagItem *tag, *tstate=msg->ops_AttrList;

	data=(struct IconContainerObserverClassData*)INST_DATA(cl, obj);

	while((tag=NextTagItem(&tstate)))
	{
		switch(tag->ti_Tag)
		{
			case ICOA_Directory:
				data->directory=(UBYTE*)tag->ti_Data;
				break;
			case OA_InTree:
			{
				struct HandlerScanRequest *hsr;

				hsr=createScanMessage(DIMC_SCANDIRECTORY, NULL, data->dirLock, obj, _app(_presentation(obj)));
				PutMsg(DesktopBase->db_HandlerPort, (struct Message*)hsr);

				retval=DoSuperMethodA(cl, obj, (Msg)msg);
				DoMethod(_presentation(obj), MUIM_KillNotify, PA_InTree);

				break;
			}
			default:
				retval=DoSuperMethodA(cl, obj, (Msg)msg);
				break;
		}
	}

	return retval;
}

IPTR iconConObsGet(Class *cl, Object *obj, struct opGet *msg)
{
	IPTR retval=1;
	struct IconContainerObserverClassData *data;

	data=(struct IconContainerObserverClassData*)INST_DATA(cl, obj);

	switch(msg->opg_AttrID)
	{
		case ICOA_Directory:
			*msg->opg_Storage=(ULONG)data->directory;
			break;
		default:
			retval=DoSuperMethodA(cl, obj, (Msg)msg);
			break;
	}

	return retval;
}

IPTR iconConObsDispose(Class *cl, Object *obj, Msg msg)
{
	IPTR retval;
	struct IconContainerObserverClassData *data;

	data=(struct IconContainerObserverClassData*)INST_DATA(cl, obj);
	UnLock(data->dirLock);
	retval=DoSuperMethodA(cl, obj, msg);

	return retval;
}

IPTR iconConObsAddIcons(Class *cl, Object *obj, struct icoAddIcon *msg)
{
	IPTR retval=0;
	ULONG i;
	Object *newIcon;
	struct TagItem *iconTags;
	ULONG kind;
	struct IconContainerObserverClassData *data;
	Object *desktop=NULL;

	data=(struct IconContainerObserverClassData*)INST_DATA(cl, obj);

	GetAttr(ICA_Desktop, _presentation(obj), &desktop);

	for(i=0; i<msg->wsr_Results; i++)
	{

		iconTags=AllocVec(18*sizeof(struct TagItem), MEMF_ANY);

		iconTags[0].ti_Tag=IA_DiskObject;
		iconTags[0].ti_Data=msg->wsr_ResultsArray[i].sr_DiskObject;
		iconTags[1].ti_Tag=IA_Label;
		iconTags[1].ti_Data=msg->wsr_ResultsArray[i].sr_Name;
		iconTags[2].ti_Tag=IOA_Name;
		iconTags[2].ti_Data=msg->wsr_ResultsArray[i].sr_Name;
		iconTags[3].ti_Tag=IOA_Directory;
		iconTags[3].ti_Data=data->directory;
		iconTags[4].ti_Tag=IOA_Comment;
		iconTags[4].ti_Data=msg->wsr_ResultsArray[i].sr_Comment;
		iconTags[5].ti_Tag=IOA_Script;
		iconTags[5].ti_Data=msg->wsr_ResultsArray[i].sr_Script;
		iconTags[6].ti_Tag=IOA_Pure;
		iconTags[6].ti_Data=msg->wsr_ResultsArray[i].sr_Pure;
		iconTags[7].ti_Tag=IOA_Readable;
		iconTags[7].ti_Data=msg->wsr_ResultsArray[i].sr_Read;
		iconTags[8].ti_Tag=IOA_Writeable;
		iconTags[8].ti_Data=msg->wsr_ResultsArray[i].sr_Write;
		iconTags[9].ti_Tag=IOA_Archived;
		iconTags[9].ti_Data=msg->wsr_ResultsArray[i].sr_Archive;
		iconTags[10].ti_Tag=IOA_Executable;
		iconTags[10].ti_Data=msg->wsr_ResultsArray[i].sr_Execute;
		iconTags[11].ti_Tag=IOA_Deleteable;
		iconTags[11].ti_Data=msg->wsr_ResultsArray[i].sr_Delete;
		iconTags[12].ti_Tag=MUIA_Draggable;
		iconTags[12].ti_Data=TRUE;
		iconTags[13].ti_Tag=IA_Size;
		iconTags[13].ti_Data=msg->wsr_ResultsArray[i].sr_Size;
		iconTags[14].ti_Tag=IA_LastModified;
		iconTags[14].ti_Data=&msg->wsr_ResultsArray[i].sr_LastModified;
		iconTags[15].ti_Tag=IA_Type;
		iconTags[15].ti_Data=msg->wsr_ResultsArray[i].sr_Type;
		iconTags[16].ti_Tag=IA_Desktop;
		iconTags[16].ti_Data=desktop;
		iconTags[17].ti_Tag=TAG_END;
		iconTags[17].ti_Data=0;

		switch(msg->wsr_ResultsArray[i].sr_DiskObject->do_Type)
		{
			case WBDISK:
				kind=CDO_DiskIcon;
				break;
			case WBDRAWER:
				kind=CDO_DrawerIcon;
				break;
			case WBTOOL:
				kind=CDO_ToolIcon;
				break;
			case WBPROJECT:
				kind=CDO_ProjectIcon;
				break;
			case WBGARBAGE:
				kind=CDO_TrashcanIcon;
				break;
			case WBDEVICE:
				break;
			case WBKICK:
				break;
			case WBAPPICON:
				break;
			default:
				// something serious has gone wrong here
				break;
		}


		newIcon=CreateDesktopObjectA(kind, iconTags);

			FreeVec(iconTags);

		DoMethod(_presentation(obj), OM_ADDMEMBER, newIcon);
	}

	FreeVec(msg->wsr_ResultsArray);


	return retval;
}

AROS_UFH3(IPTR, iconContainerObserverDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
{
	ULONG retval=0;

	switch(msg->MethodID)
	{
		case OM_NEW:
			retval=iconConObsNew(cl, obj, (struct opSet*)msg);
			break;
		case OM_SET:
			retval=iconConObsSet(cl, obj, (struct opSet*)msg);
			break;
		case OM_GET:
			retval=iconConObsGet(cl, obj, (struct opGet*)msg);
			break;
		case OM_DISPOSE:
			retval=iconConObsDispose(cl, obj, msg);
			break;
		case ICOM_AddIcons:
			retval=iconConObsAddIcons(cl, obj, (struct icoAddIcon*)msg);
			break;
		default:
			retval=DoSuperMethodA(cl, obj, msg);
			break;
	}

	return retval;
}

