/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/

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
#include <proto/icon.h>
#include <proto/utility.h>

#include "presentation.h"
#include "observer.h"
#include "iconclass.h"
#include "iconobserver.h"

#include "desktop_intern_protos.h"

IPTR iconObserverNew(Class *cl, Object *obj, struct opSet *msg)
{
	IPTR retval=0;
	struct IconObserverClassData *data;
	struct TagItem *tag;
	UBYTE *name=NULL, *directory=NULL;
	BOOL selected=FALSE;

	tag=FindTagItem(IOA_Selected, msg->ops_AttrList);
	if(tag)
	{
		selected=tag->ti_Data;
		tag->ti_Tag=TAG_IGNORE;
	}

	tag=FindTagItem(IOA_Name, msg->ops_AttrList);
	if(tag)
	{
		name=tag->ti_Data;
		tag->ti_Tag=TAG_IGNORE;
	}

	tag=FindTagItem(IOA_Directory, msg->ops_AttrList);
	if(tag)
	{
		directory=tag->ti_Data;
		tag->ti_Tag=TAG_IGNORE;
	}

	retval=DoSuperMethodA(cl, obj, (Msg)msg);
	if(retval)
	{
		obj=(Object*)retval;
		data=INST_DATA(cl, obj);
		data->selected=selected;
		data->name=name;
		data->directory=directory;

		DoMethod(_presentation(obj), MUIM_Notify, IA_Executed, TRUE, obj, 1, IOM_Execute);
		DoMethod(_presentation(obj), MUIM_Notify, IA_Selected, MUIV_EveryTime, obj, 3, MUIM_Set, IOA_Selected, MUIV_TriggerValue);
		DoMethod(_presentation(obj), MUIM_Notify, IA_Directory, MUIV_EveryTime, obj, 3, MUIM_Set, IOA_Directory, MUIV_TriggerValue);
	}

	return retval;
}

IPTR iconObserverSet(Class *cl, Object *obj, struct opSet *msg)
{
	struct IconObserverClassData *data;
	IPTR retval=1;
	struct TagItem *tag, *tstate=msg->ops_AttrList;

	data=(struct IconObserverClassData*)INST_DATA(cl, obj);

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

IPTR iconObserverGet(Class *cl, Object *obj, struct opGet *msg)
{
	IPTR retval=1;
	struct IconObserverClassData *data;

	data=(struct IconObserverClassData*)INST_DATA(cl, obj);

	switch(msg->opg_AttrID)
	{
		case IOA_Name:
			*msg->opg_Storage=data->name;
			break;
		default:
			retval=DoSuperMethodA(cl, obj, (Msg)msg);
			break;
	}

	return retval;
}

IPTR iconObserverDispose(Class *cl, Object *obj, Msg msg)
{
	IPTR retval;
	struct IconObserverClassData *data;

	data=(struct IconObserverClassData*)INST_DATA(cl, obj);

	FreeDiskObject(_diskobject(_presentation(obj)));
// note:
//	IconObserverClassData.name is part of the ExAllBuffer and is freed
//   by this object's parent
//  IconObserverClassData.directory belongs to this object's parent, and
//  is freed by it

	retval=DoSuperMethodA(cl, obj, msg);

	return retval;
}

IPTR iconObserverExecute(Class *cl, Object *obj, Msg msg)
{
	return 0;
}

AROS_UFH3(IPTR, iconObserverDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
{
	ULONG retval=0;

	switch(msg->MethodID)
	{
		case OM_NEW:
			retval=iconObserverNew(cl, obj, (struct opSet*)msg);
			break;
		case OM_SET:
			retval=iconObserverSet(cl, obj, (struct opSet*)msg);
			break;
		case OM_GET:
			retval=iconObserverGet(cl, obj, (struct opGet*)msg);
			break;
		case OM_DISPOSE:
			retval=iconObserverDispose(cl, obj, msg);
			break;
		case IOM_Execute:
			retval=iconObserverExecute(cl, obj, msg);
			break;
		default:
			retval=DoSuperMethodA(cl, obj, msg);
			break;
	}

	return retval;
}

