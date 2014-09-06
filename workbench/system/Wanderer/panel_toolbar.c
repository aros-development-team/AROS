/*
  Copyright  2004-2010, The AROS Development Team. All rights reserved.
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

static CONST_STRPTR                     extension_Name = "IconWindow ToolBar Extension";
static CONST_STRPTR                     extension_PrefsFile = "ENV:SYS/Wanderer/toolbar.prefs";
static STRPTR                           extension_PrefsData;
static struct iconWindow_Extension      panelToolBar__Extension;
struct List                             panelToolBar__ToolBars;
struct NotifyRequest                    panelToolBar__PrefsNotifyRequest;
Object                                  *panelToolBar__PrefsNotificationObject;

struct panel_ToolBar_DATA
{
    struct Node                         iwp_Node;

    Object                              *iwp_ToolBar_ToolBarObj;
    Object                              *iwp_ToolBar_LocationStringObj;
#ifdef __AROS__
    struct Hook                         iwp_ToolBar_LocationStrHook;
#else
    struct Hook                         *iwp_ToolBar_LocationStrHook;
#endif
};

///ExpandEnvName()
/* Expand a passed in env: string to its full location */
/* Wanderer doesnt free this mem at the moment but should 
   incase it is every closed */
STRPTR ExpandEnvName(CONST_STRPTR env_path)
{
    BOOL     ok = FALSE;
    char     tmp_envbuff[1024];
    STRPTR   fullpath = NULL;
    BPTR     env_lock = (BPTR) NULL;

    env_lock = Lock("ENV:", SHARED_LOCK);
    if (env_lock)
    {
        if (NameFromLock(env_lock, tmp_envbuff, 256)) ok = TRUE;
        UnLock(env_lock);
    }
    
    if (ok)
    {
        if ((fullpath = AllocVec(strlen(tmp_envbuff) + strlen(env_path) + 1 + 1 - 4, MEMF_CLEAR | MEMF_PUBLIC)) != NULL)
        {
            strcpy(fullpath, tmp_envbuff);
            AddPart(fullpath, env_path + 4, 1019);
            return fullpath;
        }     
    }

    // We couldn't expand it so just use as is, but the
    // caller is expecting a newly allocated string...
    fullpath = AllocVec(strlen(env_path) + 1, MEMF_PUBLIC);
    CopyMem(env_path, fullpath, strlen(env_path) + 1);

    return fullpath;
}
///

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
    struct panel_ToolBar_DATA           *panelToolBarPrivate = NULL;

    SETUP_ICONWINDOW_INST_DATA;

    /* Only change dir if it is a valid directory/volume */
    if ((panelToolBarPrivate = (struct panel_ToolBar_DATA *)data->iwd_TopPanel.iwp_PanelPrivate) != NULL)
    {
        if (panelToolBarPrivate->iwp_Node.ln_Name != (char *)extension_Name)
            return;

        GET(panelToolBarPrivate->iwp_ToolBar_LocationStringObj, MUIA_String_Contents, &str);

/* TODO: Signal that it is a wrong path */
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
MakeStaticHook(ToolBar_locationstrHook, panelToolBar__HookFunc_LocationStringFunc);
#endif

#define TOOLBAR_PREFSSIZE     1024
IPTR panelToolBar__HandleFSUpdate(Object *WandererObj, struct NotifyMessage *msg)
{
    if (GetVar(extension_PrefsFile, extension_PrefsData, TOOLBAR_PREFSSIZE, GVF_GLOBAL_ONLY) != -1)
    {
        D(bug("[IW.toolbar] %s: Prefs contain '%s'\n", __PRETTY_FUNCTION__, extension_PrefsData));
        if ((strcasecmp(extension_PrefsData, "True")) == 0)
        {
            SET(panelToolBar__PrefsNotificationObject, MUIA_ShowMe, TRUE);
        }
        else
        {
            SET(panelToolBar__PrefsNotificationObject, MUIA_ShowMe, FALSE);
        }
    }
    return 0;
}

/*** Main Functions ****************************************************************/

///panelToolBar__PrefsSetup()
IPTR panelToolBar__PrefsSetup(Class *CLASS, Object *self, struct opSet *message)
{
    IPTR                                panelToolBarFSNotifyPort = 0;
    struct panel_ToolBar_DATA           *panelToolBarPrivate = NULL;
    struct List                         *panelToolBarFSNotifyList = NULL;

    SETUP_ICONWINDOW_INST_DATA;

    panelToolBarFSNotifyPort = GetTagData(MUIA_Wanderer_FileSysNotifyPort, (IPTR) NULL, message->ops_AttrList);
    panelToolBarFSNotifyList = (APTR)GetTagData(MUIA_Wanderer_FileSysNotifyList, (IPTR) NULL, message->ops_AttrList);

    D(bug("[IW.toolbar]: %s()\n", __PRETTY_FUNCTION__));

    if ((panelToolBarPrivate = (struct panel_ToolBar_DATA *)data->iwd_TopPanel.iwp_PanelPrivate) != NULL)
    {
        if (panelToolBarPrivate->iwp_Node.ln_Name != (char *)extension_Name)
            return 0;

// FIXME: this is never freed
        extension_PrefsData = AllocVec(TOOLBAR_PREFSSIZE, MEMF_CLEAR);
        if (extension_PrefsData == NULL)
            return 0;

        /* Setup notification on prefs file --------------------------------*/
        struct Wanderer_FSHandler *_prefsNotifyHandler = NULL;

        if ((_prefsNotifyHandler = AllocMem(sizeof(struct Wanderer_FSHandler), MEMF_CLEAR)) != NULL)
        {
            _prefsNotifyHandler->fshn_Node.ln_Name                      = ExpandEnvName(extension_PrefsFile);
            panelToolBar__PrefsNotifyRequest.nr_Name                    = _prefsNotifyHandler->fshn_Node.ln_Name;
            panelToolBar__PrefsNotifyRequest.nr_Flags                   = NRF_SEND_MESSAGE;
            panelToolBar__PrefsNotifyRequest.nr_stuff.nr_Msg.nr_Port    = (struct MsgPort *)panelToolBarFSNotifyPort;
            _prefsNotifyHandler->HandleFSUpdate                         = panelToolBar__HandleFSUpdate;

            if (StartNotify(&panelToolBar__PrefsNotifyRequest))
            {
                D(bug("[IW.toolbar]%s: Prefs-notification setup on '%s'\n", __PRETTY_FUNCTION__, panelToolBar__PrefsNotifyRequest.nr_Name));
            }
            else
            {
                D(bug("[IW.toolbar] %s: FAILED to setup Prefs-notification!\n", __PRETTY_FUNCTION__));
            }
            AddTail(panelToolBarFSNotifyList, &_prefsNotifyHandler->fshn_Node);
        }
#ifdef __AROS__
        panelToolBar__PrefsNotificationObject = (Object *)NotifyObject, End;
#else
        panelToolBar__PrefsNotificationObject = MUI_NewObject(MUIC_Notify, TAG_DONE);
#endif
    if (GetVar(extension_PrefsFile, extension_PrefsData, TOOLBAR_PREFSSIZE, GVF_GLOBAL_ONLY) != -1)
    {
            D(bug("[IW.toolbar] %s: Prefs contain '%s'\n", __PRETTY_FUNCTION__, extension_PrefsData));
        }
    }
    return 0;
}
///

///panelToolBar__Setup()
IPTR panelToolBar__Setup(Class *CLASS, Object *self, struct opSet *message)
{
    SETUP_ICONWINDOW_INST_DATA;

    Object                              *panel_ToolBar;
    Object                              *panelToolBar_ButtonDirUp = NULL,
                                        *panelToolBar_ButtonSearch = NULL,
                                        *panelToolBar_StringLocation = NULL;

    struct panel_ToolBar_DATA           *panelToolBarPrivate = NULL;

    if (!(!(data->iwd_Flags & IWDFLAG_ISROOT) && data->iwd_TopPanel.iwp_PanelContainerObj))
        return (IPTR)NULL;

#if !defined(ICONWINDOW_OPTION_NOSEARCHBUTTON)
    panelToolBar_ButtonSearch = ImageButton("", "THEME:Images/Gadgets/Search");
#endif

    D(bug("[IW.toolbar]: %s()\n", __PRETTY_FUNCTION__));

    if (data->iwd_TopPanel.iwp_PanelPrivate == (IPTR)NULL)
    {
        if ((data->iwd_TopPanel.iwp_PanelPrivate = (IPTR)AllocVec(sizeof(struct panel_ToolBar_DATA), MEMF_CLEAR)) == (IPTR)NULL)
            return 0;

        panelToolBarPrivate = (struct panel_ToolBar_DATA *)data->iwd_TopPanel.iwp_PanelPrivate;
        panelToolBarPrivate->iwp_Node.ln_Name = (char *)extension_Name;

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
                    Child, (IPTR) (panelToolBar_ButtonDirUp = ImageButton("", "THEME:Images/Gadgets/DirUp")),
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
            D(bug("[IW.toolbar] %s: ToolBar Obj @ 0x%p\n", __PRETTY_FUNCTION__, panel_ToolBar));

            SET(panelToolBar_ButtonDirUp, MUIA_Background, XGET( panel_ToolBar, MUIA_Background ) );
            SET(panelToolBar_ButtonDirUp, MUIA_CycleChain, 1);
            SET(panelToolBar_ButtonDirUp, MUIA_Frame, MUIV_Frame_None );
#if !defined(ICONWINDOW_OPTION_NOSEARCHBUTTON)
            SET(panelToolBar_ButtonSearch, MUIA_Background, XGET( panel_ToolBar, MUIA_Background ) );
            SET(panelToolBar_ButtonSearch, MUIA_CycleChain, 1);
            SET(panelToolBar_ButtonSearch, MUIA_Frame, MUIV_Frame_None );
#endif
            panelToolBarPrivate->iwp_ToolBar_ToolBarObj = panel_ToolBar;

            if (DoMethod(data->iwd_TopPanel.iwp_PanelGroupObj, MUIM_Group_InitChange ))
            {
                DoMethod(data->iwd_TopPanel.iwp_PanelGroupObj, OM_ADDMEMBER, (IPTR)panel_ToolBar);
                if (data->iwd_TopPanel.iwp_PanelGroupSpacerObj)
                {
                    DoMethod(data->iwd_TopPanel.iwp_PanelGroupObj, OM_REMMEMBER, (IPTR)data->iwd_TopPanel.iwp_PanelGroupSpacerObj);
                    data->iwd_TopPanel.iwp_PanelGroupSpacerObj = NULL;
                }

                DoMethod(data->iwd_TopPanel.iwp_PanelGroupObj, MUIM_Group_ExitChange);
            }

            if (panelToolBarPrivate->iwp_ToolBar_ToolBarObj)
            {
                DoMethod( 
                    panelToolBar_ButtonDirUp, MUIM_Notify, MUIA_Pressed, FALSE, 
                    (IPTR)self, 1, MUIM_IconWindow_DirectoryUp
                  );

                panelToolBarPrivate->iwp_ToolBar_LocationStringObj = panelToolBar_StringLocation;
#ifdef __AROS__
                panelToolBarPrivate->iwp_ToolBar_LocationStrHook.h_Entry = ( HOOKFUNC )panelToolBar__HookFunc_LocationStringFunc;
#else
                panelToolBarPrivate->iwp_ToolBar_LocationStrHook= &ToolBar_locationstrHook;
#endif

                NNSET(
                    panelToolBarPrivate->iwp_ToolBar_LocationStringObj, MUIA_String_Contents, 
                    XGET(data->iwd_IconListObj, MUIA_IconDrawerList_Drawer)
                  );

                /* Make changes to string contents change dir on enter */
                DoMethod ( 
                    panelToolBarPrivate->iwp_ToolBar_LocationStringObj, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, 
                    (IPTR)self, 3, MUIM_CallHook, &panelToolBarPrivate->iwp_ToolBar_LocationStrHook, (IPTR)CLASS
                  );

                if (!(panelToolBar__PrefsNotificationObject))
                    panelToolBar__PrefsSetup(CLASS, self, message);

                DoMethod
                (
                    panelToolBar__PrefsNotificationObject, MUIM_Notify, MUIA_ShowMe, MUIV_EveryTime, 
                    (IPTR)data->iwd_TopPanel.iwp_PanelContainerObj, 3, MUIM_Set, MUIA_ShowMe, MUIV_TriggerValue
                  );

                if ((strcasecmp(extension_PrefsData, "True")) == 0)
                {
                    SET(data->iwd_TopPanel.iwp_PanelContainerObj, MUIA_ShowMe, TRUE);
                }

                AddTail(&panelToolBar__ToolBars, &panelToolBarPrivate->iwp_Node);
            }
        }
        else
        {
            panelToolBarPrivate->iwp_ToolBar_ToolBarObj = NULL;
        }
    }
    return 0;
}
///

