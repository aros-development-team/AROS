
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
		data->dirLock=Lock(directory, ACCESS_READ);

		DoMethod(_presentation(obj), MUIM_Notify, PA_InTree, MUIV_EveryTime, obj, 3, MUIM_Set, OA_InTree, TRUE);
	}

	return retval;
}

IPTR iconConObsSet(Class *cl, Object *obj, struct opSet *msg)
{
	struct IconContainerObserverClassData *data;
	IPTR retval=1;
	struct TagItem *tag, *tstate=msg->ops_AttrList;

	data=(struct IconContainerObserverClassData*)INST_DATA(cl, obj);

	while(tag=NextTagItem(&tstate))
    {
		switch(tag->ti_Tag)
		{
			case ICOA_Directory:
				UnLock(data->dirLock);
				data->directory=(UBYTE*)tag->ti_Data;
				data->dirLock=Lock(data->directory, ACCESS_READ);
				break;
			case OA_InTree:
			{
				struct HandlerScanRequest *hsr;
				hsr=createScanMessage(DIMC_SCANDIRECTORY, NULL, data->dirLock, obj, _app(_presentation(obj)));
				PutMsg(DesktopBase->db_HandlerPort, (struct Message*)hsr);

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

	for(i=0; i<msg->wsr_Results; i++)
	{
		iconTags=AllocVec(3*sizeof(struct TagItem), MEMF_ANY);
		iconTags[0].ti_Tag=IA_DiskObject;
		iconTags[0].ti_Data=msg->wsr_ResultsArray[i].sr_DiskObject;
		iconTags[1].ti_Tag=IA_Label;
		iconTags[1].ti_Data=msg->wsr_ResultsArray[i].sr_Name;
		iconTags[2].ti_Tag=TAG_END;
		iconTags[2].ti_Data=0;

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

		DoMethod(_presentation(obj), OM_ADDMEMBER, newIcon);
	}

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

