#include <aros/asmcall.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <libraries/mui.h>

#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/layers.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#define DEBUG 1
#define SDEBUG 1

#include <aros/debug.h>

#include "wbwindowdefaultpresclass.h"
#include "wbwindowclass.h"
#include "icondefpres.h"

#include <stdio.h>
#include <string.h>

ULONG DoSetupMethod(Object *obj, struct MUI_RenderInfo *info)
{
    /* MUI set the correct render info *before* it calls MUIM_Setup so please only use this function instead of DoMethodA() */
    muiRenderInfo(obj)=info;
    return DoMethod(obj, MUIM_Setup, info);
}

void broadcastMessage(Class *cl, Object *obj, Msg msg)
{
	struct MemberNode *mn;
	struct WBWinDefPresClassData *data;

	data=INST_DATA(cl, obj);

	mn=(struct MemberNode*)data->memberList.mlh_Head;
	while(mn->m_Node.mln_Succ)
	{
		DoMethodA(mn->m_member, msg);
		mn=(struct MemberNode*)mn->m_Node.mln_Succ;
	}
}

IPTR winDefPresNew(Class *cl, Object *obj, struct opSet *ops)
{
	IPTR retval=0;
	struct WBWinDefPresClassData *data;
	struct TagItem *tag;
	Object *vertProp=NULL, *horizProp=NULL;
	Object *semanticObject=NULL;

	tag=FindTagItem(WBA_WinPresentation_VertSize, ops->ops_AttrList);
	if(tag)
	{
		tag->ti_Tag=TAG_IGNORE;
		vertProp=(Object*)tag->ti_Data;
	}

	tag=FindTagItem(WBA_WinPresentation_HorizSize, ops->ops_AttrList);
	if(tag)
	{
		tag->ti_Tag=TAG_IGNORE;
		horizProp=(Object*)tag->ti_Data;
	}
	tag=FindTagItem(WBA_WinPresentation_SemanticObject, ops->ops_AttrList);
	if(tag)
	{
		tag->ti_Tag=TAG_IGNORE;
		semanticObject=(Object*)tag->ti_Data;
	}

	retval=DoSuperMethodA(cl, obj, (Msg)ops);

	if(retval)
	{
		obj=(Object*)retval;
		data=INST_DATA(cl, obj);

		NewList((struct List*)&data->memberList);
		data->justRemoved=NULL;
		data->lastAddedX=0;
		data->lastAddedY=0;
		data->lastAddedW=0;
		data->lastAddedH=0;
		data->biggestWidthInLastCol=0;
		data->lastColH=0;
		data->viewPointX=0;
		data->viewPointY=0;
		data->virtualHeight=0;
		data->virtualWidth=0;
		data->vertProp=vertProp;
		data->horizProp=horizProp;
		data->semanticObject=semanticObject;
		data->clicked=FALSE;
	}

	return retval;

}

ULONG layoutObject(struct WBWinDefPresClassData *data, Object *obj)
{
	ULONG retval=0;
	Object *parent;
	ULONG currX=0, currY=0;
	ULONG thisColWidth, thisColHeight;
	ULONG objWidth, objHeight;

	parent=_parent(obj);

	currX=data->lastAddedX;
	currY=data->lastAddedY+data->lastAddedH;
	thisColWidth=data->biggestWidthInLastCol;
	thisColHeight=data->lastColH;

	if(currY > _mheight(parent))
	{
		/* start a new column */
		currX+=thisColWidth;
		currY=0;

		data->virtualWidth+=_defwidth(obj);
		thisColWidth=_defwidth(obj);
	}
	else
	{
		if(_defwidth(obj) > thisColWidth)
			thisColWidth=_defwidth(obj);
	}

	if(currY>thisColHeight)
	{
		thisColHeight=currY;
		data->virtualHeight=currY;
	}

	objWidth=_defwidth(obj);
	objHeight=_defheight(obj);

//	printf("x: %ld, y: %ld, w: %ld, h: %ld\n", _mleft(obj)+currX, _mtop(obj)+currY, objWidth, objHeight);
	MUI_Layout(obj, _mleft(parent)+currX, _mtop(parent)+currY, objWidth, objHeight, 0);

	data->lastAddedX=currX;
	data->lastAddedY=currY;
	data->lastAddedW=objWidth;
	data->lastAddedH=objHeight;
	data->biggestWidthInLastCol=thisColWidth;
	data->lastColH=thisColHeight;

	return 1;
}

