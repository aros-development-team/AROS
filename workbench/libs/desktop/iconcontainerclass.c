
#define DEBUG 1
#include <aros/debug.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <libraries/mui.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/layers.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include "desktop_intern.h"
#include "presentation.h"
#include "iconcontainerclass.h"
#include "iconclass.h"

#include "desktop_intern_protos.h"

#include <stdlib.h>

ULONG DoSetupMethod(Object *obj, struct MUI_RenderInfo *info)
{
	/* MUI set the correct render info *before* it calls MUIM_Setup so please only use this function instead of DoMethodA() */
    muiRenderInfo(obj)=info;

	return DoMethod(obj, MUIM_Setup, info);
}

void broadcastMessage(Class *cl, Object *obj, Msg msg)
{
	struct MemberNode *mn;
	struct IconContainerClassData *data;

	data=INST_DATA(cl, obj);

	mn=(struct MemberNode*)data->memberList.mlh_Head;
	while(mn->m_Node.mln_Succ)
	{
		DoMethodA(mn->m_Object, msg);
		mn=(struct MemberNode*)mn->m_Node.mln_Succ;
	}

	if(data->vertProp)
		DoMethodA(data->vertProp, msg);
	if(data->horizProp)
		DoMethodA(data->horizProp, msg);
}

IPTR iconConNew(Class *cl, Object *obj, struct opSet *ops)
{
	IPTR retval=0;
	struct IconContainerClassData *data;
	struct TagItem *tag;
	Object *vert=NULL, *horiz=NULL;

	tag=FindTagItem(ICA_VertScroller, ops->ops_AttrList);
	if(tag)
	{
		tag->ti_Tag=TAG_IGNORE;
		vert=(Object*)tag->ti_Data;
	}
	tag=FindTagItem(ICA_HorizScroller, ops->ops_AttrList);
	if(tag)
	{
		tag->ti_Tag=TAG_IGNORE;
		horiz=(Object*)tag->ti_Data;
	}

	retval=DoSuperMethodA(cl, obj, (Msg)ops);

	if(retval)
	{
		obj=(Object*)retval;
		data=INST_DATA(cl, obj);
		NewList((struct List*)&data->memberList);
		data->searchForGaps=FALSE;
		data->thisColumnWidth=0;
		data->thisColumnHeight=0;
		data->virtualWidth=0;
		data->virtualHeight=0;
		data->xView=0;
		data->yView=0;
		data->visibleWidth=0;
		data->visibleHeight=0;
		data->heightAdjusted=0;
		data->widthAdjusted=0;
		data->horizScroll=FALSE;
		data->vertScroll=FALSE;
		data->horizProp=horiz;
		data->vertProp=vert;
		data->iconSelected=FALSE;
		data->justSelected=FALSE;
	}

	return retval;
}

IPTR iconConSetup(Class *cl, Object *obj, struct MUIP_Setup *msg)
{
	IPTR retval;
	struct MemberNode *mn;
	struct IconContainerClassData *data;

	data=(struct IconContainerClassData*)INST_DATA(cl, obj);

	retval=DoSuperMethodA(cl, obj, (Msg)msg);

	mn=(struct MemberNode*)data->memberList.mlh_Head;
	while(mn->m_Node.mln_Succ)
	{
		DoSetupMethod(mn->m_Object, msg->RenderInfo);
		mn=(struct MemberNode*)mn->m_Node.mln_Succ;
	}

	if(data->horizProp)
		DoSetupMethod(data->horizProp, msg->RenderInfo);

	if(data->vertProp)
		DoSetupMethod(data->vertProp, msg->RenderInfo);

	MUI_RequestIDCMP(obj, IDCMP_MOUSEBUTTONS);


	return retval;
}

