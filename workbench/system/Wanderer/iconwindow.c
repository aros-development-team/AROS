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

#include "wanderer.h"
#include "wandererprefs.h"
#include "iconwindow.h"

/*** Instance Data **********************************************************/
struct IconWindow_DATA
{
    Object *iconlist; /* The iconlist it displays */
    BOOL is_root;
    BOOL is_backdrop;
    struct Hook *action_hook;
};

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct IconWindow_DATA *data = INST_DATA(CLASS, self)

/*** Methods ****************************************************************/
Object *IconWindow__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    struct Hook *action_hook;
    int is_root, is_backdrop;
    Object *iconlist;
    
    /* More than one GetTagData is not really efficient but since this is called very unoften... */
    is_backdrop = GetTagData(MUIA_IconWindow_IsBackdrop, 0, message->ops_AttrList);
    is_root = GetTagData(MUIA_IconWindow_IsRoot, 0, message->ops_AttrList);
    action_hook = (struct Hook*) GetTagData(MUIA_IconWindow_ActionHook, (IPTR) NULL, message->ops_AttrList);
    
    if (is_root)
    {
        iconlist = IconVolumeListObject, End;
    }
    else
    {
        STRPTR drw = (STRPTR) GetTagData(MUIA_IconWindow_Drawer,(IPTR)NULL,message->ops_AttrList);
        iconlist = IconDrawerListObject,
            MUIA_IconDrawerList_Drawer, (IPTR) drw, 
        End;
    }
    
    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        
        MUIA_Window_Width,              300,
        MUIA_Window_Height,             300,
        MUIA_Window_ScreenTitle, (IPTR) "",
        
        WindowContents, (IPTR) VGroup,
            InnerSpacing(0,0),
            
            Child, (IPTR) IconListviewObject,
                MUIA_IconListview_UseWinBorder,        TRUE,
                MUIA_IconListview_IconList,     (IPTR) iconlist,
            End,
        End,
        
        TAG_MORE, (IPTR) message->ops_AttrList
    );
    
    if (self != NULL)
    {
        SETUP_INST_DATA;
        
        data->iconlist = iconlist;
        data->is_root = is_root;
        data->action_hook = action_hook;
        
        data->is_backdrop = -1;
        set(self, MUIA_IconWindow_IsBackdrop, is_backdrop);
        
        /*
            If double clicked then we call our own private methods, that's 
            easier then using Hooks 
        */
        DoMethod
        (
            iconlist, MUIM_Notify, MUIA_IconList_DoubleClick, TRUE, 
            (IPTR) self, 1, MUIM_IconWindow_DoubleClicked
        );
        
        DoMethod
        (
            iconlist, MUIM_Notify, MUIA_IconList_IconsDropped, MUIV_EveryTime,
            (IPTR) self, 1, MUIM_IconWindow_IconsDropped
        );
        
        DoMethod
        (
            iconlist, MUIM_Notify, MUIA_IconList_Clicked, MUIV_EveryTime,
            (IPTR) self, 1, MUIM_IconWindow_Clicked
        );
    }
    
    return self;
}

IPTR IconWindow__OM_SET(Class *CLASS, Object *self, struct opSet *message)
{
    SETUP_INST_DATA;
    struct TagItem *tstate = message->ops_AttrList, 
                   *tag;

    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_IconWindow_IsBackdrop:
                if ((!!tag->ti_Data) != data->is_backdrop)
                {
                    BOOL is_open = XGET(self, MUIA_Window_Open);
                    if (is_open) set(self, MUIA_Window_Open, FALSE);
                    if (tag->ti_Data)
                    {
                        SetAttrs
                        (
                            self,
                            MUIA_Window_Title,                   (IPTR) NULL,
                            MUIA_Window_UseBottomBorderScroller,        FALSE,
                            MUIA_Window_UseRightBorderScroller,         FALSE,
                            MUIA_Window_WandererBackdrop,               TRUE,
                            TAG_DONE
                        );
                    }
                    else
                    {
                        char *title;
                        if (data->is_root) title = "Wanderer";
                        else title = (char*)XGET(data->iconlist, MUIA_IconDrawerList_Drawer);
                        
                        SetAttrs
                        (
                            self,
                            MUIA_Window_Title,                   (IPTR) title,
                            MUIA_Window_UseBottomBorderScroller,        TRUE,
                            MUIA_Window_UseRightBorderScroller,         TRUE,
                            MUIA_Window_WandererBackdrop,               FALSE,
                            TAG_DONE
                        );
                    }
                    data->is_backdrop = !!tag->ti_Data;
                    if (is_open) set(self, MUIA_Window_Open, TRUE);
                 }
                 break;
        }
    }
    
    return DoSuperMethodA(CLASS, self, (Msg) message);
}


