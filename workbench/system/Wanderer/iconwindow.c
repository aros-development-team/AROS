/*
    Copyright © 2004-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG
#define DEBUG 0

#include <exec/types.h>
#include <libraries/mui.h>
#include <zune/customclasses.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/muimaster.h>

#include "wanderer.h"
#include "wandererprefs.h"
#include "iconwindow.h"


#include <aros/debug.h>

/*** Instance Data **********************************************************/
struct IconWindow_DATA
{
    Object           *iwd_IconList;
    Object           *iwd_toolbarPanel;
    BOOL              iwd_IsRoot;
    BOOL              iwd_IsBackdrop;
    BOOL              iwd_hasToolbar;
    struct Hook      *iwd_ActionHook;
    struct TextFont  *iwd_WindowFont;
    
    char              directory_path[1024];
};

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct IconWindow_DATA *data = INST_DATA(CLASS, self)

/*** Methods ****************************************************************/
Object *IconWindow__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    Object           *iconList, *toolbarPanel, *bt_dirup, *bt_search;
    BOOL              isRoot,
                      isBackdrop,
                      hasToolbar;
    struct Hook      *actionHook;
    struct TextFont  *WindowFont;

    /* More than one GetTagData is not really efficient but since this is called very unoften... */
    hasToolbar = GetTagData(MUIA_IconWindow_Toolbar_Enabled, 0, message->ops_AttrList);
    isBackdrop = GetTagData(MUIA_IconWindow_IsBackdrop, 0, message->ops_AttrList);
    isRoot = GetTagData(MUIA_IconWindow_IsRoot, 0, message->ops_AttrList);
    
    actionHook = (struct Hook *) GetTagData(MUIA_IconWindow_ActionHook, (IPTR) NULL, message->ops_AttrList);
    WindowFont = (struct TextFont *) GetTagData(MUIA_IconWindow_Font, (IPTR) NULL, message->ops_AttrList);

    if (isRoot)
    {
        iconList = (IPTR) IconVolumeListObject,
            MUIA_Font, (IPTR) WindowFont,
        End;
    }
    else
    {
        STRPTR drawer = (STRPTR) GetTagData(MUIA_IconWindow_Drawer, (IPTR) NULL, message->ops_AttrList);
        iconList = (IPTR) IconDrawerListObject,
            MUIA_Font, (IPTR) WindowFont,
            MUIA_IconDrawerList_Drawer, (IPTR) drawer, 
        End;
    }

D(bug("[iconwindow] Font @ %x\n", WindowFont));

    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        
        MUIA_Window_Width,              300,
        MUIA_Window_Height,             300,
        MUIA_Window_AltWidth,           100,
        MUIA_Window_AltHeight,           80,
        MUIA_Window_ScreenTitle, (IPTR) "",
        MUIA_Font, (IPTR) WindowFont,
        
        WindowContents, (IPTR) VGroup,
            /*MUIA_Group_Spacing, 0,*/
            InnerSpacing(0,0),

            /* window navigation bar */
            Child, (IPTR) (toolbarPanel = GroupObject,
                InnerSpacing(0,0),
                MUIA_Group_Horiz, TRUE,
                Child, (IPTR) (bt_dirup = ImageButton("", "THEME:Images/Gadgets/Prefs/Revert")),
                Child, (IPTR) (bt_search = ImageButton("", "THEME:Images/Gadgets/Prefs/Test")),
            End ),
        
            /* icon list */
            Child, (IPTR) IconListviewObject,
                MUIA_IconListview_UseWinBorder,        TRUE,
                MUIA_IconListview_IconList,     (IPTR) iconList,
            End,
        End,
        
        TAG_MORE, (IPTR) message->ops_AttrList
    );
    
    if (self != NULL)
    {
        SETUP_INST_DATA;
        
        data->iwd_IconList   = iconList;
        data->iwd_toolbarPanel = toolbarPanel;        
        data->iwd_IsRoot     = isRoot;
        data->iwd_ActionHook = actionHook;
        data->iwd_IsBackdrop = -1;
        SET(self, MUIA_IconWindow_IsBackdrop, isBackdrop);
        
        data->iwd_WindowFont = WindowFont;        

        /* no tool bar when root */
        if ( (isRoot) || (!hasToolbar) )
        {
            SET(toolbarPanel, MUIA_ShowMe, FALSE);
                   
            /* remove tooblbar objects - DOESNT WORK.. WHY??? */
            /*DoMethod ( toolbarPanel, MUIM_Group_InitChange );
            DoMethod ( toolbarPanel, OM_REMMEMBER, (IPTR) bt_dirup );
            DoMethod ( toolbarPanel, OM_REMMEMBER, (IPTR) bt_search );
            DoMethod ( toolbarPanel, MUIM_Group_ExitChange );*/
        }
        else
        {
            /* bt_dirup button notification */
            DoMethod(bt_dirup, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR) self, 1, MUIM_IconWindow_DirectoryUp);            
        }
            
        /*
            If double clicked then we call our own private methods, that's 
            easier then using Hooks 
        */
        DoMethod
        (
            iconList, MUIM_Notify, MUIA_IconList_DoubleClick, TRUE, 
            (IPTR) self, 1, MUIM_IconWindow_DoubleClicked
        );
        
        DoMethod
        (
            iconList, MUIM_Notify, MUIA_IconList_IconsDropped, MUIV_EveryTime,
            (IPTR) self, 1, MUIM_IconWindow_IconsDropped
        );
        
        DoMethod
        (
            iconList, MUIM_Notify, MUIA_IconList_Clicked, MUIV_EveryTime,
            (IPTR) self, 1, MUIM_IconWindow_Clicked
        );
    }
    
    return self;
}