IPTR iconConShow(Class *cl, Object *obj, Msg msg)
{
	IPTR retval;
	struct MemberNode *mn;
	struct IconContainerClassData *data;

	data=(struct IconContainerClassData*)INST_DATA(cl, obj);

	retval=DoSuperMethodA(cl, obj, (Msg)msg);

	mn=(struct MemberNode*)data->memberList.mlh_Head;
	while(mn->m_Node.mln_Succ)
	{
		DoMethodA(mn->m_Object, msg);
		mn=(struct MemberNode*)mn->m_Node.mln_Succ;
	}

	if(data->horizProp)
		DoMethod(data->horizProp, MUIM_Show);

	if(data->vertProp)
		DoMethod(data->vertProp, MUIM_Show);

	MUI_RequestIDCMP(obj, IDCMP_MOUSEBUTTONS);

	return retval;
}


IPTR iconConAskMinMax(Class *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
	IPTR retval;
	struct MUI_MinMax minMax;
	struct IconContainerClassData *data;

	data=(struct IconContainerClassData*)INST_DATA(cl, obj);

	retval=DoSuperMethodA(cl, obj, (Msg)msg);

	broadcastMessage(cl, obj, (Msg)msg);

	msg->MinMaxInfo->MinWidth+=20;
	msg->MinMaxInfo->DefWidth+=300;
	msg->MinMaxInfo->MaxWidth=MUI_MAXMAX;
	msg->MinMaxInfo->MinHeight+=20;
	msg->MinMaxInfo->DefHeight+=300;
	msg->MinMaxInfo->MaxHeight=MUI_MAXMAX;

	minMax.MinWidth=0;
	minMax.DefWidth=0;
	minMax.MaxWidth=0;
	minMax.MinHeight=0;
	minMax.DefHeight=0;
	minMax.MaxHeight=0;
	DoMethod(data->vertProp, MUIM_AskMinMax, &minMax);

	minMax.MinWidth=0;
	minMax.DefWidth=0;
	minMax.MaxWidth=0;
	minMax.MinHeight=0;
	minMax.DefHeight=0;
	minMax.MaxHeight=0;
	DoMethod(data->horizProp, MUIM_AskMinMax, &minMax);

	return retval;
}

ULONG layoutObject(struct IconContainerClassData *data, Object *obj, Object *newObject)
{
	ULONG retval=0;
	struct MemberNode *last;
	ULONG newX, newY;

//	if(!searchForGaps)
//	{
		if(IsListEmpty(&data->memberList))
		{
			newX=ICONSPACINGX;
			newY=ICONSPACINGY;

			data->virtualWidth=_defwidth(newObject)+ICONSPACINGX;
			data->thisColumnWidth=_defwidth(newObject);
		}
		else
		{
			last=data->memberList.mlh_TailPred;
			newX=_left(last->m_Object)-_mleft(obj);
			newY=_bottom(last->m_Object)+ICONSPACINGY-_mtop(obj);
			if(newY+_defheight(newObject) > _mheight(obj))
			{
				// new column
				newX+=data->thisColumnWidth+ICONSPACINGX;
				newY=ICONSPACINGY;
				data->virtualWidth+=_defwidth(newObject)+ICONSPACINGX;
				data->thisColumnHeight=0;
				data->thisColumnWidth=0;
			}

			if(_defwidth(newObject) > data->thisColumnWidth)
			{
				data->virtualWidth+=(_defwidth(newObject)-data->thisColumnWidth);
				data->thisColumnWidth=_defwidth(newObject);
			}
		}
//	}

	data->thisColumnHeight+=(ICONSPACINGY+_defheight(newObject));
	if(data->thisColumnHeight+_defheight(newObject)+ICONSPACINGY > data->virtualHeight)
		data->virtualHeight+=data->thisColumnHeight;

	MUI_Layout(newObject, newX, newY, _defwidth(newObject), _defheight(newObject), 0);

	return retval;
}