IPTR IconWindow__OM_GET(Class *CLASS, Object *self, struct opGet *message)
{
    SETUP_INST_DATA;
    IPTR *store = message->opg_Storage;
    IPTR  rv    = TRUE;
    
    switch (message->opg_AttrID)
    {
        case MUIA_IconWindow_Drawer:
            *store = !data->is_root
                ? XGET(data->iconlist, MUIA_IconDrawerList_Drawer)
                : (IPTR) NULL;
            break;
        
        case MUIA_IconWindow_IconList:
            *message->opg_Storage = (ULONG)data->iconlist;
            break;;
        
        default:
            rv = DoSuperMethodA(CLASS, self, (Msg) message);
    }
    
    return rv;
}

IPTR IconWindow__MUIM_Window_Setup
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    Object *prefs;
    ULONG   attribute = data->is_root 
                      ? MUIA_WandererPrefs_WorkbenchBackground
                      : MUIA_WandererPrefs_DrawerBackground;
    
    if (!DoSuperMethodA(CLASS, self, message)) return FALSE;
    
    prefs = (Object *) XGET(_app(self), MUIA_Wanderer_Prefs);
    
    SET(data->iconlist, MUIA_Background, XGET(prefs, attribute));
    DoMethod
    (
        prefs, MUIM_Notify, attribute, MUIV_EveryTime,
        (IPTR) data->iconlist, 3, MUIM_Set, MUIA_Background, MUIV_TriggerValue
    );
    
    return TRUE;
}

IPTR IconWindow__MUIM_Window_Cleanup
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    ULONG attribute = data->is_root 
                    ? MUIA_WandererPrefs_WorkbenchBackground
                    : MUIA_WandererPrefs_DrawerBackground;
    
    DoMethod
    (
        (Object *) XGET(_app(self), MUIA_Wanderer_Prefs),
        MUIM_KillNotifyObj, attribute, (IPTR) self
    );
    
    return DoSuperMethodA(CLASS, self, message);
}

IPTR IconWindow__MUIM_IconWindow_DoubleClicked
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    
    if (data->action_hook)
    {
        struct IconWindow_ActionMsg msg;
        msg.type     = ICONWINDOW_ACTION_OPEN;
        msg.iconlist = data->iconlist;
        msg.isroot   = data->is_root;
        msg.click    = NULL;
        CallHookPkt(data->action_hook,self,&msg);
    }
    
    return TRUE;
}

IPTR IconWindow__MUIM_IconWindow_Clicked
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    
    if (data->action_hook)
    {
        struct IconWindow_ActionMsg msg;
        msg.type     = ICONWINDOW_ACTION_CLICK;
        msg.iconlist = data->iconlist;
        msg.isroot   = data->is_root;
        msg.click    = (struct IconList_Click * )XGET(data->iconlist, MUIA_IconList_Clicked);
        CallHookPkt(data->action_hook,self,&msg);
    }
    
    return TRUE;
}

IPTR IconWindow__MUIM_IconWindow_IconsDropped
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    
    if (data->action_hook)
    {
        struct IconWindow_ActionMsg msg;
        msg.type     = ICONWINDOW_ACTION_ICONDROP;
        msg.iconlist = data->iconlist;
        msg.isroot   = data->is_root;
        msg.click    = (struct IconList_Click*)XGET(data->iconlist, MUIA_IconList_Clicked);
        CallHookPkt(data->action_hook,self,&msg);
    }
    
    return TRUE;
}

IPTR IconWindow__MUIM_IconWindow_Open
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    
    DoMethod(data->iconlist, MUIM_IconList_Clear);
    set(self, MUIA_Window_Open, TRUE);
    DoMethod(data->iconlist, MUIM_IconList_Update);
    
    return TRUE;
}

IPTR IconWindow__MUIM_IconWindow_UnselectAll
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    
    DoMethod(data->iconlist, MUIM_IconList_UnselectAll);
    
    return TRUE;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_10
(
    IconWindow, NULL, MUIC_Window, NULL,
    OM_NEW,                        struct opSet *,
    OM_SET,                        struct opSet *,
    OM_GET,                        struct opGet *,
    MUIM_Window_Setup,             Msg,
    MUIM_Window_Cleanup,           Msg,
    MUIM_IconWindow_Open,          Msg,
    MUIM_IconWindow_UnselectAll,   Msg,
    MUIM_IconWindow_DoubleClicked, Msg,
    MUIM_IconWindow_IconsDropped,  Msg,
    MUIM_IconWindow_Clicked,       Msg
);