IPTR panelToolBar__Cleanup(Class *CLASS, Object *self, Msg msg)
{
    SETUP_ICONWINDOW_INST_DATA;

    struct panel_ToolBar_DATA *panelToolBarPrivate;

    D(bug("[IW.toolbar]: %s()\n", __PRETTY_FUNCTION__));

    if ((panelToolBarPrivate = (struct panel_ToolBar_DATA *)data->iwd_TopPanel.iwp_PanelPrivate) != NULL)
    {
        if (panelToolBarPrivate->iwp_Node.ln_Name != (char *)extension_Name)
            return 0;
        
        if (panelToolBar__PrefsNotificationObject)
        {
            DoMethod
              (
                panelToolBar__PrefsNotificationObject, MUIM_KillNotifyObj, MUIA_ShowMe, 
                (IPTR)data->iwd_TopPanel.iwp_PanelContainerObj
              );
        }
        if (panelToolBarPrivate->iwp_ToolBar_LocationStringObj)
        {
            DoMethod
              (
                panelToolBarPrivate->iwp_ToolBar_LocationStringObj, MUIM_KillNotifyObj, MUIA_String_Acknowledge, 
                (IPTR)self
              );
        }
    }
    return 0;
}

///OM_GET()
IPTR panelToolBar__OM_GET(Class *CLASS, Object *self, struct opGet *message)
{
    SETUP_ICONWINDOW_INST_DATA;

    struct panel_ToolBar_DATA   *panelToolBarPrivate = NULL;
    // IPTR                        *store = message->opg_Storage;
    IPTR                        rv    = FALSE;

    D(bug("[IW.toolbar]: %s()\n", __PRETTY_FUNCTION__));

    if ((panelToolBarPrivate = (struct panel_ToolBar_DATA *)data->iwd_TopPanel.iwp_PanelPrivate) != NULL)
    {
        if (panelToolBarPrivate->iwp_Node.ln_Name != (char *)extension_Name)
            return rv;

        switch (message->opg_AttrID)
        {
        }
    }

    return rv;
}

