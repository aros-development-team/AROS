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

#include <string.h>

IPTR iconObserverNew(Class *cl, Object *obj, struct opSet *msg)
{
	IPTR retval=0;
	struct IconObserverClassData *data;
	struct TagItem *tag;
	UBYTE *name=NULL, *directory=NULL;
	BOOL selected=FALSE;
	UBYTE *comment;
	BOOL script, pure, archived, readable, writeable, executable, deleteable;

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

	tag=FindTagItem(IOA_Comment, msg->ops_AttrList);
	if(tag)
	{
		comment=tag->ti_Data;
		tag->ti_Tag=TAG_IGNORE;
	}

	tag=FindTagItem(IOA_Script, msg->ops_AttrList);
	if(tag)
	{
		script=tag->ti_Data;
		tag->ti_Tag=TAG_IGNORE;
	}

	tag=FindTagItem(IOA_Pure, msg->ops_AttrList);
	if(tag)
	{
		pure=tag->ti_Data;
		tag->ti_Tag=TAG_IGNORE;
	}

	tag=FindTagItem(IOA_Archived, msg->ops_AttrList);
	if(tag)
	{
		archived=tag->ti_Data;
		tag->ti_Tag=TAG_IGNORE;
	}

	tag=FindTagItem(IOA_Readable, msg->ops_AttrList);
	if(tag)
	{
		readable=tag->ti_Data;
		tag->ti_Tag=TAG_IGNORE;
	}

	tag=FindTagItem(IOA_Writeable, msg->ops_AttrList);
	if(tag)
	{
		writeable=tag->ti_Data;
		tag->ti_Tag=TAG_IGNORE;
	}

	tag=FindTagItem(IOA_Executable, msg->ops_AttrList);
	if(tag)
	{
		executable=tag->ti_Data;
		tag->ti_Tag=TAG_IGNORE;
	}

	tag=FindTagItem(IOA_Deleteable, msg->ops_AttrList);
	if(tag)
	{
		deleteable=tag->ti_Data;
		tag->ti_Tag=TAG_IGNORE;
	}

	retval=DoSuperMethodA(cl, obj, (Msg)msg);
	if(retval)
	{
		UBYTE *fullPath;

		obj=(Object*)retval;
		data=INST_DATA(cl, obj);
		data->selected=selected;
		data->name=name;
		data->directory=directory;
		data->comment=comment;
		data->script=script;
		data->pure=pure;
		data->archived=archived;
		data->readable=readable;
		data->writeable=writeable;
		data->executable=executable;
		data->deleteable=deleteable;

		DoMethod(_presentation(obj), MUIM_Notify, IA_Executed, TRUE, obj, 1, IOM_Execute);
		DoMethod(_presentation(obj), MUIM_Notify, IA_Selected, MUIV_EveryTime, obj, 3, MUIM_Set, IOA_Selected, MUIV_TriggerValue);
		DoMethod(_presentation(obj), MUIM_Notify, IA_Directory, MUIV_EveryTime, obj, 3, MUIM_Set, IOA_Directory, MUIV_TriggerValue);
		DoMethod(_presentation(obj), MUIM_Notify, IA_Comment, MUIV_EveryTime, obj, 3, MUIM_Set, IOA_Comment, MUIV_TriggerValue);

		DoMethod(_presentation(obj), MUIM_Notify, IA_Script, MUIV_EveryTime, obj, 3, MUIM_Set, IOA_Script, MUIV_TriggerValue);
		DoMethod(_presentation(obj), MUIM_Notify, IA_Pure, MUIV_EveryTime, obj, 3, MUIM_Set, IOA_Pure, MUIV_TriggerValue);
		DoMethod(_presentation(obj), MUIM_Notify, IA_Archived, MUIV_EveryTime, obj, 3, MUIM_Set, IOA_Archived, MUIV_TriggerValue);
		DoMethod(_presentation(obj), MUIM_Notify, IA_Readable, MUIV_EveryTime, obj, 3, MUIM_Set, IOA_Readable, MUIV_TriggerValue);
		DoMethod(_presentation(obj), MUIM_Notify, IA_Writeable, MUIV_EveryTime, obj, 3, MUIM_Set, IOA_Writeable, MUIV_TriggerValue);
		DoMethod(_presentation(obj), MUIM_Notify, IA_Executable, MUIV_EveryTime, obj, 3, MUIM_Set, IOA_Executable, MUIV_TriggerValue);
		DoMethod(_presentation(obj), MUIM_Notify, IA_Deleteable, MUIV_EveryTime, obj, 3, MUIM_Set, IOA_Deleteable, MUIV_TriggerValue);
	}

	return retval;
}

