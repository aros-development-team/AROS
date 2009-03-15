/*
  Copyright  2004-2009, The AROS Development Team. All rights reserved.
  $Id$
*/

#include "portable_macros.h"
#ifdef __AROS__
#define MUIMASTER_YES_INLINE_STDARG
#endif

#define ICONWINDOW_OPTION_NOSEARCHBUTTON

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

struct panel_ToolBar_DATA
{
    Object                               *iwp_Toolbar_PrefsNotificationObject;
    Object                               *iwp_Toolbar_LocationStringObj;
#ifdef __AROS__
    struct Hook                          iwp_Toolbar_LocationStrHook;
#else
    struct Hook                          *iwp_Toolbar_LocationStrHook;
#endif
};

/*** Hook functions *********************************************************/

///panelToolBar__HookFunc_LocationStringFunc()
#ifdef __AROS__
AROS_UFH3(
  void, panelToolBar__HookFunc_LocationStringFunc,
  AROS_UFHA(struct Hook *,    hook,   A0),
  AROS_UFHA(APTR *,           obj,    A2),
  AROS_UFHA(APTR,             param,  A1)
)
{
#else
HOOKPROTO(panelToolBar__HookFunc_LocationStringFunc, void, APTR *obj, APTR param)
{
#endif
    AROS_USERFUNC_INIT

    /* Get data */
    Object                              *self = ( Object *)obj;
    Class                               *CLASS = *( Class **)param;
    STRPTR                              str = NULL;
    BPTR                                fp = (BPTR) NULL;
    struct FileInfoBlock                *fib;
    struct panel_ToolBar_DATA           *tbpanel_Private = NULL;

#warning "stegerg: doesn't allocate fib with AllocDOSObject"

    SETUP_ICONWINDOW_INST_DATA;

    /* Only change dir if it is a valid directory/volume */
    if ((tbpanel_Private = (struct panel_ToolBar_DATA *)data->iwd_TopPanel.iwp_PanelPrivate) != NULL)
    {
        GET(tbpanel_Private->iwp_Toolbar_LocationStringObj, MUIA_String_Contents, &str);

#warning "TODO: Signal that it is a wrong path"
        /* so that the user understands (here where we abort with return) */

        fib = AllocDosObject(DOS_FIB, NULL);
        if (!fib)
            return;

        if (!(fp = Lock(str, ACCESS_READ)))
        {
            FreeDosObject(DOS_FIB, fib);
            return;
        }

        if (!(Examine(fp, fib)))
        {
            UnLock (fp );
            FreeDosObject(DOS_FIB, fib);
            return;
        }

        /* Change directory! */
        if (fib->fib_DirEntryType >= 0)
        {
            SET(self, MUIA_IconWindow_Location, (IPTR)str);
        }

        UnLock(fp);

        FreeDosObject(DOS_FIB, fib);
    }

    AROS_USERFUNC_EXIT
}
#ifndef __AROS__
MakeStaticHook(Toolbar_locationstrHook, panelToolBar__HookFunc_LocationStringFunc);
#endif

/*** Main Functions ****************************************************************/

///panelToolBar__Setup()
IPTR panelToolBar__Setup(Class *CLASS, Object *self, struct opSet *message)
{
    SETUP_ICONWINDOW_INST_DATA;

    Object                              *panel_ToolBar;
    Object                              *panelToolBar_ButtonDirUp = NULL,
                                        *panelToolBar_ButtonSearch = NULL,
                                        *panelToolBar_StringLocation = NULL,
                                        *wandererPrefsObj = NULL;
    BOOL                                hasToolbar = FALSE;

    struct panel_ToolBar_DATA           *panelToolBarPrivate = NULL;

    hasToolbar = (BOOL)GetTagData(MUIA_IconWindowExt_Toolbar_Enabled, (IPTR)FALSE, message->ops_AttrList);

    if (!(!(data->iwd_Flags & IWDFLAG_ISROOT) && hasToolbar && data->iwd_TopPanel.iwp_PanelContainerObj))
        return (IPTR)NULL;

    wandererPrefsObj = (Object *)GetTagData(MUIA_Wanderer_Prefs, (IPTR) NULL, message->ops_AttrList);

#if !defined(ICONWINDOW_OPTION_NOSEARCHBUTTON)
    panelToolBar_ButtonSearch = ImageButton("", "THEME:Images/Gadgets/Prefs/Test");
#endif

    D(bug("[IW.toolbar]: %s()\n", __PRETTY_FUNCTION__));
    D(bug("[IW.toolbar] %s: App PrefsObj @ 0x%p\n", __PRETTY_FUNCTION__, wandererPrefsObj));

    if (data->iwd_TopPanel.iwp_PanelPrivate == (IPTR)NULL)
    {
        if ((data->iwd_TopPanel.iwp_PanelPrivate = (IPTR)AllocVec(sizeof(struct panel_ToolBar_DATA), MEMF_CLEAR)) == (IPTR)NULL)
            return NULL;

        panelToolBarPrivate = (struct panel_ToolBar_DATA *)data->iwd_TopPanel.iwp_PanelPrivate;

        if (wandererPrefsObj != NULL)
        {
            panelToolBarPrivate->iwp_Toolbar_PrefsNotificationObject =(Object *) DoMethod(wandererPrefsObj,
                                  MUIM_WandererPrefs_ViewSettings_GetNotifyObject,
                                  (STRPTR) "Toolbar");

            //Set up our prefs notification handlers ..
            DoMethod
            (
                panelToolBarPrivate->iwp_Toolbar_PrefsNotificationObject, MUIM_Notify, MUIA_IconWindowExt_Toolbar_Enabled, MUIV_EveryTime, 
                (IPTR)self, 3, MUIM_Set, MUIA_IconWindowExt_Toolbar_Enabled, MUIV_TriggerValue
              );
        }

        /* Create the "ToolBar" panel object .. */
        panel_ToolBar = MUI_NewObject(MUIC_Group,
            MUIA_InnerLeft,(0),
            MUIA_InnerRight,(0),
            MUIA_InnerTop,(0),
            MUIA_InnerBottom,(0),
            MUIA_Frame, MUIV_Frame_None,
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
                    Child, (IPTR)( panelToolBar_StringLocation = MUI_NewObject(MUIC_String,
                        MUIA_String_Contents, (IPTR)"",
                        MUIA_CycleChain, 1,
                        MUIA_Frame, MUIV_Frame_String,
                    TAG_DONE) ),
                TAG_DONE),
                Child, (IPTR)MUI_NewObject(MUIC_Group, MUIA_Group_Horiz, TRUE                      ,
                    MUIA_InnerLeft,(0),
                    MUIA_InnerRight,(0),
                    MUIA_InnerTop,(0),
                    MUIA_InnerBottom,(0),
                    MUIA_HorizWeight,   0,
                    MUIA_VertWeight,    100,
                    Child, (IPTR) (panelToolBar_ButtonDirUp = ImageButton("", "THEME:Images/Gadgets/Prefs/Revert")),
                    (panelToolBar_ButtonSearch ? Child : TAG_IGNORE), (IPTR) (panelToolBar_ButtonSearch),
                TAG_DONE),
            TAG_DONE),
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
        TAG_DONE);

        /* Got a toolbarpanel? setup notifies and other values are 
         copied to the data of the object */
        if ( panel_ToolBar != NULL )
        {
            SET(data->iwd_TopPanel.iwp_PanelContainerObj, MUIA_ShowMe, TRUE);

            SET(panelToolBar_ButtonDirUp, MUIA_Background, XGET( panel_ToolBar, MUIA_Background ) );
            SET(panelToolBar_ButtonDirUp, MUIA_CycleChain, 1);
            SET(panelToolBar_ButtonDirUp, MUIA_Frame, MUIV_Frame_None );
#if !defined(ICONWINDOW_OPTION_NOSEARCHBUTTON)
            SET(panelToolBar_ButtonSearch, MUIA_Background, XGET( panel_ToolBar, MUIA_Background ) );
            SET(panelToolBar_ButtonSearch, MUIA_CycleChain, 1);
            SET(panelToolBar_ButtonSearch, MUIA_Frame, MUIV_Frame_None );
#endif

            if (DoMethod(data->iwd_TopPanel.iwp_PanelGroupObj, MUIM_Group_InitChange ))
            {
                DoMethod(data->iwd_TopPanel.iwp_PanelGroupObj, OM_ADDMEMBER, (IPTR)panel_ToolBar);
                if (data->iwd_TopPanel.iwp_PanelGroupSpacerObj)
                {
                    DoMethod(data->iwd_TopPanel.iwp_PanelGroupObj, OM_REMMEMBER, (IPTR)data->iwd_TopPanel.iwp_PanelGroupSpacerObj);
                    data->iwd_TopPanel.iwp_PanelGroupSpacerObj = NULL;
                }

                DoMethod(data->iwd_TopPanel.iwp_PanelGroupObj, MUIM_Group_ExitChange);
                data->iwd_PanelObj_ToolBar = panel_ToolBar;
            }

            if (data->iwd_PanelObj_ToolBar)
            {
                DoMethod( 
                    panelToolBar_ButtonDirUp, MUIM_Notify, MUIA_Pressed, FALSE, 
                    (IPTR)self, 1, MUIM_IconWindow_DirectoryUp
                  );

                panelToolBarPrivate->iwp_Toolbar_LocationStringObj = panelToolBar_StringLocation;
#ifdef __AROS__
                panelToolBarPrivate->iwp_Toolbar_LocationStrHook.h_Entry = ( HOOKFUNC )panelToolBar__HookFunc_LocationStringFunc;
#else
                panelToolBarPrivate->iwp_Toolbar_LocationStrHook= &Toolbar_locationstrHook;
#endif

                NNSET(
                    panelToolBarPrivate->iwp_Toolbar_LocationStringObj, MUIA_String_Contents, 
                    XGET(data->iwd_IconListObj, MUIA_IconDrawerList_Drawer)
                  );

                /* Make changes to string contents change dir on enter */
                DoMethod ( 
                    panelToolBar_StringLocation, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, 
                    (IPTR)self, 3, MUIM_CallHook, &panelToolBarPrivate->iwp_Toolbar_LocationStrHook, (IPTR)CLASS
                  );
            }
        }
        else
        {
            data->iwd_PanelObj_ToolBar = NULL;
        }
    }
    return NULL;
}
///

