
#define DEBUG 1
#include <aros/debug.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <libraries/mui.h>
#include <libraries/desktop.h>

#include "support.h"
#include "worker.h"
#include "desktop_intern.h"

#include <proto/desktop.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "presentation.h"
#include "observer.h"
#include "iconobserver.h"
#include "drawericonobserver.h"

#include "desktop_intern_protos.h"

#include <string.h>

IPTR drawerIconObserverNew(Class *cl, Object *obj, struct opSet *msg)
{
	IPTR retval=0;
	struct DrawerIconObserverClassData *data;
	struct TagItem *tag;

	retval=DoSuperMethodA(cl, obj, (Msg)msg);
	if(retval)
	{
		obj=(Object*)retval;
		data=INST_DATA(cl, obj);
	}

	return retval;
}

IPTR drawerIconObserverSet(Class *cl, Object *obj, struct opSet *msg)
{
	struct DrawerIconObserverClassData *data;
	IPTR retval=1;
	struct TagItem *tag, *tstate=msg->ops_AttrList;

	data=(struct DrawerIconObserverClassData*)INST_DATA(cl, obj);

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

IPTR drawerIconObserverGet(Class *cl, Object *obj, struct opGet *msg)
{
	IPTR retval=1;
	struct DrawerIconObserverClassData *data;

	data=(struct DrawerIconObserverClassData*)INST_DATA(cl, obj);

	switch(msg->opg_AttrID)
	{
		default:
			retval=DoSuperMethodA(cl, obj, (Msg)msg);
			break;
	}

	return retval;
}

IPTR drawerIconObserverDispose(Class *cl, Object *obj, Msg msg)
{
	IPTR retval;

	retval=DoSuperMethodA(cl, obj, msg);

	return retval;
}

IPTR drawerIconObserverExecute(Class *cl, Object *obj, Msg msg)
{
	IPTR retval;
	UBYTE *name, *directory;
	struct TagItem *tags;
	UBYTE *newDir;

	struct DrawerIconObserverClassData *data;

	data=(struct DrawerIconObserverClassData*)INST_DATA(cl, obj);
	retval=DoSuperMethodA(cl, obj, msg);

kprintf("do/e1\n");


	name=_name(obj);
	directory=_directory(obj);

kprintf("do/e2\n");

	newDir=AllocVec(strlen(name)+strlen(directory)+2, MEMF_ANY);
	strcpy(newDir, directory);
	strcat(newDir, "/");
	strcat(newDir, name);

kprintf("do/e3\n");

	tags=AllocVec(sizeof(struct TagItem)*2, MEMF_ANY);
	tags[0].ti_Tag=ICOA_Directory;
	tags[0].ti_Data=newDir;
	tags[1].ti_Tag=TAG_END;
	tags[1].ti_Data=0;

kprintf("do/e4\n");

	CreateDesktopObjectA(CDO_DirectoryWindow, tags);

kprintf("do/e5\n");

	retval=1;

	return retval;
}

AROS_UFH3(IPTR, drawerIconObserverDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
{
	ULONG retval=0;

	switch(msg->MethodID)
	{
		case OM_NEW:
			retval=drawerIconObserverNew(cl, obj, (struct opSet*)msg);
			break;
		case OM_SET:
			retval=drawerIconObserverSet(cl, obj, (struct opSet*)msg);
			break;
		case OM_GET:
			retval=drawerIconObserverGet(cl, obj, (struct opGet*)msg);
			break;
		case OM_DISPOSE:
			retval=drawerIconObserverDispose(cl, obj, msg);
			break;
		case IOM_Execute:
			retval=drawerIconObserverExecute(cl, obj, msg);
			break;
		default:
			retval=DoSuperMethodA(cl, obj, msg);
			break;
	}

	return retval;
}