IPTR iconObserverSet(Class *cl, Object *obj, struct opSet *msg)
{
	struct IconObserverClassData *data;
	IPTR retval=1;
	struct TagItem *tag, *tstate=msg->ops_AttrList;

	data=(struct IconObserverClassData*)INST_DATA(cl, obj);

	while((tag=NextTagItem(&tstate)))
	{
		switch(tag->ti_Tag)
		{
			case IOA_Comment:
				data->comment=tag->ti_Data;
				/* was this OM_SET triggered by a notify? */
				if(strcmp(_comment(_presentation(obj)), data->comment))
					DoMethod(_presentation(obj), MUIM_NoNotifySet, IA_Comment, data->comment);
				break;
// TODO: When one of these bits is set, send a request to the
// handler to do the change
			case IOA_Script:
				data->script=tag->ti_Data;
				/* was this OM_SET triggered by a notify? */
				if(_script(_presentation(obj))!=data->script)
					DoMethod(_presentation(obj), MUIM_NoNotifySet, IA_Script, data->script);
				break;
			case IOA_Pure:
				data->pure=tag->ti_Data;
				/* was this OM_SET triggered by a notify? */
				if(_pure(_presentation(obj))!=data->pure)
					DoMethod(_presentation(obj), MUIM_NoNotifySet, IA_Pure, data->pure);
				break;
			case IOA_Archived:
				data->archived=tag->ti_Data;
				/* was this OM_SET triggered by a notify? */
				if(_archived(_presentation(obj))!=data->archived)
					DoMethod(_presentation(obj), MUIM_NoNotifySet, IA_Archived, data->archived);
				break;
			case IOA_Readable:
				data->readable=tag->ti_Data;
				/* was this OM_SET triggered by a notify? */
				if(_readable(_presentation(obj))!=data->readable)
					DoMethod(_presentation(obj), MUIM_NoNotifySet, IA_Readable, data->readable);
				break;
			case IOA_Writeable:
				data->writeable=tag->ti_Data;
				/* was this OM_SET triggered by a notify? */
				if(_writeable(_presentation(obj))!=data->writeable)
					DoMethod(_presentation(obj), MUIM_NoNotifySet, IA_Writeable, data->writeable);
				break;
			case IOA_Executable:
				data->comment=tag->ti_Data;
				/* was this OM_SET triggered by a notify? */
				if(_executable(_presentation(obj))!=data->executable)
					DoMethod(_presentation(obj), MUIM_NoNotifySet, IA_Executable, data->executable);
				break;
			case IOA_Deleteable:
				data->comment=tag->ti_Data;
				/* was this OM_SET triggered by a notify? */
				if(_deleteable(_presentation(obj))!=data->deleteable)
					DoMethod(_presentation(obj), MUIM_NoNotifySet, IA_Deleteable, data->deleteable);
				break;
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
		case IOA_Script:
			*msg->opg_Storage=data->script;
			break;
		case IOA_Pure:
			*msg->opg_Storage=data->pure;
			break;
		case IOA_Archived:
			*msg->opg_Storage=data->archived;
			break;
		case IOA_Readable:
			*msg->opg_Storage=data->readable;
			break;
		case IOA_Writeable:
			*msg->opg_Storage=data->writeable;
			break;
		case IOA_Executable:
			*msg->opg_Storage=data->executable;
			break;
		case IOA_Deleteable:
			*msg->opg_Storage=data->deleteable;
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

