
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
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "presentation.h"
#include "iconclass.h"

#include "desktop_intern_protos.h"

IPTR iconNew(Class *cl, Object *obj, struct opSet *msg)
{
	IPTR retval=0;
	struct IconClassData *data;
	struct TagItem *tag;
	struct DiskObject *diskobject=NULL;
	UBYTE *label=NULL;

	tag=FindTagItem(IA_DiskObject, msg->ops_AttrList);
	if(tag)
	{
		diskobject=tag->ti_Data;
		tag->ti_Tag=TAG_IGNORE;
	}

	tag=FindTagItem(IA_Label, msg->ops_AttrList);
	if(tag)
	{
		label=tag->ti_Data;
		tag->ti_Tag=TAG_IGNORE;
	}

	retval=DoSuperMethodA(cl, obj, msg);
	if(retval)
	{
		obj=(Object*)retval;
		data=INST_DATA(cl, obj);
		data->diskObject=diskobject;
		data->label=label;

		data->imagePart=RectangleObject,
			ButtonFrame,
			MUIA_FixHeight, 30,
			MUIA_FixWidth, 30,
			End;

		data->labelPart=TextObject,
			MUIA_Text_Contents, label,
			End;

		DoMethod(obj, OM_ADDMEMBER, data->imagePart);
		DoMethod(obj, OM_ADDMEMBER, data->labelPart);
	}

	return retval;
}

IPTR iconSet(Class *cl, Object *obj, struct opSet *msg)
{
	struct IconClassData *data;
	IPTR retval=1;
	struct TagItem *tag, *tstate=msg->ops_AttrList;

	data=(struct IconClassData*)INST_DATA(cl, obj);

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

IPTR iconGet(Class *cl, Object *obj, struct opGet *msg)
{
	IPTR retval=1;
	struct IconClassData *data;

	data=(struct IconClassData*)INST_DATA(cl, obj);

	switch(msg->opg_AttrID)
	{
		default:
			retval=DoSuperMethodA(cl, obj, (Msg)msg);
			break;
	}

	return retval;
}

IPTR iconDispose(Class *cl, Object *obj, Msg msg)
{
	IPTR retval;
	struct IconClassData *data;

	data=(struct IconClassData*)INST_DATA(cl, obj);

	retval=DoSuperMethodA(cl, obj, msg);

	return retval;
}

#define MAX(x, y) (x>y?x:y)

IPTR iconAskMinMax(Class *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
	IPTR retval;
	struct MUI_MinMax minMax;
	struct IconClassData *data;

	data=(struct IconClassData*)INST_DATA(cl, obj);

	retval=DoSuperMethodA(cl, obj, (Msg)msg);

	msg->MinMaxInfo->MinWidth+=(MAX(_minwidth(data->imagePart),_minwidth(data->labelPart)));
	msg->MinMaxInfo->DefWidth+=(MAX(_minwidth(data->imagePart),_minwidth(data->labelPart)));
	msg->MinMaxInfo->MaxWidth+=(MAX(_minwidth(data->imagePart),_minwidth(data->labelPart)));

	msg->MinMaxInfo->MinHeight+=(_minheight(data->imagePart)+_minheight(data->labelPart));
	msg->MinMaxInfo->DefHeight+=(_minheight(data->imagePart)+_minheight(data->labelPart));
	msg->MinMaxInfo->MaxHeight+=(_minheight(data->imagePart)+_minheight(data->labelPart));

kprintf("_defheight: %d, _defwidth: %d\n", msg->MinMaxInfo->DefHeight, msg->MinMaxInfo->DefWidth);

	return retval;
}

AROS_UFH3(IPTR, iconDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
{
	ULONG retval=0;

	switch(msg->MethodID)
	{
		case OM_NEW:
			retval=iconNew(cl, obj, (struct opSet*)msg);
			break;
		case OM_SET:
			retval=iconSet(cl, obj, (struct opSet*)msg);
			break;
		case OM_GET:
			retval=iconGet(cl, obj, (struct opGet*)msg);
			break;
		case OM_DISPOSE:
			retval=iconDispose(cl, obj, msg);
			break;
//		case MUIM_AskMinMax:
//			retval=iconAskMinMax(cl, obj, (struct MUIP_AskMinMax*)msg);
//			break;
		default:
			retval=DoSuperMethodA(cl, obj, msg);
			break;
	}

	return retval;
}

