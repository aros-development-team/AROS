/*
  Copyright  2004-2009, The AROS Development Team. All rights reserved.
  $Id$
*/

#include "portable_macros.h"
#ifdef __AROS__
#define MUIMASTER_YES_INLINE_STDARG
#endif

#define ICONWINDOW_OPTION_NOSEARCHBUTTON
#define ICONWINDOW_NODETAILVIEWCLASS
//#define ICONWINDOW_BUFFERLIST

#ifdef __AROS__
#define DEBUG 0
#include <aros/debug.h>
#endif

#include <exec/types.h>
#include <libraries/mui.h>

#ifdef __AROS__
#include <zune/customclasses.h>
#else
#include <zune_AROS/customclasses.h>
#endif

#include <proto/utility.h>

#include <proto/graphics.h>

#include <proto/exec.h>
#include <proto/datatypes.h>

#include <dos/dos.h>
#include <proto/dos.h>

#include <proto/icon.h>

#include <stdio.h>
#include <string.h>

#include <intuition/screens.h>
#include <datatypes/pictureclass.h>
#include <clib/macros.h>

#ifdef __AROS__
#include <clib/alib_protos.h>

#include <prefs/wanderer.h>
#else
#include <prefs_AROS/wanderer.h>
#endif

#if defined(__AMIGA__) && !defined(__PPC__)
#define NO_INLINE_STDARG
#endif
#ifndef _PROTO_INTUITION_H
#include <proto/intuition.h>
#endif
#include <proto/muimaster.h>

#include "Classes/iconlist.h"
#include "Classes/iconlistview.h"
#include "Classes/iconlist_attributes.h"
#include "Classes/icon_attributes.h"
#include "wanderer.h"
#include "wandererprefs.h"

#include "iconwindow.h"
#include "iconwindow_attributes.h"

extern struct List                     iconwindow_Extensions;

/*** Private Data *********************************************************/

#warning "TODO: Toolbars Attributes etc should be in an own file"
/*** Identifier Base ********************************************************/
#define MUIB_IconWindowExt_Toolbar                            (MUIB_IconWindowExt | 0x200000)

#define MUIA_IconWindowExt_Toolbar_Enabled                    (MUIB_IconWindowExt_Toolbar | 0x00000001) /* ISG */
#define MUIA_IconWindowExt_Toolbar_NavigationMethod           (MUIB_IconWindowExt_Toolbar | 0x00000002) /* ISG */
/*** Variables **************************************************************/

struct panel_StatusBar_DATA
{
    Object                               *iwp_StatusBar_StatusTextObj;
#ifdef __AROS__
    struct Hook                          iwp_StatusBar_updateHook;
#else
    struct Hook                          *iwp_StatusBar_updateHook;
#endif
};

/*** Hook functions *********************************************************/