ULONG iconConLayout(Class *cl, Object *obj, Msg msg)
{
	ULONG retval;
	struct IconContainerClassData *data;

	data=INST_DATA(cl, obj);
	retval=DoSuperMethodA(cl, obj, msg);

	if(data->visibleHeight==0)
	{
		// first layout
	}
	else
	{
		if(data->visibleHeight!=_mheight(obj))
		{
			// height adjusted
			data->heightAdjusted=_mheight(obj)-data->visibleHeight;
		}

		if(data->visibleWidth!=_mwidth(obj))
		{
			// width adjusted
			data->widthAdjusted=_mwidth(obj)-data->visibleWidth;
		}
	}

//	layoutGroup(data);

	return retval;
}

IPTR iconConAdd(Class *cl, Object *obj, struct opMember *msg)
{
	struct IconContainerClassData *data;
	struct MemberNode *mn;
	ULONG retval=1;
	struct MUI_MinMax minMax;
	Object *container, *win, *rect, *par;
	struct Region *clippingRegion;
	struct ConnectParentData *connectParent;

	minMax.MinWidth=0;
	minMax.DefWidth=0;
	minMax.MaxWidth=0;
	minMax.MinHeight=0;
	minMax.DefHeight=0;
	minMax.MaxHeight=0;

	data=(struct IconContainerClassData*)INST_DATA(cl, obj);

	mn=AllocVec(sizeof(struct MemberNode), MEMF_ANY);
	mn->m_Object=msg->opam_Object;

	muiNotifyData(msg->opam_Object)->mnd_ParentObject = obj;
	DoMethod(msg->opam_Object, MUIM_ConnectParent, obj);

	DoMethod(msg->opam_Object, MUIM_Notify, IA_Selected, TRUE, obj, 3, MUIM_Set, ICA_JustSelected, TRUE);
	DoMethod(msg->opam_Object, MUIM_Notify, IA_Executed, TRUE, obj, 3, MUIM_Set, ICA_JustSelected, TRUE);

	DoSetupMethod(msg->opam_Object, muiRenderInfo(obj));

	DoMethod(msg->opam_Object, MUIM_AskMinMax, &minMax);

	_minwidth(msg->opam_Object)=minMax.MinWidth;
    _minheight(msg->opam_Object)=minMax.MinHeight;
    _maxwidth(msg->opam_Object)=minMax.MaxWidth;
    _maxheight(msg->opam_Object)=minMax.MaxHeight;
    _defwidth(msg->opam_Object)=minMax.DefWidth;
    _defheight(msg->opam_Object)=minMax.DefHeight;

	layoutObject(data, obj, msg->opam_Object);

	DoMethod(msg->opam_Object, MUIM_Show);

//	data->clippingRectangle.MinX=_mleft(obj);
//	data->clippingRectangle.MinY=_mtop(obj);
//	data->clippingRectangle.MaxX=_mleft(obj)+_mwidth(obj);
//	data->clippingRectangle.MaxY=_mtop(obj)+_mheight(obj);

//	clippingRegion=NewRegion();
//	OrRectRegion(clippingRegion, &data->clippingRectangle);
//	InstallClipRegion(_window(obj)->WLayer, clippingRegion);

	MUI_Redraw(msg->opam_Object, MADF_DRAWOBJECT);

	AddTail((struct List*)&data->memberList, (struct Node*)mn);

	SetAttrs(data->horizProp, MUIA_Prop_Entries, data->virtualWidth, TAG_END);
	SetAttrs(data->vertProp, MUIA_Prop_Entries, data->virtualHeight, TAG_END);

//	InstallClipRegion(_window(obj)->WLayer, NULL);

	return retval;
}

