/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <libraries/mui.h>
#include <zune/customclasses.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "iconwindow.h"

extern CONST_STRPTR rootBG, dirsBG;


/*** Instance Data **********************************************************/
struct IconWindow_DATA
{
    Object *iconlist; /* The iconlist it displays */
    int is_root;
    int is_backdrop;
    struct Hook *action_hook;
};

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct IconWindow_DATA *data = INST_DATA(CLASS, self)


STATIC IPTR IconWindow__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Hook *action_hook;
    int is_root, is_backdrop;
    struct IconWindow_DATA *data;

    Object *iconlist;

    /* More than one GetTagData is not really efficient but since this is called very unoften... */
    is_backdrop = (int)GetTagData(MUIA_IconWindow_IsBackdrop, 0, msg->ops_AttrList);
    is_root = (int)GetTagData(MUIA_IconWindow_IsRoot, 0, msg->ops_AttrList);
    action_hook = (struct Hook*)GetTagData(MUIA_IconWindow_ActionHook, NULL, msg->ops_AttrList);

    if (is_root)
    {
        iconlist = MUI_NewObject
        (
            MUIC_IconVolumeList,
            MUIA_Background,     (IPTR) rootBG, 
            TAG_DONE
        );
    }
    else
    {
	STRPTR drw = (STRPTR) GetTagData(MUIA_IconWindow_Drawer,NULL,msg->ops_AttrList);
	iconlist = MUI_NewObject
        (
            MUIC_IconDrawerList,
            MUIA_Background,            (IPTR) dirsBG,
            MUIA_IconDrawerList_Drawer, (IPTR) drw, 
            TAG_DONE
        );
    }

    /* Now call the super methods new method with additional tags */
    obj = (Object*)DoSuperNewTags(cl,obj,NULL,
	MUIA_Window_Width,300,
	MUIA_Window_Height,300,
	MUIA_Window_ScreenTitle, (IPTR) "",
	WindowContents, VGroup,
		InnerSpacing(0,0),
		MUIA_Group_Child, MUI_NewObject(MUIC_IconListview, MUIA_IconListview_UseWinBorder, TRUE, MUIA_IconListview_IconList, iconlist, TAG_DONE),
		End,
	TAG_MORE, (IPTR)msg->ops_AttrList);
    if (!obj) return NULL;

    /* Get and initialize the instance's data */
    data = (struct IconWindow_DATA*)INST_DATA(cl,obj);
    data->iconlist = iconlist;
    data->is_root = is_root;
    data->action_hook = action_hook;

    data->is_backdrop = -1;
    set(obj, MUIA_IconWindow_IsBackdrop, is_backdrop);

    /* If double clicked then we call our own private methods, that's easier then using Hooks */
    DoMethod(iconlist, MUIM_Notify, MUIA_IconList_DoubleClick,TRUE, obj, 1, MUIM_IconWindow_DoubleClicked);
    DoMethod(iconlist, MUIM_Notify, MUIA_IconList_IconsDropped, MUIV_EveryTime, obj, 1, MUIM_IconWindow_IconsDropped);
    DoMethod(iconlist, MUIM_Notify, MUIA_IconList_Clicked, MUIV_EveryTime, obj, 1, MUIM_IconWindow_Clicked);
    return (IPTR)obj;
}

STATIC IPTR IconWindow__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct IconWindow_DATA *data = (struct IconWindow_DATA*)INST_DATA(cl,obj);
    struct TagItem        *tags = msg->ops_AttrList;
    struct TagItem        *tag;

    while ((tag = NextTagItem((struct TagItem **)&tags)) != NULL)
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_IconWindow_IsBackdrop:
		    if ((!!tag->ti_Data) != data->is_backdrop)
		    {
		    	int is_open = XGET(obj, MUIA_Window_Open);
		    	if (is_open) set(obj, MUIA_Window_Open, FALSE);
		        if (tag->ti_Data)
		        {
		            SetAttrs(obj,
				MUIA_Window_Title, NULL,
				MUIA_Window_UseBottomBorderScroller, FALSE,
				MUIA_Window_UseRightBorderScroller, FALSE,
				MUIA_Window_WandererBackdrop, TRUE,
				TAG_DONE);
				
		        } else
		        {
			    char *title;
			    if (data->is_root) title = "Wanderer";
			    else title = (char*)XGET(data->iconlist, MUIA_IconDrawerList_Drawer);

			    SetAttrs(obj,
				MUIA_Window_Title,title,
				MUIA_Window_UseBottomBorderScroller, TRUE,
				MUIA_Window_UseRightBorderScroller, TRUE,
				MUIA_Window_WandererBackdrop, FALSE,
				TAG_DONE);
			}
		        data->is_backdrop = !!tag->ti_Data;
		    	if (is_open) set(obj, MUIA_Window_Open, TRUE);
		     }
		     break;

	}
    }
    return DoSuperMethodA(cl,obj,(Msg)msg);
}