///panelStatusBar__HookFunc_UpdateStatusFunc()
#ifdef __AROS__
AROS_UFH3(
  void, panelStatusBar__HookFunc_UpdateStatusFunc,
  AROS_UFHA(struct Hook *,    hook,   A0),
  AROS_UFHA(APTR *,           obj,    A2),
  AROS_UFHA(APTR,             param,  A1)
)
{
#else
HOOKPROTO(panelStatusBar__HookFunc_UpdateStatusFunc, void, APTR *obj, APTR param)
{
#endif
    AROS_USERFUNC_INIT

    /* Get data */
    Object                              *self = ( Object *)obj;
    Class                               *CLASS = *( Class **)param;
    STRPTR                              str = NULL;
    BPTR                                fp = (BPTR) NULL;
    struct FileInfoBlock                *fib;
    struct panel_StatusBar_DATA         *sbpanel_Private = NULL;

    SETUP_ICONWINDOW_INST_DATA;

    D(bug("[IW.statusbar]: %s()\n", __PRETTY_FUNCTION__));

    /* Only change dir if it is a valid directory/volume */
    if ((sbpanel_Private = (struct panel_StatusBar_DATA *)data->iwd_BottomPanel.iwp_PanelPrivate) != NULL)
    {
        struct List *iconList = NULL;
        struct IconEntry    *icon = NULL;
        UBYTE buffer[1024];

        int files = 0, dirs = 0, hidden = 0;
        ULONG size = 0;

        GET(data->iwd_IconListObj, MUIA_Group_ChildList, &iconList);
        if (iconList)
        {
            ForeachNode(iconList, icon)
            {
                if (icon->ie_Flags & ICONENTRY_FLAG_VISIBLE)
                {
                    if (icon->ie_IconListEntry.type == ST_USERDIR)
                    {
                        dirs += 1;
                    }
                    else
                    {
                        files += 1;
                        size += icon->ie_FileInfoBlock->fib_Size;
                    }
                }
                else
                {
                    hidden += 1;
                }
            }
        }
        sprintf(buffer, "%dbytes in %d files, %d drawers (%d hidden)", size, files, dirs, hidden);
        SET(sbpanel_Private->iwp_StatusBar_StatusTextObj, MUIA_Text_Contents, (IPTR)buffer);
    }

    AROS_USERFUNC_EXIT
}
#ifndef __AROS__
MakeStaticHook(StatusBar_updateHook, panelStatusBar__HookFunc_UpdateStatusFunc);
#endif

/*** Main Functions ****************************************************************/

///panelStatusBar__Setup()
IPTR panelStatusBar__Setup(Class *CLASS, Object *self, struct opSet *message)
{
    SETUP_ICONWINDOW_INST_DATA;

    Object                              *panel_StatusBar = NULL,
                                        *panelStatusBar_TextStatus = NULL,
                                        *wandererPrefsObj = NULL;
    BOOL                                hasToolbar = FALSE;

    struct panel_StatusBar_DATA         *panelStatusBarPrivate = NULL;

    hasToolbar = (BOOL)GetTagData(MUIA_IconWindowExt_Toolbar_Enabled, (IPTR)FALSE, message->ops_AttrList);

    if (!(!(data->iwd_Flags & IWDFLAG_ISROOT) && hasToolbar && data->iwd_BottomPanel.iwp_PanelContainerObj))
        return NULL;

    wandererPrefsObj = (Object *)GetTagData(MUIA_Wanderer_Prefs, (IPTR) NULL, message->ops_AttrList);

    D(bug("[IW.statusbar]: %s()\n", __PRETTY_FUNCTION__));
    D(bug("[IW.statusbar] %s: App PrefsObj @ 0x%p\n", __PRETTY_FUNCTION__, wandererPrefsObj));

    if (data->iwd_BottomPanel.iwp_PanelPrivate == NULL)
    {
        if ((data->iwd_BottomPanel.iwp_PanelPrivate = AllocVec(sizeof(struct panel_StatusBar_DATA), MEMF_CLEAR)) == NULL)
            return;

        panelStatusBarPrivate = (struct panel_StatusBar_DATA *)data->iwd_BottomPanel.iwp_PanelPrivate;

        if (wandererPrefsObj != NULL)
        {
        /*    data->iwd_Toolbar_PrefsNotificationObject =(Object *) DoMethod(wandererPrefsObj,
                                  MUIM_WandererPrefs_ViewSettings_GetNotifyObject,
                                  (STRPTR) "StatusBar");
        */
        }

        /* Create the "StatusBar" panel object .. */
        panel_StatusBar = MUI_NewObject(MUIC_Group,
            MUIA_InnerLeft,(0),
            MUIA_InnerRight,(0),
            MUIA_InnerTop,(0),
            MUIA_InnerBottom,(0),
            MUIA_Frame, MUIV_Frame_None,
            Child, (IPTR)MUI_NewObject(MUIC_Group, MUIA_Group_Horiz, TRUE                      ,
                MUIA_InnerLeft,(0),
                MUIA_InnerRight,(0),
                MUIA_InnerTop,(0),
                MUIA_InnerBottom,(0),
                MUIA_Group_Spacing, 0,
                MUIA_FixHeight, 1,
                MUIA_Frame, MUIV_Frame_None,
                MUIA_Background, MUII_SHADOW,
                Child, (IPTR)MUI_NewObject(MUIC_Rectangle,
                    MUIA_Frame, MUIV_Frame_None,
                TAG_DONE),
            TAG_DONE),
            Child, (IPTR)MUI_NewObject(MUIC_Group, MUIA_Group_Horiz, TRUE                      ,
                MUIA_InnerLeft,(4),
                MUIA_InnerRight,(4),
                MUIA_InnerTop,(4),
                MUIA_InnerBottom,(4),
                MUIA_Frame, MUIV_Frame_None,
                MUIA_Weight, 100,
                Child, (IPTR)MUI_NewObject(MUIC_Group, MUIA_Group_Horiz, TRUE                      ,
                    MUIA_InnerLeft,(0),
                    MUIA_InnerRight,(0),
                    MUIA_InnerTop,(0),
                    MUIA_InnerBottom,(0),
                    MUIA_Weight, 100,
                    Child, (IPTR)( panelStatusBar_TextStatus = MUI_NewObject(MUIC_Text,
                        MUIA_Font, MUIV_Font_Tiny,
                        MUIA_Text_Contents, (IPTR)"Status Bar!",
                    TAG_DONE) ),
                TAG_DONE),
            TAG_DONE),
        TAG_DONE);

        /* Got a StatusBarPanel? setup notifies and other values are 
         copied to the data of the object */
        if ( panel_StatusBar != NULL )
        {
            SET(data->iwd_BottomPanel.iwp_PanelContainerObj, MUIA_ShowMe, TRUE);

            panelStatusBarPrivate->iwp_StatusBar_StatusTextObj = panelStatusBar_TextStatus;

            if (DoMethod(data->iwd_BottomPanel.iwp_PanelGroupObj, MUIM_Group_InitChange ))
            {
                DoMethod(data->iwd_BottomPanel.iwp_PanelGroupObj, OM_ADDMEMBER, (IPTR)panel_StatusBar);
                if (data->iwd_BottomPanel.iwp_PanelGroupSpacerObj)
                {
                    DoMethod(data->iwd_BottomPanel.iwp_PanelGroupObj, OM_REMMEMBER, (IPTR)data->iwd_BottomPanel.iwp_PanelGroupSpacerObj);
                    data->iwd_BottomPanel.iwp_PanelGroupSpacerObj = NULL;
                }

                DoMethod(data->iwd_BottomPanel.iwp_PanelGroupObj, MUIM_Group_ExitChange);
                data->iwd_PanelObj_StatusBar = panel_StatusBar;
            }

            if (data->iwd_PanelObj_StatusBar)
            {
#ifdef __AROS__
                panelStatusBarPrivate->iwp_StatusBar_updateHook.h_Entry = ( HOOKFUNC )panelStatusBar__HookFunc_UpdateStatusFunc;
#else
                panelStatusBarPrivate->iwp_StatusBar_updateHook= &StatusBar_updateHook;
#endif
                DoMethod ( 
                    data->iwd_IconListObj, MUIM_Notify, MUIA_IconList_Changed, MUIV_EveryTime, 
                    (IPTR)self, 3, MUIM_CallHook, &panelStatusBarPrivate->iwp_StatusBar_updateHook, (IPTR)CLASS
                  );
            }
        }
        else
        {
            data->iwd_PanelObj_StatusBar = NULL;
        }
    }
}
///

///panelStatusBar__Cleanup()
IPTR panelStatusBar__Cleanup(Class *CLASS, Object *self, struct opSet *message)
{
    SETUP_ICONWINDOW_INST_DATA;

    D(bug("[IW.statusbar]: %s()\n", __PRETTY_FUNCTION__));

    return NULL;
}
///

///panelStatusBar__OM_GET()
IPTR panelStatusBar__OM_GET(Class *CLASS, Object *self, struct opGet *message)
{
    SETUP_ICONWINDOW_INST_DATA;

    D(bug("[IW.statusbar]: %s()\n", __PRETTY_FUNCTION__));

    return NULL;
}
///

///OM_SET()
IPTR panelStatusBar__OM_SET(Class *CLASS, Object *self, struct opSet *message)
{
    SETUP_ICONWINDOW_INST_DATA;

    struct TagItem  *tstate = message->ops_AttrList, *tag;
    BOOL      UpdateIconlist = FALSE;
    IPTR      focusicon = (IPTR) NULL;
    IPTR        rv = TRUE;

    D(bug("[IW.statusbar]: %s()\n", __PRETTY_FUNCTION__));

    while ((tag = NextTagItem((TAGITEM)&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_IconWindowExt_Toolbar_Enabled:   
            if ((!(data->iwd_Flags & IWDFLAG_ISROOT)) && (data->iwd_BottomPanel.iwp_PanelContainerObj))
            {
                // remove statusbar
                if (!(( BOOL )tag->ti_Data))
                {
                    if (data->iwd_PanelObj_StatusBar != NULL)
                    {
                        data->iwd_BottomPanel.iwp_PanelGroupSpacerObj = HSpace(0);
            
                        SET(data->iwd_BottomPanel.iwp_PanelContainerObj, MUIA_Frame, MUIV_Frame_None);
                        SET(data->iwd_BottomPanel.iwp_PanelContainerObj, MUIA_Group_Spacing, 0);

                        if ((data->iwd_BottomPanel.iwp_PanelGroupSpacerObj) && (DoMethod(data->iwd_BottomPanel.iwp_PanelGroupObj, MUIM_Group_InitChange)))
                        {
                            DoMethod(data->iwd_BottomPanel.iwp_PanelGroupObj, OM_REMMEMBER, (IPTR)data->iwd_PanelObj_StatusBar);
                            DoMethod(data->iwd_BottomPanel.iwp_PanelGroupObj, OM_ADDMEMBER, (IPTR)data->iwd_BottomPanel.iwp_PanelGroupSpacerObj);
                            DoMethod(data->iwd_BottomPanel.iwp_PanelGroupObj, MUIM_Group_ExitChange);
                            data->iwd_PanelObj_StatusBar = NULL;
                        }
                    }
                }
                else
                {
                    // setup toolbar
                    if (data->iwd_PanelObj_StatusBar == NULL)
                    {
                        Object *prefs = NULL;
                        GET(_app(self), MUIA_Wanderer_Prefs, &prefs);
                        panelStatusBar__Setup(CLASS, self, prefs);
                    }
                }
            }
            break;   
        }
    }

    return rv;
}

#define PANELSTATUSBAR_PRIORITY 10

static const UBYTE extension_Name[] = "StatusBar Extension";
static struct iconWindow_Extension panelStatusBar__Extension;

IPTR panelStatusBar__Init()
{
    D(bug("[IW.statusbar]: %s()\n", __PRETTY_FUNCTION__));

    panelStatusBar__Extension.iwe_Node.ln_Pri = PANELSTATUSBAR_PRIORITY;
    panelStatusBar__Extension.iwe_Node.ln_Name = extension_Name;
    panelStatusBar__Extension.iwe_Setup = panelStatusBar__Setup;
    panelStatusBar__Extension.iwe_Cleanup = panelStatusBar__Cleanup;
    panelStatusBar__Extension.iwe_Set = panelStatusBar__OM_SET;
    panelStatusBar__Extension.iwe_Get = panelStatusBar__OM_GET;

    Enqueue(&iconwindow_Extensions, (struct Node *)&panelStatusBar__Extension);

    D(bug("[IconWindow] %s: Added Extension '%s' @ %p to list @ %p\n", __PRETTY_FUNCTION__, panelStatusBar__Extension.iwe_Node.ln_Name, &panelStatusBar__Extension.iwe_Node, &iconwindow_Extensions));

    return TRUE;
}

ADD2INIT(panelStatusBar__Init, PANELSTATUSBAR_PRIORITY);