void redrawRectangle(ULONG x1, ULONG y1, ULONG x2, ULONG y2, Object *obj, struct IconContainerClassData *data)
{
	struct MemberNode *mn;

//	printf("redrawRectangle: %d, %d, %d, %d\n", x1,x2,y1,y2);

//	BeginRefresh(_window(obj));

	mn=(struct MemberNode*)data->memberList.mlh_Head;
	while(mn->m_Node.mln_Succ)
	{
		// check to see if the left or right edge is in the damaged
		// area - also, check to see whether an object is in the area where
		// both edges are to the left and right of the damaged area
		if((_mleft(mn->m_Object) >= x1 && _mleft(mn->m_Object) <= x2) ||
			(_mright(mn->m_Object) >= x1 && _mright(mn->m_Object) <= x2) ||
			(_mleft(mn->m_Object) <= x1 && _mright(mn->m_Object) >= x2))
		{
			// as above, except with the top and bottom edges of the member
			// objects
			if((_mtop(mn->m_Object) >= y1 && _mtop(mn->m_Object) <= y2) ||
				(_mbottom(mn->m_Object) >= y1 && _mbottom(mn->m_Object) <= y2) ||
				(_mtop(mn->m_Object) <= y1 && _mbottom(mn->m_Object) >= y2))
			{
				MUI_Redraw(mn->m_Object, MADF_DRAWOBJECT);
			}
		}

		mn=(struct MemberNode*)mn->m_Node.mln_Succ;
	}

//	EndRefresh(_window(obj), TRUE);
}

IPTR iconConDraw(Class *cl, Object *obj, struct MUIP_Draw *msg)
{
	struct IconContainerClassData *data;
	struct MemberNode *mn;
	IPTR retval=1;

	data=(struct IconContainerClassData*)INST_DATA(cl, obj);

	retval=DoSuperMethodA(cl, obj, (Msg)msg);

	if(msg->flags & MADF_DRAWOBJECT)
	{
		if(data->widthAdjusted!=0)
		{
			SetAttrs(data->horizProp, MUIA_Prop_Visible, _mwidth(obj), TAG_END);
			if(data->widthAdjusted>0)
				redrawRectangle(_mright(obj)-data->widthAdjusted, _mtop(obj), _mright(obj), _mbottom(obj), obj, data);
		}

		if(data->heightAdjusted!=0)
		{
			if(data->heightAdjusted>0)
			{
				SetAttrs(data->vertProp, MUIA_Prop_Visible, _mheight(obj), TAG_END);
				if(data->heightAdjusted>0)
					redrawRectangle(_mleft(obj), _mbottom(obj)-data->heightAdjusted, _mright(obj), _mbottom(obj), obj, data);
			}
		}

		if(data->heightAdjusted==0 && data->widthAdjusted==0)
		{
			SetAttrs(data->horizProp, MUIA_Prop_Visible, _mwidth(obj), TAG_END);
			SetAttrs(data->vertProp, MUIA_Prop_Visible, _mheight(obj), TAG_END);

			mn=(struct MemberNode*)data->memberList.mlh_Head;
			while(mn->m_Node.mln_Succ)
			{
				MUI_Redraw(mn->m_Object, MADF_DRAWOBJECT);
				mn=(struct MemberNode*)mn->m_Node.mln_Succ;
			}
		}

		data->visibleWidth=_mwidth(obj);
		data->visibleHeight=_mheight(obj);
		data->widthAdjusted=0;
		data->heightAdjusted=0;
	}
	else if(msg->flags & MADF_DRAWUPDATE)
	{
		LONG scrollAmountX=0, scrollAmountY=0;
		LONG redrawX1, redrawY1, redrawX2, redrawY2;

		if(data->vertScroll)
		{
			// vertical scroll
			scrollAmountY=data->yView-data->lastYView;

			if(scrollAmountY>0)
			{
				// scroll bottom, displays shifts up, redraw gap at bottom
				redrawY1=_mbottom(obj)-abs(scrollAmountY);
				redrawY2=_mbottom(obj);
			}
			else if(scrollAmountY<0)
			{
				// scroll top, display shifts bottom, redraw gap at top
				redrawY1=_mtop(obj);
				redrawY2=_mtop(obj)+abs(scrollAmountY);
			}

			redrawX1=_mleft(obj);
			redrawX2=_mright(obj);

			// shift the positions of the member objects
			mn=(struct MemberNode*)data->memberList.mlh_Head;
			while(mn->m_Node.mln_Succ)
			{
				//_left(mn->m_Object)=_left(mn->m_Object)-scrollAmountX;
				//_top(mn->m_Object)=_top(mn->m_Object)-scrollAmountY;

				// this is slow... but necessary. If members are groups, then
				// we can't resort to the above
				// todo: ONLY layout those objects that are, or will be, on-scren
				MUI_Layout(mn->m_Object, _left(mn->m_Object)-scrollAmountX-_mleft(obj), _top(mn->m_Object)-scrollAmountY-_mtop(obj), _width(mn->m_Object), _height(mn->m_Object), 0);

				mn=(struct MemberNode*)mn->m_Node.mln_Succ;
			}

			ScrollRaster(_rp(obj), 0, scrollAmountY, _mleft(obj), _mtop(obj), _mright(obj), _mbottom(obj));

			// redraw in gap
			redrawRectangle(redrawX1, redrawY1, redrawX2, redrawY2, obj, data);

			data->vertScroll=FALSE;
		}
		else if(data->horizScroll)
		{
			// horizontal scroll
			scrollAmountX=data->xView-data->lastXView;

			// redraw gap area
			if(scrollAmountX>0)
			{
				// scroll right, displays shifts left, redraw gap at right
				redrawX1=_mright(obj)-abs(scrollAmountX);
				redrawX2=_mright(obj);

			}
			else if(scrollAmountX<0)
			{
				// scroll left, display shifts right, redraw gap at left
				redrawX1=_mleft(obj);
				redrawX2=_mleft(obj)+abs(scrollAmountX);
			}

			redrawY1=_mtop(obj);
			redrawY2=_mbottom(obj);

			// shift the positions of the member objects
			mn=(struct MemberNode*)data->memberList.mlh_Head;
			while(mn->m_Node.mln_Succ)
			{
//				_left(mn->m_Object)=_left(mn->m_Object)-scrollAmountX;
//				_top(mn->m_Object)=_top(mn->m_Object)-scrollAmountY;

				MUI_Layout(mn->m_Object, _left(mn->m_Object)-scrollAmountX-_mleft(obj), _top(mn->m_Object)-scrollAmountY-_mtop(obj), _width(mn->m_Object), _height(mn->m_Object), 0);


				mn=(struct MemberNode*)mn->m_Node.mln_Succ;
			}

			ScrollRaster(_rp(obj), scrollAmountX, 0, _mleft(obj), _mtop(obj), _mwidth(obj)+_mleft(obj), _mheight(obj)+_mtop(obj));

			// redraw in gap
			redrawRectangle(redrawX1, redrawY1, redrawX2, redrawY2, obj, data);

			data->horizScroll=FALSE;

		}
	}

	return retval;
}