IPTR IconWindow__OM_SET(Class *CLASS, Object *self, struct opSet *message)
{
    SETUP_INST_DATA;
    struct TagItem *tstate = message->ops_AttrList, *tag;

    while ((tag = NextTagItem((const struct TagItem**)&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_Window_Open:
                /* Commented out for unknown reason - please elaborate here!
                {            
                IPTR retVal = DoSuperMethodA(CLASS, self, (Msg) message);
                if (data->iwd_WindowFont)
                {
D(bug("[iconwindow] MUIA_Window_Open: Setting Window Font [%x]\n", data->iwd_WindowFont));
                    SetFont(_rp(self), data->iwd_WindowFont);
                }
                return retVal;
                break;
                }
                */
            case MUIA_IconWindow_Font:
                D(bug("[iconwindow] MUIA_IconWindow_Font: Setting Window Font [%x]\n", data->iwd_WindowFont));
                data->iwd_WindowFont = (struct TextFont  *)tag->ti_Data;
                if ( data->iwd_WindowFont != 1 )
                    SetFont(_rp(self), data->iwd_WindowFont);
                // Cause the window to redraw here!
                break;
            case MUIA_IconWindow_Drawer:
                 strcpy(data->directory_path, (IPTR)tag->ti_Data);    
                 set(data->iwd_IconList, MUIA_IconDrawerList_Drawer, (IPTR) data->directory_path);
                 set(self, MUIA_Window_Title, data->directory_path);
                break;
            case MUIA_IconWindow_Toolbar_Enabled:   
                 if (!data->iwd_IsRoot)
                 {               
                     data->iwd_hasToolbar = (IPTR)tag->ti_Data;
                     set(data->iwd_toolbarPanel, MUIA_ShowMe, (IPTR)tag->ti_Data);
                     /* update the window contents; shall be reworked!! */
                     set(self, MUIA_Window_Open, FALSE); 
                     set(self, MUIA_Window_Open, TRUE);                                  
                 }                 
                 break;            
            case MUIA_IconWindow_IsBackdrop:
                if ((!!tag->ti_Data) != data->iwd_IsBackdrop)
                {
                    BOOL isOpen = XGET(self, MUIA_Window_Open);
                    if (isOpen) SET(self, MUIA_Window_Open, FALSE);
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
                        STRPTR title;
                        
                        if (data->iwd_IsRoot)
                        {
                            title = "Wanderer";
                        }
                        else
                        {
                            title = (STRPTR) XGET
                            (
                                data->iwd_IconList, 
                                MUIA_IconDrawerList_Drawer
                            );
                        }
                        
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
                    data->iwd_IsBackdrop = !!tag->ti_Data;
                    if (isOpen) SET(self, MUIA_Window_Open, TRUE);
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
            *store = !data->iwd_IsRoot
                ? XGET(data->iwd_IconList, MUIA_IconDrawerList_Drawer)
                : (IPTR) NULL;
            break;      
        case MUIA_IconWindow_IconList:
            *store = (IPTR) data->iwd_IconList;
            break;;
        case MUIA_IconWindow_Toolbar_Enabled:
            *store = XGET(data->iwd_toolbarPanel, MUIA_ShowMe);
            break; 
        case MUIA_IconWindow_IsRoot:
            *store = data->iwd_IsRoot;
            break;
             
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
    ULONG   attribute = data->iwd_IsRoot 
                    ? MUIA_WandererPrefs_WorkbenchBackground
                    : MUIA_WandererPrefs_DrawerBackground;
    
    if (!DoSuperMethodA(CLASS, self, message)) return FALSE;
    
    prefs = (Object *) XGET(_app(self), MUIA_Wanderer_Prefs);
    
    SET(data->iwd_IconList, MUIA_Background, XGET(prefs, attribute));
    DoMethod
    (
        prefs, MUIM_Notify, attribute, MUIV_EveryTime,
        (IPTR) data->iwd_IconList, 3, 
        MUIM_Set, MUIA_Background, MUIV_TriggerValue
    );
    
    return TRUE;
}

IPTR IconWindow__MUIM_Window_Cleanup
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    ULONG attribute = data->iwd_IsRoot 
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
    
    if (data->iwd_ActionHook)
    {
        struct IconWindow_ActionMsg msg;
        msg.type     = ICONWINDOW_ACTION_OPEN;
        msg.iconlist = data->iwd_IconList;
        msg.isroot   = data->iwd_IsRoot;
        msg.click    = NULL;
        CallHookPkt(data->iwd_ActionHook, self, &msg);
    }
    
    return TRUE;
}

IPTR IconWindow__MUIM_IconWindow_Clicked
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    
    if (data->iwd_ActionHook)
    {
        struct IconWindow_ActionMsg msg;
        msg.type     = ICONWINDOW_ACTION_CLICK;
        msg.iconlist = data->iwd_IconList;
        msg.isroot   = data->iwd_IsRoot;
        msg.click    = (struct IconList_Click *) XGET(data->iwd_IconList, MUIA_IconList_Clicked);
        CallHookPkt(data->iwd_ActionHook, self, &msg);
    }
    
    return TRUE;
}

IPTR IconWindow__MUIM_IconWindow_IconsDropped
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    
    if (data->iwd_ActionHook)
    {
        struct IconWindow_ActionMsg msg;
        msg.type     = ICONWINDOW_ACTION_ICONDROP;
        msg.iconlist = data->iwd_IconList;
        msg.isroot   = data->iwd_IsRoot;
        msg.drop     = (struct IconList_Drop *) XGET(data->iwd_IconList, MUIA_IconList_IconsDropped);
        CallHookPkt(data->iwd_ActionHook, self, &msg);
    }
    
    return TRUE;
}

IPTR IconWindow__MUIM_IconWindow_Open
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    
    DoMethod(data->iwd_IconList, MUIM_IconList_Clear);
    SET(self, MUIA_Window_Open, TRUE);
    SET(self, MUIA_Window_Activate, TRUE);
    DoMethod(data->iwd_IconList, MUIM_IconList_Update);
    
    return TRUE;
}

IPTR IconWindow__MUIM_IconWindow_DirectoryUp
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    
    if (data->iwd_ActionHook)
    {
        struct IconWindow_ActionMsg msg;
        msg.type     = ICONWINDOW_ACTION_DIRUP;
        msg.iconlist = data->iwd_IconList;
        msg.isroot   = data->iwd_IsRoot;
        msg.click    = NULL;
        CallHookPkt(data->iwd_ActionHook, self, &msg);
        
    }
    
    return TRUE;
}

IPTR IconWindow__MUIM_IconWindow_UnselectAll
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    
    DoMethod(data->iwd_IconList, MUIM_IconList_UnselectAll);
    
    return TRUE;
}

/*** Setup ******************************************************************/
ICONWINDOW_CUSTOMCLASS
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
    MUIM_IconWindow_Clicked,       Msg,
    MUIM_IconWindow_DirectoryUp,   Msg
);