///OM_SET()
IPTR panelToolBar__OM_SET(Class *CLASS, Object *self, struct opSet *message)
{
    SETUP_ICONWINDOW_INST_DATA;

    struct panel_ToolBar_DATA   *panelToolBarPrivate = NULL;
    struct TagItem              *tstate = message->ops_AttrList, *tag;
    // BOOL                        UpdateIconlist = FALSE;
    // IPTR                        focusicon = (IPTR) NULL;
    IPTR                        rv = FALSE;

    D(bug("[IW.toolbar]: %s()\n", __PRETTY_FUNCTION__));

    if ((panelToolBarPrivate = (struct panel_ToolBar_DATA *)data->iwd_TopPanel.iwp_PanelPrivate) != NULL)
    {
        if (panelToolBarPrivate->iwp_Node.ln_Name != (char *)extension_Name)
            return rv;

        while ((tag = NextTagItem((TAGITEM)&tstate)) != NULL)
        {
            switch (tag->ti_Tag)
            {
            case MUIA_IconWindow_Location:
                D(bug("[IW.toolbar] %s: MUIA_IconWindow_Location '%s'\n", __PRETTY_FUNCTION__, data->iwd_DirectoryPath));

                SET(panelToolBarPrivate->iwp_ToolBar_LocationStringObj, MUIA_String_Contents, (IPTR)data->iwd_DirectoryPath);

                break;
            }
        }
    }

    return rv;
}

#define PANELTOOLBAR_PRIORITY 10

IPTR panelToolBar__Init()
{
    D(bug("[IW.toolbar]: %s()\n", __PRETTY_FUNCTION__));

    panelToolBar__Extension.iwe_Node.ln_Pri = PANELTOOLBAR_PRIORITY;
    panelToolBar__Extension.iwe_Node.ln_Name = (char *)extension_Name;
    panelToolBar__Extension.iwe_Setup = panelToolBar__Setup;
    panelToolBar__Extension.iwe_Cleanup = panelToolBar__Cleanup;
    panelToolBar__Extension.iwe_Set = panelToolBar__OM_SET;
    panelToolBar__Extension.iwe_Get = panelToolBar__OM_GET;

    NewList(&panelToolBar__ToolBars);

    Enqueue(&iconwindow_Extensions, (struct Node *)&panelToolBar__Extension);

    D(bug("[IconWindow] %s: Added Extension '%s' @ %p to list @ %p\n", __PRETTY_FUNCTION__, panelToolBar__Extension.iwe_Node.ln_Name, &panelToolBar__Extension.iwe_Node, &iconwindow_Extensions));

    return TRUE;
}

ADD2INIT(panelToolBar__Init, PANELTOOLBAR_PRIORITY);