IPTR panelToolBar__Cleanup(Class *CLASS, Object *self, struct opSet *message)
{
    SETUP_ICONWINDOW_INST_DATA;

    struct panel_ToolBar_DATA *panelToolBarPrivate = (struct panel_ToolBar_DATA *)data->iwd_TopPanel.iwp_PanelPrivate;

    D(bug("[IW.toolbar]: %s()\n", __PRETTY_FUNCTION__));

    DoMethod
      (
        panelToolBarPrivate->iwp_Toolbar_PrefsNotificationObject,
        MUIM_KillNotifyObj, MUIA_IconWindowExt_Toolbar_Enabled, (IPTR) self
      );
}

///OM_SET()
IPTR panelToolBar__OM_SET(Class *CLASS, Object *self, struct opSet *message)
{
    SETUP_ICONWINDOW_INST_DATA;

    struct TagItem  *tstate = message->ops_AttrList, *tag;
    BOOL      UpdateIconlist = FALSE;
    IPTR      focusicon = (IPTR) NULL;
    IPTR        rv = TRUE;

    D(bug("[IW.toolbar]: %s()\n", __PRETTY_FUNCTION__));

    while ((tag = NextTagItem((TAGITEM)&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_IconWindowExt_Toolbar_Enabled:
            D(bug("[IW.toolbar] %s: MUIA_IconWindowExt_Toolbar_Enabled\n", __PRETTY_FUNCTION__));
            if ((!(data->iwd_Flags & IWDFLAG_ISROOT)) && (data->iwd_TopPanel.iwp_PanelContainerObj))
            {
                // remove toolbar
                if (!(( BOOL )tag->ti_Data))
                {
                    //Force classic navigation when the toolbar is disabled ..
                    Object *prefs = NULL;

                    if (data->iwd_PanelObj_ToolBar != NULL)
                    {
                        data->iwd_TopPanel.iwp_PanelGroupSpacerObj = HSpace(0);
            
                        SET(data->iwd_TopPanel.iwp_PanelContainerObj, MUIA_Frame, MUIV_Frame_None);
                        SET(data->iwd_TopPanel.iwp_PanelContainerObj, MUIA_Group_Spacing, 0);

                        if ((data->iwd_TopPanel.iwp_PanelGroupSpacerObj) && (DoMethod(data->iwd_TopPanel.iwp_PanelGroupObj, MUIM_Group_InitChange)))
                        {
                            DoMethod(data->iwd_TopPanel.iwp_PanelGroupObj, OM_REMMEMBER, (IPTR)data->iwd_PanelObj_ToolBar);
                            DoMethod(data->iwd_TopPanel.iwp_PanelGroupObj, OM_ADDMEMBER, (IPTR)data->iwd_TopPanel.iwp_PanelGroupSpacerObj);
                            DoMethod(data->iwd_TopPanel.iwp_PanelGroupObj, MUIM_Group_ExitChange);
                            data->iwd_PanelObj_ToolBar = NULL;
                        }
                    }

                    GET(_app(self), MUIA_Wanderer_Prefs, &prefs);
                    if (prefs)
                    {
                        SET(prefs, MUIA_IconWindowExt_Toolbar_NavigationMethod, WPD_NAVIGATION_CLASSIC);
                    }
                }
                else
                {
                    // setup toolbar
                    if (data->iwd_PanelObj_ToolBar == NULL)
                    {
                        Object *prefs = NULL;
                        GET(_app(self), MUIA_Wanderer_Prefs, &prefs);
                        panelToolBar__Setup(CLASS, self, prefs);
                    }
                }
                data->iwd_Flags |= (tag->ti_Data) ? IWDFLAG_EXT_TOOLBARENABLED : 0;
            }
            break;
        case MUIA_IconWindow_Location:
            D(bug("[iconwindow] %s: MUIA_IconWindow_Location [drawer '%s']\n", __PRETTY_FUNCTION__, data->iwd_DirectoryPath));

            if ((!(data->iwd_Flags & IWDFLAG_ISROOT)) && (data->iwd_TopPanel.iwp_PanelPrivate))
            {
                struct panel_ToolBar_DATA *panelToolBarPrivate = (struct panel_ToolBar_DATA *)data->iwd_TopPanel.iwp_PanelPrivate;

                SET(panelToolBarPrivate->iwp_Toolbar_LocationStringObj, MUIA_String_Contents, (IPTR)data->iwd_DirectoryPath);
            }
            break;
        }
    }

    return rv;
}

///OM_GET()
IPTR panelToolBar__OM_GET(Class *CLASS, Object *self, struct opGet *message)
{
    SETUP_ICONWINDOW_INST_DATA;

    IPTR *store = message->opg_Storage;
    IPTR  rv    = TRUE;

    D(bug("[IW.toolbar]: %s()\n", __PRETTY_FUNCTION__));

    switch (message->opg_AttrID)
    {
        case MUIA_IconWindowExt_Toolbar_Enabled:
        *store = (IPTR)(data->iwd_Flags & IWDFLAG_EXT_TOOLBARENABLED);
        break;
    }
}

#define PANELTOOLBAR_PRIORITY 10

static const UBYTE extension_Name[] = "ToolBar Extension";
static struct iconWindow_Extension panelToolBar__Extension;

IPTR panelToolBar__Init()
{
    D(bug("[IW.toolbar]: %s()\n", __PRETTY_FUNCTION__));

    panelToolBar__Extension.iwe_Node.ln_Pri = PANELTOOLBAR_PRIORITY;
    panelToolBar__Extension.iwe_Node.ln_Name = extension_Name;
    panelToolBar__Extension.iwe_Setup = panelToolBar__Setup;
    panelToolBar__Extension.iwe_Cleanup = panelToolBar__Cleanup;
    panelToolBar__Extension.iwe_Set = panelToolBar__OM_SET;
    panelToolBar__Extension.iwe_Get = panelToolBar__OM_GET;

    Enqueue(&iconwindow_Extensions, (struct Node *)&panelToolBar__Extension);

    D(bug("[IconWindow] %s: Added Extension '%s' @ %p to list @ %p\n", __PRETTY_FUNCTION__, panelToolBar__Extension.iwe_Node.ln_Name, &panelToolBar__Extension.iwe_Node, &iconwindow_Extensions));
    
    return TRUE;
}

ADD2INIT(panelToolBar__Init, PANELTOOLBAR_PRIORITY);