IPTR winDefPresAdd(Class *cl, Object *obj, struct opMember *msg)
{
	struct WBWinDefPresClassData *data;
	struct MemberNode *mn;
	ULONG retval=1;
	struct MUI_MinMax minMax;
	Object *container, *win, *rect, *par;
	struct Region *clippingRegion;

	minMax.MinWidth=0;
	minMax.DefWidth=0;
	minMax.MaxWidth=0;
	minMax.MinHeight=0;
	minMax.DefHeight=0;
	minMax.MaxHeight=0;

	data=(struct WBWinDefPresClassData*)INST_DATA(cl, obj);

	mn=AllocVec(sizeof(struct MemberNode), MEMF_ANY);
	mn->m_member=msg->opam_Object;
	AddTail((struct List*)&data->memberList, (struct Node*)mn);

	muiNotifyData(msg->opam_Object)->mnd_ParentObject = obj;
	DoMethod(msg->opam_Object, MUIM_ConnectParent, (IPTR)obj);

	DoSetupMethod(msg->opam_Object, muiRenderInfo(obj));

//	kprintf("About to call AskMinMax on %lx\n", msg->opam_Object);
	DoMethod(msg->opam_Object, MUIM_AskMinMax, &minMax);
    _minwidth(msg->opam_Object) = minMax.MinWidth;
    _minheight(msg->opam_Object) = minMax.MinHeight;
    _maxwidth(msg->opam_Object) = minMax.MaxWidth;
    _maxheight(msg->opam_Object) = minMax.MaxHeight;
    _defwidth(msg->opam_Object) = minMax.DefWidth;
    _defheight(msg->opam_Object) = minMax.DefHeight;

//	kprintf("MinMax: dw: %ld, dh: %ld\n", minMax.DefWidth, minMax.DefHeight);

//	kprintf("dw: %ld, dh: %ld\n", _defwidth(msg->opam_Object), _defheight(msg->opam_Object));

	layoutObject(data, msg->opam_Object);

	DoMethod(msg->opam_Object, MUIM_Show);

//	kprintf("*********** START REDRAW ***************\n");

//	win=_win(obj);
//	container=obj;
//	rect=msg->opam_Object;
//
//	kprintf("window: 0x%lx, parent: 0x%lx\n", win, _parent(win));
//	kprintf("container: 0x%lx, parent: 0x%lx\n", container, _parent(container));
//	kprintf("rect: 0x%lx, parent: 0x%lx\n", rect, _parent(rect));
//
//	get(rect,MUIA_Parent,&par);
//	kprintf("rect parent: 0x%lx\n", par);
//
//	printf("left: %ld, top: %ld, width: %ld, height: %ld\n", _mleft(obj), _mtop(obj), _mleft(obj)+_mwidth(obj), _mtop(obj)+_mheight(obj));

	data->clippingRectangle.MinX=_mleft(obj);
	data->clippingRectangle.MinY=_mtop(obj);
	data->clippingRectangle.MaxX=_mleft(obj)+_mwidth(obj);
	data->clippingRectangle.MaxY=_mtop(obj)+_mheight(obj);

	clippingRegion=NewRegion();
	OrRectRegion(clippingRegion, &data->clippingRectangle);
	InstallClipRegion(_window(obj)->WLayer, clippingRegion);

	MUI_Redraw(msg->opam_Object, MADF_DRAWOBJECT);

	InstallClipRegion(_window(obj)->WLayer, NULL);

//	kprintf("*********** FINISH REDRAW ***************\n");

	return retval;
}

IPTR winDefPresRemove(Class *cl, Object *obj, struct opMember *msg)
{
	struct WBWinDefPresClassData *data;
	struct MemberNode *mn;
	ULONG retval=0;
	BOOL found=FALSE;

	data=(struct WBWinDefPresClassData*)INST_DATA(cl, obj);

	mn=(struct MemberNode*)data->memberList.mlh_Head;
//	while(!found && mn->m_Node.mln_Succ)
//	{
///		if(mn->m_member==msg->opam_Object)
//			found=TRUE;
//		else
//			mn=(struct MemberNode*)mn->m_Node.mln_Succ;
//	}

	found=TRUE;

	if(found)
	{
		Remove((struct Node*)mn);

		DoMethod(mn->m_member, MUIM_DisconnectParent);
		DoMethod(mn->m_member, MUIM_Hide);
		DoMethod(mn->m_member, MUIM_Cleanup);

		data->justRemoved=mn->m_member;

		MUI_Redraw(obj, MADF_DRAWUPDATE);

		data->justRemoved=NULL;

		retval=1;
	}

	return retval;
}

