
#define DEBUG 1
#include <aros/debug.h>

#include <exec/types.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <libraries/mui.h>

#include "support.h"
#include "worker.h"
#include "desktop_intern.h"

#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "presentation.h"
#include "diskiconobserver.h"

#include "desktop_intern_protos.h"

IPTR diskIconObserverNew(Class *cl, Object *obj, struct opSet *msg)
{
	IPTR retval=0;
	struct DiskIconObserverClassData *data;
	struct TagItem *tag;

	retval=DoSuperMethodA(cl, obj, (Msg)msg);
	if(retval)
	{
		obj=(Object*)retval;
		data=INST_DATA(cl, obj);
	}

	return retval;
}

IPTR diskIconObserverSet(Class *cl, Object *obj, struct opSet *msg)
{
	struct DiskIconObserverClassData *data;
	IPTR retval=1;
	struct TagItem *tag, *tstate=msg->ops_AttrList;

	data=(struct DiskIconObserverClassData*)INST_DATA(cl, obj);

	while(tag=NextTagItem(&tstate))
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

IPTR diskIconObserverGet(Class *cl, Object *obj, struct opGet *msg)
{
	IPTR retval=1;
	struct DiskIconObserverClassData *data;

	data=(struct DiskIconObserverClassData*)INST_DATA(cl, obj);

	switch(msg->opg_AttrID)
	{
		default:
			retval=DoSuperMethodA(cl, obj, (Msg)msg);
			break;
	}

	return retval;
}

IPTR diskIconObserverDispose(Class *cl, Object *obj, Msg msg)
{
	IPTR retval;

	retval=DoSuperMethodA(cl, obj, msg);

	return retval;
}

AROS_UFH3(IPTR, diskIconObserverDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
{
	ULONG retval=0;

	switch(msg->MethodID)
	{
		case OM_NEW:
			retval=diskIconObserverNew(cl, obj, (struct opSet*)msg);
			break;
		case OM_SET:
			retval=diskIconObserverSet(cl, obj, (struct opSet*)msg);
			break;
		case OM_GET:
			retval=diskIconObserverGet(cl, obj, (struct opGet*)msg);
			break;
		case OM_DISPOSE:
			retval=diskIconObserverDispose(cl, obj, msg);
			break;
		default:
			retval=DoSuperMethodA(cl, obj, msg);
			break;
	}

	return retval;
}