IPTR iconConSet(Class *cl, Object *obj, struct opSet *msg)
{
	struct IconContainerClassData *data;
	IPTR retval;
	struct TagItem *tag, *tstate=msg->ops_AttrList;

	data=(struct IconContainerClassData*)INST_DATA(cl, obj);

	while(tag=NextTagItem(&tstate))
    {
		switch(tag->ti_Tag)
		{
			case ICA_ScrollToHoriz:
			{
//				printf("ScrollX: %d\n", tag->ti_Data);
				data->lastXView=data->xView;
				data->xView=tag->ti_Data;
				data->horizScroll=TRUE;
				MUI_Redraw(obj, MADF_DRAWUPDATE);
				break;
			}
			case ICA_ScrollToVert:
//				printf("ScrollY: %d\n", tag->ti_Data);
				data->lastYView=data->yView;
				data->yView=tag->ti_Data;
				data->vertScroll=TRUE;
				MUI_Redraw(obj, MADF_DRAWUPDATE);
				break;
			case ICA_JustSelected:
				data->justSelected=tag->ti_Data;
				retval=DoSuperMethodA(cl, obj, (Msg)msg);
				break;
			default:
				retval=DoSuperMethodA(cl, obj, (Msg)msg);
				break;
		}
	}

	return 0;
}

