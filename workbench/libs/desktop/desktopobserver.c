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
#include "desktopobserver.h"
#include "presentation.h"
#include "iconcontainerclass.h"
#include "observer.h"
#include "iconclass.h"

#include "desktop_intern_protos.h"

IPTR desktopObsNew(Class *cl, Object *obj, struct opSet *msg)
{
	IPTR retval=0;
	struct DesktopObserverClassData *data;
	struct TagItem *tag;
	struct Class *defaultWindowClass=NULL;
	struct TagItem *defaultWindowArgs=NULL;

	tag=FindTagItem(DOA_DefaultWindowClass, msg->ops_AttrList);
	if(tag)
	{
		defaultWindowClass=tag->ti_Data;
		// this will change, save the variable in a new
		// desktopcontext area
		DesktopBase->db_DefaultWindow=defaultWindowClass;
		tag->ti_Tag=TAG_IGNORE;
	}

	tag=FindTagItem(DOA_DefaultWindowArguments, msg->ops_AttrList);
	if(tag)
	{
		defaultWindowArgs=tag->ti_Data;
		// this will change, save the variable in a new
		// desktopcontext area
		DesktopBase->db_DefaultWindowArguments=defaultWindowArgs;
		tag->ti_Tag=TAG_IGNORE;
	}

	retval=DoSuperMethodA(cl, obj, (Msg)msg);
	if(retval)
	{
		obj=(Object*)retval;
		data=INST_DATA(cl, obj);
		data->defaultWindow=defaultWindowClass;
		data->defaultWindowArgs=defaultWindowArgs;
	}

	return retval;
}

IPTR desktopObsSet(Class *cl, Object *obj, struct opSet *msg)
{
	struct DesktopObserverClassData *data;
	IPTR retval=1;
	struct TagItem *tag, *tstate=msg->ops_AttrList;

	data=(struct DesktopObserverClassData*)INST_DATA(cl, obj);

	while((tag=NextTagItem(&tstate)))
	{
		switch(tag->ti_Tag)
		{
			case OA_InTree:
			{
				struct HandlerTopLevelRequest *htl;

				htl=createTLScanMessage(DIMC_TOPLEVEL, NULL, LDF_VOLUMES, obj, _app(_presentation(obj)));
				PutMsg(DesktopBase->db_HandlerPort, (struct Message*)htl);
				retval=DoSuperMethodA(cl, obj, (Msg)msg);

				break;
			}
			default:
				retval=DoSuperMethodA(cl, obj, (Msg)msg);
				break;
		}
	}

	return retval;
}

IPTR desktopObsGet(Class *cl, Object *obj, struct opGet *msg)
{
	IPTR retval=1;
	struct DesktopObserverClassData *data;

	data=(struct DesktopObserverClassData*)INST_DATA(cl, obj);

	switch(msg->opg_AttrID)
	{
		default:
			retval=DoSuperMethodA(cl, obj, (Msg)msg);
			break;
	}

	return retval;
}

IPTR desktopObsDispose(Class *cl, Object *obj, Msg msg)
{
	IPTR retval;
	struct DesktopObserverClassData *data;

	data=(struct DesktopObserverClassData*)INST_DATA(cl, obj);
	retval=DoSuperMethodA(cl, obj, msg);

	return retval;
}

IPTR desktopObsAddIcons(Class *cl, Object *obj, struct icoAddIcon *msg)
{
	IPTR retval=0;
	ULONG i;
	Object *newIcon;
	struct TagItem *iconTags;
	ULONG kind;
//	struct DesktopObserverClassData *data;

	for(i=0; i<msg->wsr_Results; i++)
	{
		iconTags=AllocVec(5*sizeof(struct TagItem), MEMF_ANY);
		iconTags[0].ti_Tag=IA_DiskObject;
		iconTags[0].ti_Data=msg->wsr_ResultsArray[i].sr_DiskObject;
		iconTags[1].ti_Tag=IA_Label;
		iconTags[1].ti_Data=msg->wsr_ResultsArray[i].sr_Name;
//		iconTags[2].ti_Tag=IA_Directory;
//		iconTags[2].ti_Data=data->directory;
		iconTags[2].ti_Tag=MUIA_Draggable;
		iconTags[2].ti_Data=TRUE;
		iconTags[3].ti_Tag=IA_Desktop;
		iconTags[3].ti_Data=_presentation(obj);
		iconTags[4].ti_Tag=TAG_END;
		iconTags[4].ti_Data=0;

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

AROS_UFH3(IPTR, desktopObserverDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
{
	ULONG retval=0;

	switch(msg->MethodID)
	{
		case OM_NEW:
			retval=desktopObsNew(cl, obj, (struct opSet*)msg);
			break;
		case OM_SET:
			retval=desktopObsSet(cl, obj, (struct opSet*)msg);
			break;
		case OM_GET:
			retval=desktopObsGet(cl, obj, (struct opGet*)msg);
			break;
		case OM_DISPOSE:
			retval=desktopObsDispose(cl, obj, msg);
			break;
		case ICOM_AddIcons:
			retval=desktopObsAddIcons(cl, obj, (struct icoAddIcon*)msg);
			break;
		default:
			retval=DoSuperMethodA(cl, obj, msg);
			break;
	}

	return retval;
}

