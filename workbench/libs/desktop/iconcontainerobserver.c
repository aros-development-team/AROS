
#define DEBUG 1
#include <aros/debug.h>

#include <exec/types.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>

#include "support.h"
#include "worker.h"
#include "desktop_intern.h"
#include "iconcontainerobserver.h"

#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "desktop_intern_protos.h"

IPTR iconConObsNew(Class *cl, Object *obj, struct opSet *msg)
{
	IPTR retval=0;
	struct IconContainerObserverClassData *data;
	struct TagItem *tag;
	Object *presentation;
	UBYTE *directory;

	tag=FindTagItem(ICOA_Presentation, msg->ops_AttrList);
	if(tag)
	{
		tag->ti_Tag=TAG_IGNORE;
		presentation=(Object*)tag->ti_Data;
	}

	tag=FindTagItem(ICOA_Directory, msg->ops_AttrList);
	if(tag)
	{
		tag->ti_Tag=TAG_IGNORE;
		directory=(UBYTE*)tag->ti_Data;
	}

	retval=DoSuperMethodA(cl, obj, (Msg)msg);
	if(retval)
	{
		struct HandlerScanRequest *hsr;

		obj=(Object*)retval;
		data=INST_DATA(cl, obj);
		data->presentation=presentation;
		data->directory=directory;
		data->dirLock=Lock(directory, ACCESS_READ);

		hsr=createScanMessage(DIMC_SCANDIRECTORY, NULL, data->dirLock, obj);
		PutMsg(DesktopBase->db_HandlerPort, (struct Message*)hsr);
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
		default:
			retval=DoSuperMethodA(cl, obj, msg);
			break;
	}

	return retval;
}