IPTR iconConConnectParent(Class *cl, Object *obj, struct MUIP_ConnectParent *msg)
{
	struct IconContainerClassData *data;
	IPTR retval;

	retval=DoSuperMethodA(cl, obj, msg);

	data=(struct IconContainerClassData*)INST_DATA(cl, obj);

	if(data->horizProp)
	{
		muiNotifyData(data->horizProp)->mnd_ParentObject=obj;
		DoMethod(data->horizProp, MUIM_ConnectParent, obj);
	}
	if(data->vertProp)
	{
		muiNotifyData(data->vertProp)->mnd_ParentObject=obj;
		DoMethod(data->vertProp, MUIM_ConnectParent, obj);
	}

	SetAttrs(obj, PA_InTree, TRUE, TAG_END);

	return retval;
}

IPTR iconConHandleInput(Class *cl, Object *obj, struct MUIP_HandleInput *msg)
{
	IPTR retval=0;

	struct IconContainerClassData *data;

	data=(struct IconContainerClassData*)INST_DATA(cl, obj);

	if(msg->imsg)
	{
		switch(msg->imsg->Class)
		{
			case IDCMP_MOUSEBUTTONS:
			{
				if(msg->imsg->Code==SELECTDOWN)
				{
					if(msg->imsg->MouseX >= _mleft(obj) && msg->imsg->MouseX <= _mright(obj) &&
						msg->imsg->MouseY >= _mtop(obj) && msg->imsg->MouseY <= _mbottom(obj))
					{
						if(!data->justSelected)
							DoMethod(obj, ICM_UnselectAll);
						else
							data->justSelected=FALSE;
					}
				}
				break;
			}
		}
	}

	return retval;
}

IPTR iconConUnselectAll(Class *cl, Object *obj, Msg msg)
{
	IPTR retval=0;
	struct MemberNode *mn;
	struct IconContainerClassData *data;

	data=(struct IconContainerClassData*)INST_DATA(cl, obj);

	mn=(struct MemberNode*)data->memberList.mlh_Head;
	while(mn->m_Node.mln_Succ)
	{
		if(_selected(mn->m_Object))
			SetAttrs(mn->m_Object, IA_Selected, FALSE, TAG_END);
		mn=(struct MemberNode*)mn->m_Node.mln_Succ;
	}

	return retval;
}

AROS_UFH3(IPTR, iconContainerDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
{
	ULONG retval=0;

	switch(msg->MethodID)
	{
		case OM_NEW:
			retval=iconConNew(cl, obj, (struct opSet*)msg);
			break;
		case MUIM_Setup:
			retval=iconConSetup(cl, obj, (struct MUIP_Setup*)msg);
			break;
		case MUIM_Show:
			retval=iconConShow(cl, obj, (struct MUIP_Show*)msg);
			break;
		case MUIM_Draw:
			retval=iconConDraw(cl, obj, (struct MUIP_Draw*)msg);
			break;
		case MUIM_AskMinMax:
			retval=iconConAskMinMax(cl, obj, (struct MUIP_AskMinMax*)msg);
			break;
		case MUIM_Layout:
			iconConLayout(cl, obj, msg);
			break;
		case MUIM_DrawBackground:
			break;
		case OM_ADDMEMBER:
			retval=iconConAdd(cl, obj, (struct opMember*)msg);
			break;
		case OM_SET:
			retval=iconConSet(cl, obj, (struct opSet*)msg);
			break;
		case OM_GET:
			retval=DoSuperMethodA(cl, obj, msg);
			break;
		case MUIM_ConnectParent:
			retval=iconConConnectParent(cl, obj, (struct MUIP_ConnectParent*)msg);
			break;
		case MUIM_HandleInput:
			retval=iconConHandleInput(cl, obj, (struct MUIP_HandleInput*)msg);
			break;
		case ICM_UnselectAll:
			retval=iconConUnselectAll(cl, obj, msg);
			break;
		default:
			retval=DoSuperMethodA(cl, obj, msg);
			broadcastMessage(cl, obj, msg);
			break;
	}

	return retval;
}