/* TODO: a more robust icon layout system */
ULONG layoutGroup(struct WBWinDefPresClassData *data)
{
	ULONG retval=0;

	return retval;
}

ULONG winDefPresLayout(Class *cl, Object *obj, Msg msg)
{
	ULONG retval;
	struct WBWinDefPresClassData *data;

	data=INST_DATA(cl, obj);
	retval=DoSuperMethodA(cl, obj, msg);

	layoutGroup(data);

	return retval;
}

IPTR winDefPresDraw(Class *cl, Object *obj, struct MUIP_Draw *msg)
{
	struct WBWinDefPresClassData *data;
	struct MemberNode *mn;
	IPTR retval;
	struct Region *clippingRegion;

	data=(struct WBWinDefPresClassData*)INST_DATA(cl, obj);

	retval=DoSuperMethodA(cl, obj, (Msg)msg);

	if(msg->flags & MADF_DRAWOBJECT)
	{
//		printf("MADF_DRAWOBJECT\n");

		data->clippingRectangle.MinX=_mleft(obj);
		data->clippingRectangle.MinY=_mtop(obj);
		data->clippingRectangle.MaxX=_mleft(obj)+_mwidth(obj);
		data->clippingRectangle.MaxY=_mtop(obj)+_mheight(obj);

		clippingRegion=NewRegion();
		OrRectRegion(clippingRegion, &data->clippingRectangle);
		InstallClipRegion(_window(obj)->WLayer, clippingRegion);

		//SetAttrs(data->horizProp, MUIA_Prop_Entries, 100, MUIA_Prop_Visible, 5, TAG_END);

		mn=(struct MemberNode*)data->memberList.mlh_Head;
		while(mn->m_Node.mln_Succ)
		{
			MUI_Redraw(mn->m_member, MADF_DRAWOBJECT);
			mn=(struct MemberNode*)mn->m_Node.mln_Succ;
		}

		InstallClipRegion(_window(obj)->WLayer, NULL);

	}
	else if(msg->flags & MADF_DRAWUPDATE)
	{
//		printf("MADF_DRAWUPDATE\n");

		if(data->justRemoved)
		{
			/* USE MUIM_DrawBackground here instead */
			SetAPen(_rp(obj), _dri(obj)->dri_Pens[BACKGROUNDPEN]);
			RectFill(_rp(obj), _left(data->justRemoved), _top(data->justRemoved), _right(data->justRemoved), _bottom(data->justRemoved));
		}
	}
//	else
//		printf("MADF_unknown\n");

	return retval;
}

IPTR winDefPresSetup(Class *cl, Object *obj, struct MUIP_Setup *msg)
{
	IPTR retval;
	struct MemberNode *mn;
	struct WBWinDefPresClassData *data;

	data=(struct WBWinDefPresClassData*)INST_DATA(cl, obj);

	retval=DoSuperMethodA(cl, obj, (Msg)msg);

	mn=(struct MemberNode*)data->memberList.mlh_Head;
	while(mn->m_Node.mln_Succ)
	{
		DoSetupMethod(mn->m_member, msg->RenderInfo);
		mn=(struct MemberNode*)mn->m_Node.mln_Succ;
	}

	MUI_RequestIDCMP(obj, IDCMP_MOUSEBUTTONS);

	return retval;
}

IPTR winDefPresAskMinMax(Class *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
	IPTR retval;

	retval=DoSuperMethodA(cl, obj, (Msg)msg);

	broadcastMessage(cl, obj, (Msg)msg);

	msg->MinMaxInfo->MinWidth+=20;
	msg->MinMaxInfo->DefWidth+=300;
	msg->MinMaxInfo->MaxWidth=MUI_MAXMAX;
	msg->MinMaxInfo->MinHeight+=20;
	msg->MinMaxInfo->DefHeight+=300;
	msg->MinMaxInfo->MaxHeight=MUI_MAXMAX;

	return retval;
}

