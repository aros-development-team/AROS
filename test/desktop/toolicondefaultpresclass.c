#include <aros/asmcall.h>
#include <exec/types.h>

#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include "wbwindowclass.h"
#include "toolicondefaultpresclass.h"
#include "wbtooliconclass.h"
#include "wbiconclass.h"

/*
ULONG tidpNew(Class *cl, Object *obj, opSet *ops)
{
	ULONG retval=0;
	TagItem *tag;
	Object *target;
	WBToolIconDefPresClassData *data;

	tag=FindTagItem(WBA_IconDefaultPres_Target, ops->ops_AttrList);
	if(tag)
	{
		tag->ti_Tag=TAG_IGNORE;
		target=(Object*)tag->ti_Data;
	}
	else
		return 0;

	retval=DoSuperMethodA(cl, obj, (Msg)ops);
	if(retval)
	{
		obj=(Object*)retval;

		data=(WBToolIconDefPresClassData*)INST_DATA(cl, obj);
		data->target=target;
	}

	return retval;
}
*/
/*
ULONG tidpGet(Class *cl, Object *obj, opGet *opg)
{
	ULONG retval=0;
	WBToolIconDefPresClassData *data;

	data=(WBToolIconDefPresClassData*)INST_DATA(cl, obj);

	switch(opg->opg_AttrID)
	{
		default:
			retval=DoSuperMethodA(cl, obj,(Msg)opg);
			break;
	}

	return retval;
}
*/
/*
ULONG tidpSet(Class *cl, Object *obj, opSet *ops)
{
	ULONG retval=0;
	TagItem *tstate, *tag;
	WBToolIconDefPresClassData *data;

	data=(WBToolIconDefPresClassData*)INST_DATA(cl, obj);

	tstate=ops->ops_AttrList;
	while(tag=(NextTagItem(&tstate)))
	{
		switch(tag->ti_Tag)
		{
			case WBA_ToolIconDefaultPres_Target:
				tag->ti_Tag=TAG_IGNORE;
				data->target=(Object*)tag->ti_Data;
				break;
		}
	}

	retval=DoSuperMethodA(cl, obj, (Msg)ops);

	return retval;
}
*/
/*
ULONG tidpDraw(Class *cl, Object *obj, MUIP_Draw *msg)
{
	ULONG retval=0;
	WBToolIconDefPresClassData *data;
	char *labelText;

	retval=DoSuperMethodA(cl, obj, (Msg)msg);

	data=(WBToolIconDefPresClassData*)INST_DATA(cl, obj);

	GetAttr(WBA_Icon_Name, data->target, (ULONG*)&labelText);

	SetAPen(_rp(obj), 1);
	RectFill(_rp(obj), _mleft(obj)+data->iconBounds.Left, _mtop(obj)+data->iconBounds.Top, _mleft(obj)+data->iconBounds.Left+data->iconBounds.Width, _mtop(obj)+data->iconBounds.Top+data->iconBounds.Height);
	Move(_rp(obj), _mleft(obj), _mtop(obj)+data->labelBounds.Height+data->iconBounds.Height+1);
	Text(_rp(obj), labelText, strlen(labelText));

	return retval;
}
*/
/*
ULONG tidpSetup(Class *cl, Object *obj, Msg msg)
{
	ULONG retval=0;
	TextFont *tf;
	RastPort rp;
	WBToolIconDefPresClassData *data;
	struct TextExtent te;
	char *labelText;

	data=(WBToolIconDefPresClassData*)INST_DATA(cl, obj);

	GetAttr(WBA_Icon_Name, data->target, (ULONG*)&labelText);

	retval=DoSuperMethodA(cl, obj, msg);

	InitRastPort(&rp);
	tf=_font(obj);
	SetFont(&rp, tf);
	TextExtent(&rp, labelText, strlen(labelText), &te);

	data->labelBounds.Left=0;
	data->labelBounds.Top=41;
	data->labelBounds.Width=te.te_Width;
	data->labelBounds.Height=te.te_Height;

	data->iconBounds.Left=0;
	data->iconBounds.Top=0;
	data->iconBounds.Width=40;
	data->iconBounds.Height=40;

	return retval;
}

ULONG tidpAskMinMax(Class *cl, Object *obj, MUIP_AskMinMax *msg)
{
	ULONG retval=0;
	WBToolIconDefPresClassData *data;

	retval=DoSuperMethodA(cl, obj, (Msg)msg);

	data=(WBToolIconDefPresClassData*)INST_DATA(cl, obj);

	msg->MinMaxInfo->MinWidth+=(data->labelBounds.Width>data->iconBounds.Width ? data->labelBounds.Width : data->iconBounds.Width);
	msg->MinMaxInfo->DefWidth+=(data->labelBounds.Width>data->iconBounds.Width ? data->labelBounds.Width : data->iconBounds.Width);
	msg->MinMaxInfo->MaxWidth+=(data->labelBounds.Width>data->iconBounds.Width ? data->labelBounds.Width : data->iconBounds.Width);
	msg->MinMaxInfo->MinHeight+=data->labelBounds.Height+data->iconBounds.Height;
	msg->MinMaxInfo->DefHeight+=data->labelBounds.Height+data->iconBounds.Height;
	msg->MinMaxInfo->MaxHeight+=data->labelBounds.Height+data->iconBounds.Height;

	return retval;
}
*/

AROS_UFH3(IPTR,wbToolIconClassDefPresDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
{
	ULONG retval=0;

	switch(msg->MethodID)
	{
//		case OM_NEW:
//			retval=tidpNew(cl, obj, (opSet*)msg);
//			break;
//		case OM_GET:
//			retval=tidpGet(cl, obj, (opGet*)msg);
//			break;
//		case OM_SET:
//			retval=tidpSet(cl, obj, (opSet*)msg);
//			break;
//		case MUIM_Setup:
//			retval=tidpSetup(cl, obj, msg);
//			break;
//		case MUIM_AskMinMax:
//			retval=tidpAskMinMax(cl, obj, (MUIP_AskMinMax*)msg);
//			break;
//		case MUIM_Draw:
//			retval=tidpDraw(cl, obj, (MUIP_Draw*)msg);
//			break;
		default:
			retval=DoSuperMethodA(cl, obj, msg);
			break;
	}

	return retval;
}

