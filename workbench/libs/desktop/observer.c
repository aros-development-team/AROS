
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
#include "observer.h"

#include "desktop_intern_protos.h"

IPTR observerNew(Class *cl, Object *obj, struct opSet *msg)
{
	IPTR retval=0;
	struct ObserverClassData *data;
	struct TagItem *tag;
	Object *presentation;

	tag=FindTagItem(OA_Presentation, msg->ops_AttrList);
	if(tag)
	{
		tag->ti_Tag=TAG_IGNORE;
		presentation=(Object*)tag->ti_Data;
	}

	retval=DoSuperMethodA(cl, obj, (Msg)msg);
	if(retval)
	{
		obj=(Object*)retval;
		data=INST_DATA(cl, obj);
		data->presentation=presentation;

		DoMethod(presentation, MUIM_Notify, PA_InTree, MUIV_EveryTime, obj, 3, MUIM_Set, OA_InTree, TRUE);
	}

	return retval;
}

IPTR observerSet(Class *cl, Object *obj, struct opSet *msg)
{
	struct ObserverClassData *data;
	IPTR retval=1;
	struct TagItem *tag, *tstate=msg->ops_AttrList;

	data=(struct ObserverClassData*)INST_DATA(cl, obj);

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

IPTR observerGet(Class *cl, Object *obj, struct opGet *msg)
{
	IPTR retval=1;
	struct ObserverClassData *data;

	data=(struct ObserverClassData*)INST_DATA(cl, obj);

	switch(msg->opg_AttrID)
	{
		default:
			retval=DoSuperMethodA(cl, obj, (Msg)msg);
			break;
	}

	return retval;
}

IPTR observerDispose(Class *cl, Object *obj, Msg msg)
{
	IPTR retval;

	retval=DoSuperMethodA(cl, obj, msg);

	return retval;
}

AROS_UFH3(IPTR, observerDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
{
	ULONG retval=0;

	switch(msg->MethodID)
	{
		case OM_NEW:
			retval=observerNew(cl, obj, (struct opSet*)msg);
			break;
		case OM_SET:
			retval=observerSet(cl, obj, (struct opSet*)msg);
			break;
		case OM_GET:
			retval=observerGet(cl, obj, (struct opGet*)msg);
			break;
		case OM_DISPOSE:
			retval=observerDispose(cl, obj, msg);
			break;
		default:
			retval=DoSuperMethodA(cl, obj, msg);
			break;
	}

	return retval;
}