STATIC IPTR IconWindow__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct IconWindow_DATA *data = (struct IconWindow_DATA*)INST_DATA(cl,obj);
    switch (msg->opg_AttrID)
    {
	case	MUIA_IconWindow_Drawer:
		if (!data->is_root)
		    *msg->opg_Storage = XGET(data->iconlist,MUIA_IconDrawerList_Drawer);
		else *msg->opg_Storage = NULL;
		return 1;

	case    MUIA_IconWindow_IconList:
		*msg->opg_Storage = (ULONG)data->iconlist;
		return 1;
    }
    return DoSuperMethodA(cl,obj,(Msg)msg);
}

STATIC IPTR IconWindow__MUIM_IconWindow_DoubleClicked(struct IClass *cl, Object *obj, Msg msg)
{
    struct IconWindow_DATA *data = (struct IconWindow_DATA*)INST_DATA(cl,obj);
    if (data->action_hook)
    {
	struct IconWindow_ActionMsg msg;
	msg.type = ICONWINDOW_ACTION_OPEN;
	msg.iconlist = data->iconlist;
	msg.isroot = data->is_root;
	msg.click = NULL;
	CallHookPkt(data->action_hook,obj,&msg);
    }
    return NULL; /* irrelevant */
}

STATIC IPTR IconWindow__MUIM_IconWindow_Clicked(struct IClass *cl, Object *obj, Msg msg)
{
    struct IconWindow_DATA *data = (struct IconWindow_DATA*)INST_DATA(cl,obj);;
    if (data->action_hook)
    {
	struct IconWindow_ActionMsg msg;
	msg.type = ICONWINDOW_ACTION_CLICK;
	msg.iconlist = data->iconlist;
	msg.isroot = data->is_root;
	msg.click = (struct IconList_Click*)XGET(data->iconlist, MUIA_IconList_Clicked);
	CallHookPkt(data->action_hook,obj,&msg);
    }
    return NULL; /* irrelevant */
}

STATIC IPTR IconWindow__MUIM_IconWindow_IconsDropped(struct IClass *cl, Object *obj, Msg msg)
{
    struct IconWindow_DATA *data = (struct IconWindow_DATA*)INST_DATA(cl,obj);
    if (data->action_hook)
    {
	struct IconWindow_ActionMsg msg;
	msg.type = ICONWINDOW_ACTION_ICONDROP;
	msg.iconlist = data->iconlist;
	msg.isroot = data->is_root;
	msg.click = (struct IconList_Click*)XGET(data->iconlist, MUIA_IconList_Clicked);
	CallHookPkt(data->action_hook,obj,&msg);
    }
    return NULL;
}

STATIC IPTR IconWindow__MUIM_IconWindow_Open(struct IClass *cl, Object *obj, Msg msg)
{
    struct IconWindow_DATA *data = (struct IconWindow_DATA*)INST_DATA(cl,obj);
    DoMethod(data->iconlist,MUIM_IconList_Clear);
    set(obj,MUIA_Window_Open,TRUE);
    DoMethod(data->iconlist,MUIM_IconList_Update);
    return 1;
}

STATIC IPTR IconWindow__MUIM_IconWindow_UnselectAll(struct IClass *cl, Object *obj, Msg msg)
{
    struct IconWindow_DATA *data = (struct IconWindow_DATA*)INST_DATA(cl,obj);
    DoMethod(data->iconlist,MUIM_IconList_UnselectAll);
    return 1;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_8
(
    IconWindow, NULL, MUIC_Window, NULL,
    OM_NEW,                        struct opSet *,
    OM_SET,                        struct opSet *,
    OM_GET,                        struct opGet *,
    MUIM_IconWindow_Open,          Msg,
    MUIM_IconWindow_UnselectAll,   Msg,
    MUIM_IconWindow_DoubleClicked, Msg,
    MUIM_IconWindow_IconsDropped,  Msg,
    MUIM_IconWindow_Clicked,       Msg
);