IPTR winDefPresSet(Class *cl, Object *obj, struct opSet *ops)
{
	IPTR retval=0;
	struct WBWinDefPresClassData *data;
	struct TagItem *tag;

	data=(struct WBWinDefPresClassData*)INST_DATA(cl, obj);

	tag=FindTagItem(WBA_WinPresentation_ViewPointX, ops->ops_AttrList);
	if(tag)
	{
		tag->ti_Tag=TAG_IGNORE;
		data->viewPointX=(BOOL)tag->ti_Data;
		printf("viewPointX: %ld\n", data->viewPointX);
	}
	tag=FindTagItem(WBA_WinPresentation_ViewPointY, ops->ops_AttrList);
	if(tag)
	{
		tag->ti_Tag=TAG_IGNORE;
		data->viewPointY=(BOOL)tag->ti_Data;
		printf("viewPointY: %ld\n", data->viewPointY);
	}
	tag=FindTagItem(WBA_WinPresentation_Clicked, ops->ops_AttrList);
	if(tag)
	{
		data->clicked=(struct IntuiMessage*)tag->ti_Data;
	}

	retval=DoSuperMethodA(cl, obj, (Msg)ops);

	broadcastMessage(cl, obj, (Msg)ops);

	return retval;
}

IPTR winDefPresHandleInput(Class *cl, Object *obj, struct MUIP_HandleInput *msg)
{
	IPTR retval=0;
	Object *semanticObject;
	struct WBWinDefPresClassData *data;

	data=(struct WBWinDefPresClassData*)INST_DATA(cl, obj);

	if(msg->imsg->Class==IDCMP_MOUSEBUTTONS)
	{
		if(msg->imsg->Code==IECODE_LBUTTON)
			SetAttrs(obj, WBA_WinPresentation_Clicked, msg->imsg, TAG_END);
	}

	return retval;
}

IPTR winDefPresGet(Class *cl, Object *obj, struct opGet *opg)
{
	IPTR retval=0;
	struct WBWinDefPresClassData *data;

	data=(struct WBWinDefPresClassData*)INST_DATA(cl, obj);

	switch(opg->opg_AttrID)
	{
		case WBA_WinPresentation_SemanticObject:
			*opg->opg_Storage=(Object*)data->semanticObject;
			break;
		case WBA_WinPresentation_Clicked:
			retval=1;
			*opg->opg_Storage=(struct IntuiMessage*)data->clicked;
			break;
		default:
			retval=DoSuperMethodA(cl, obj, (Msg)opg);
			break;
	}

	return retval;
}

AROS_UFH3(IPTR, wbWindowDefaultPresDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
{
	ULONG retval=0;

	switch(msg->MethodID)
	{
		case OM_NEW:
			retval=winDefPresNew(cl, obj, (struct opSet*)msg);
			break;
		case OM_ADDMEMBER:
//			printf("OM_ADDMEMBER\n");
			retval=winDefPresAdd(cl, obj, (struct opMember*)msg);
			break;
		case OM_REMMEMBER:
//			printf("OM_REMMEMBER\n");
			retval=winDefPresRemove(cl, obj, (struct opMember*)msg);
			break;
		case MUIM_Setup:
//			printf("MUIM_Setup\n");
			retval=winDefPresSetup(cl, obj, (struct MUIP_Setup*)msg);
			break;
		case MUIM_Draw:
//			printf("MUIM_Draw\n");
			retval=winDefPresDraw(cl, obj, (struct MUIP_Draw*)msg);
			break;
		case MUIM_AskMinMax:
//			printf("MUIM_AskMinMax\n");
			retval=winDefPresAskMinMax(cl, obj, (struct MUIP_AskMinMax*)msg);
			break;
		case MUIM_Show:
//			printf("MUIM_Show\n");
			retval=DoSuperMethodA(cl, obj, msg);
			broadcastMessage(cl, obj, msg);
			break;
		case MUIM_Layout:
//			printf("MUIM_Layout\n");
			winDefPresLayout(cl, obj, msg);
			break;
		case OM_GET:
			retval=winDefPresGet(cl, obj, (struct opGet*)msg);
			break;
		case MUIM_DrawBackground:
			retval=DoSuperMethodA(cl, obj, msg);
			break;
		case OM_SET:
			retval=winDefPresSet(cl, obj, (struct opSet*)msg);
			break;
		case MUIM_HandleInput:
			retval=winDefPresHandleInput(cl, obj, (struct MUIP_HandleInput*)msg);
			break;
		default:
//			printf("Unknown method\n");
			retval=DoSuperMethodA(cl, obj, msg);
			broadcastMessage(cl, obj, msg);
			break;
	}

	return retval;
}



