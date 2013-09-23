/*
  Copyright  2004-2013, The AROS Development Team. All rights reserved.
  $Id$
*/

#include "portable_macros.h"
#ifdef __AROS__
#define MUIMASTER_YES_INLINE_STDARG
#endif

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

static CONST_STRPTR                     extension_Name = "IconWindow StatusBar Extension";
static CONST_STRPTR                     extension_PrefsFile = "ENV:SYS/Wanderer/statusbar.prefs";
static STRPTR                           extension_PrefsData;
static struct iconWindow_Extension      panelStatusBar__Extension;
static struct List                      panelStatusBar__StatusBars;
static struct NotifyRequest             panelStatusBar__PrefsNotifyRequest;
static Object                           *panelStatusBar__PrefsNotificationObject;

struct panel_StatusBar_DATA
{
    struct Node                         iwp_Node;
    IPTR                                iwp_Flags;
    Object                              *iwp_StatusBar_StatusBarObj;
    Object                              *iwp_StatusBar_StatusTextObj;
#ifdef __AROS__
    struct Hook                         iwp_StatusBar_updateHook;
#else
    struct Hook                         *iwp_StatusBar_updateHook;
#endif
};

/// From panel_toolbar
STRPTR ExpandEnvName(CONST_STRPTR env_path);
///

const UBYTE MSG_MEM_G[] = "GB";
const UBYTE MSG_MEM_M[] = "MB";
const UBYTE MSG_MEM_K[] = "KB";
const UBYTE MSG_MEM_B[] = "Bytes";

///FmtSizeToString()
static void FmtSizeToString(UBYTE *buf, ULONG num)
{
  UQUAD d;
  const UBYTE *ch;
  struct
  {
    IPTR val;
    IPTR  dec;
  } array =
  {
    num,
    0
  };

  if (num >= 1073741824)
  {
    //Gigabytes
    array.val = num >> 30;
    d = ((UQUAD)num * 10 + 536870912) / 1073741824;
    array.dec = d % 10;
    ch = MSG_MEM_G;
  }
  else if (num >= 1048576)
  {
    //Megabytes
    array.val = num >> 20;
    d = ((UQUAD)num * 10 + 524288) / 1048576;
    array.dec = d % 10;
    ch = MSG_MEM_M;
  }
  else if (num >= 1024)
  {
    //Kilobytes
    array.val = num >> 10;
    d = (num * 10 + 512) / 1024;
    array.dec = d % 10;
    ch = MSG_MEM_K;
  }
  else
  {
    //Bytes
    array.val = num;
    array.dec = 0;
    d = 0;
    ch = MSG_MEM_B;
  }

  if (!array.dec && (d > array.val * 10))
  {
    array.val++;
  }

  RawDoFmt(array.dec ? "%lu.%lu" : "%lu", &array, NULL, buf);

  while (*buf)
  {
    buf++;
  }

  sprintf(buf, " %s", ch);
}
///

/*** Hook functions *********************************************************/
static
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
    struct panel_StatusBar_DATA         *panelStatusBarPrivate = NULL;

    SETUP_ICONWINDOW_INST_DATA;

    D(bug("[IW.statusbar]: %s()\n", __PRETTY_FUNCTION__));

    /* Only change dir if it is a valid directory/volume */
    if ((panelStatusBarPrivate = (struct panel_StatusBar_DATA *)data->iwd_BottomPanel.iwp_PanelPrivate) != NULL)
    {
        if (panelStatusBarPrivate->iwp_Node.ln_Name != (char *)extension_Name)
            return;

        struct List *iconList = NULL;
        struct IconEntry    *icon = NULL;
        UBYTE status_str[1024];
        UBYTE size_str[256];

        int files = 0, dirs = 0, hidden = 0;
        ULONG size = 0;

        GET(data->iwd_IconListObj, MUIA_Family_List, &iconList);
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
                        if (icon->ie_FileInfoBlock)
                        {
                            size += icon->ie_FileInfoBlock->fib_Size;
D(bug("[IW.statusbar] %s: '%s' FIB Size = %d bytes\n", __PRETTY_FUNCTION__, icon->ie_IconNode.ln_Name, icon->ie_FileInfoBlock->fib_Size));
                        }
                    }
                }
                else
                {
                    hidden += 1;
                }
            }
        }
        int previous = 0;
        if (files  > 0)
        {
            FmtSizeToString(size_str, size);
            sprintf(status_str, " %s in %d files", size_str, files);
            previous = strlen(status_str);
        }
        if (dirs > 0)
        {
            sprintf(status_str + previous, "%s%d drawers", (previous > 0) ? ", " : " " , dirs);
            previous = strlen(status_str);
        }
        if (hidden > 0)
        {
            sprintf(status_str + previous, " (%d hidden)", hidden);
            previous = strlen(status_str);
        }

        if (previous > 0)
        {
            SET(panelStatusBarPrivate->iwp_StatusBar_StatusTextObj, MUIA_Text_Contents, (IPTR)status_str);
        }
        else
        {
            SET(panelStatusBarPrivate->iwp_StatusBar_StatusTextObj, MUIA_Text_Contents, (IPTR)"");
        }
    }

    AROS_USERFUNC_EXIT
}
#ifndef __AROS__
MakeStaticHook(StatusBar_updateHook, panelStatusBar__HookFunc_UpdateStatusFunc);
#endif

/*** Main Functions ****************************************************************/

#define STATUSBAR_PREFSSIZE     1024
static IPTR panelStatusBar__HandleFSUpdate(Object *WandererObj, struct NotifyMessage *msg)
{
    if (GetVar(extension_PrefsFile, extension_PrefsData, STATUSBAR_PREFSSIZE, GVF_GLOBAL_ONLY) != -1)
    {
        D(bug("[IW.statusbar] %s: Prefs contain '%s'\n", __PRETTY_FUNCTION__, extension_PrefsData));
        if ((strcasecmp(extension_PrefsData, "True")) == 0)
        {
            SET(panelStatusBar__PrefsNotificationObject, MUIA_ShowMe, TRUE);
        }
        else
        {
            SET(panelStatusBar__PrefsNotificationObject, MUIA_ShowMe, FALSE);
        }
    }
    return 0;
}

///panelStatusBar__PrefsSetup()
static IPTR panelStatusBar__PrefsSetup(Class *CLASS, Object *self, struct opSet *message)
{
    IPTR                                panelStatusBarFSNotifyPort = 0;
    struct panel_StatusBar_DATA         *panelStatusBarPrivate = NULL;
    struct List                         *panelStatusBarFSNotifyList = NULL;

    SETUP_ICONWINDOW_INST_DATA;

    panelStatusBarFSNotifyPort = GetTagData(MUIA_Wanderer_FileSysNotifyPort, (IPTR) NULL, message->ops_AttrList);
    panelStatusBarFSNotifyList = (APTR)GetTagData(MUIA_Wanderer_FileSysNotifyList, (IPTR) NULL, message->ops_AttrList);

    D(bug("[IW.statusbar]: %s()\n", __PRETTY_FUNCTION__));

    if ((panelStatusBarPrivate = (struct panel_StatusBar_DATA *)data->iwd_BottomPanel.iwp_PanelPrivate) != NULL)
    {
        if (panelStatusBarPrivate->iwp_Node.ln_Name != (char *)extension_Name)
            return 0;

// FIXME: this is never freed
        extension_PrefsData = AllocVec(STATUSBAR_PREFSSIZE, MEMF_CLEAR);
        if (extension_PrefsData == NULL)
            return 0;

        /* Setup notification on prefs file --------------------------------*/
        struct Wanderer_FSHandler *_prefsNotifyHandler = NULL;

        if ((_prefsNotifyHandler = AllocMem(sizeof(struct Wanderer_FSHandler), MEMF_CLEAR)) != NULL)
        {
            _prefsNotifyHandler->fshn_Node.ln_Name                     = ExpandEnvName(extension_PrefsFile);
            panelStatusBar__PrefsNotifyRequest.nr_Name                 = _prefsNotifyHandler->fshn_Node.ln_Name;
            panelStatusBar__PrefsNotifyRequest.nr_Flags                = NRF_SEND_MESSAGE;
            panelStatusBar__PrefsNotifyRequest.nr_stuff.nr_Msg.nr_Port = (struct MsgPort *)panelStatusBarFSNotifyPort;
            _prefsNotifyHandler->HandleFSUpdate                        = panelStatusBar__HandleFSUpdate;

            if (StartNotify(&panelStatusBar__PrefsNotifyRequest))
            {
                D(bug("[IW.statusbar]%s: Prefs-notification setup on '%s'\n", __PRETTY_FUNCTION__, panelStatusBar__PrefsNotifyRequest.nr_Name));
            }
            else
            {
                D(bug("[IW.statusbar] %s: FAILED to setup Prefs-notification!\n", __PRETTY_FUNCTION__));
            }
            AddTail(panelStatusBarFSNotifyList, &_prefsNotifyHandler->fshn_Node);
        }
#ifdef __AROS__
        panelStatusBar__PrefsNotificationObject = (Object *)NotifyObject, End;
#else
        panelStatusBar__PrefsNotificationObject = MUI_NewObject(MUIC_Notify, TAG_DONE);
#endif
    if (GetVar(extension_PrefsFile, extension_PrefsData, STATUSBAR_PREFSSIZE, GVF_GLOBAL_ONLY) != -1)
    {
            D(bug("[IW.statusbar] %s: Prefs contain '%s'\n", __PRETTY_FUNCTION__, extension_PrefsData));
        }
    }
    return 0;
}
///

///panelStatusBar__Setup()
static IPTR panelStatusBar__Setup(Class *CLASS, Object *self, struct opSet *message)
{
    SETUP_ICONWINDOW_INST_DATA;

    Object                              *panel_StatusBar = NULL,
                                        *panelStatusBar_TextStatus = NULL;

    struct panel_StatusBar_DATA         *panelStatusBarPrivate = NULL;

    if (!(!(data->iwd_Flags & IWDFLAG_ISROOT) && data->iwd_BottomPanel.iwp_PanelContainerObj))
        return 0;

    D(bug("[IW.statusbar]: %s()\n", __PRETTY_FUNCTION__));

    if (data->iwd_BottomPanel.iwp_PanelPrivate == 0)
    {
        if ((data->iwd_BottomPanel.iwp_PanelPrivate = (IPTR)AllocVec(sizeof(struct panel_StatusBar_DATA), MEMF_CLEAR)) == (IPTR)NULL)
            return 0;

        panelStatusBarPrivate = (struct panel_StatusBar_DATA *)data->iwd_BottomPanel.iwp_PanelPrivate;
        panelStatusBarPrivate->iwp_Node.ln_Name = (char *)extension_Name;

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
                        MUIA_Text_Contents, (IPTR)"",
                    TAG_DONE) ),
                TAG_DONE),
            TAG_DONE),
        TAG_DONE);

        /* Got a StatusBarPanel? setup notifies and other values are 
         copied to the data of the object */
        if ( panel_StatusBar != NULL )
        {
            D(bug("[IW.statusbar] %s: StatusBar Obj @ 0x%p\n", __PRETTY_FUNCTION__, panel_StatusBar));

            panelStatusBarPrivate->iwp_StatusBar_StatusTextObj = panelStatusBar_TextStatus;
            panelStatusBarPrivate->iwp_StatusBar_StatusBarObj = panel_StatusBar;

            if (DoMethod(data->iwd_BottomPanel.iwp_PanelGroupObj, MUIM_Group_InitChange ))
            {
                DoMethod(data->iwd_BottomPanel.iwp_PanelGroupObj, OM_ADDMEMBER, (IPTR)panel_StatusBar);
                if (data->iwd_BottomPanel.iwp_PanelGroupSpacerObj)
                {
                    DoMethod(data->iwd_BottomPanel.iwp_PanelGroupObj, OM_REMMEMBER, (IPTR)data->iwd_BottomPanel.iwp_PanelGroupSpacerObj);
                    data->iwd_BottomPanel.iwp_PanelGroupSpacerObj = NULL;
                }

                DoMethod(data->iwd_BottomPanel.iwp_PanelGroupObj, MUIM_Group_ExitChange);
            }

            if (panelStatusBarPrivate->iwp_StatusBar_StatusBarObj)
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

                if (!(panelStatusBar__PrefsNotificationObject))
                    panelStatusBar__PrefsSetup(CLASS, self, message);

                DoMethod
                  (
                    panelStatusBar__PrefsNotificationObject, MUIM_Notify, MUIA_ShowMe, MUIV_EveryTime, 
                    (IPTR)data->iwd_BottomPanel.iwp_PanelContainerObj, 3, MUIM_Set, MUIA_ShowMe, MUIV_TriggerValue
                  );

                if ((strcasecmp(extension_PrefsData, "True")) == 0)
                {
                    SET(data->iwd_BottomPanel.iwp_PanelContainerObj, MUIA_ShowMe, TRUE);
                }

                AddTail(&panelStatusBar__StatusBars, &panelStatusBarPrivate->iwp_Node);
            }
        }
        else
        {
            panelStatusBarPrivate->iwp_StatusBar_StatusBarObj = NULL;
        }
    }
    return 0;
}
///

///panelStatusBar__Cleanup()
static IPTR panelStatusBar__Cleanup(Class *CLASS, Object *self, Msg msg)
{
    SETUP_ICONWINDOW_INST_DATA;
    struct panel_StatusBar_DATA *panelStatusBarPrivate;

    D(bug("[IW.statusbar]: %s()\n", __PRETTY_FUNCTION__));

    if ((panelStatusBarPrivate = (struct panel_StatusBar_DATA *)data->iwd_TopPanel.iwp_PanelPrivate) != NULL)
    {
        if (panelStatusBarPrivate->iwp_Node.ln_Name != (char *)extension_Name)
            return 0;

        if (panelStatusBar__PrefsNotificationObject)
        {
            DoMethod
              (
                panelStatusBar__PrefsNotificationObject, MUIM_KillNotifyObj, MUIA_ShowMe, 
                (IPTR)data->iwd_BottomPanel.iwp_PanelContainerObj
              );
        }
    }
    return 0;
}
///

///panelStatusBar__OM_GET()
static IPTR panelStatusBar__OM_GET(Class *CLASS, Object *self, struct opGet *message)
{
    SETUP_ICONWINDOW_INST_DATA;

    struct panel_StatusBar_DATA *panelStatusBarPrivate = NULL;
    // IPTR                        *store = message->opg_Storage;
    IPTR                        rv = FALSE;

    D(bug("[IW.statusbar]: %s()\n", __PRETTY_FUNCTION__));

    if ((panelStatusBarPrivate = (struct panel_StatusBar_DATA *)data->iwd_BottomPanel.iwp_PanelPrivate) != NULL)
    {
        if (panelStatusBarPrivate->iwp_Node.ln_Name != (char *)extension_Name)
            return rv;

        switch (message->opg_AttrID)
        {
        }
    }
    return rv;
}
///

///OM_SET()
static IPTR panelStatusBar__OM_SET(Class *CLASS, Object *self, struct opSet *message)
{
    SETUP_ICONWINDOW_INST_DATA;

    struct panel_StatusBar_DATA *panelStatusBarPrivate = NULL;
    struct TagItem              *tstate = message->ops_AttrList, *tag;
    // BOOL                        UpdateIconlist = FALSE;
    // IPTR                        focusicon = (IPTR) NULL;
    IPTR                        rv = FALSE;

    D(bug("[IW.statusbar]: %s()\n", __PRETTY_FUNCTION__));

    if ((panelStatusBarPrivate = (struct panel_StatusBar_DATA *)data->iwd_BottomPanel.iwp_PanelPrivate) != NULL)
    {
        if (panelStatusBarPrivate->iwp_Node.ln_Name != (char *)extension_Name)
            return rv;

        while ((tag = NextTagItem((TAGITEM)&tstate)) != NULL)
        {
            switch (tag->ti_Tag)
            {
            /*case MUIA_IconWindowExt_ToolBar_Enabled:   
                if ((!(data->iwd_Flags & IWDFLAG_ISROOT)) && (data->iwd_BottomPanel.iwp_PanelContainerObj))
                {
                    // remove statusbar
                    if (!(( BOOL )tag->ti_Data))
                    {
                        if (panelStatusBarPrivate->iwp_StatusBar_StatusBarObj != NULL)
                        {
                            data->iwd_BottomPanel.iwp_PanelGroupSpacerObj = HSpace(0);
                
                            SET(data->iwd_BottomPanel.iwp_PanelContainerObj, MUIA_Frame, MUIV_Frame_None);
                            SET(data->iwd_BottomPanel.iwp_PanelContainerObj, MUIA_Group_Spacing, 0);

                            if ((data->iwd_BottomPanel.iwp_PanelGroupSpacerObj) && (DoMethod(data->iwd_BottomPanel.iwp_PanelGroupObj, MUIM_Group_InitChange)))
                            {
                                DoMethod(data->iwd_BottomPanel.iwp_PanelGroupObj, OM_REMMEMBER, (IPTR)panelStatusBarPrivate->iwp_StatusBar_StatusBarObj);
                                DoMethod(data->iwd_BottomPanel.iwp_PanelGroupObj, OM_ADDMEMBER, (IPTR)data->iwd_BottomPanel.iwp_PanelGroupSpacerObj);
                                DoMethod(data->iwd_BottomPanel.iwp_PanelGroupObj, MUIM_Group_ExitChange);
                                panelStatusBarPrivate->iwp_StatusBar_StatusBarObj = NULL;
                            }
                        }
                    }
                    else
                    {
                        // setup statusbar
                        if (panelStatusBarPrivate->iwp_StatusBar_StatusBarObj == NULL)
                        {
                            Object *prefs = NULL;
                            GET(_app(self), MUIA_Wanderer_Prefs, &prefs);
                            panelStatusBar__Setup(CLASS, self, prefs);
                        }
                    }
                }
                break;*/
            }
        }
    }
    return rv;
}

#define PANELSTATUSBAR_PRIORITY 10

IPTR panelStatusBar__Init()
{
    D(bug("[IW.statusbar]: %s()\n", __PRETTY_FUNCTION__));

    panelStatusBar__Extension.iwe_Node.ln_Pri = PANELSTATUSBAR_PRIORITY;
    panelStatusBar__Extension.iwe_Node.ln_Name = (char *)extension_Name;
    panelStatusBar__Extension.iwe_Setup = panelStatusBar__Setup;
    panelStatusBar__Extension.iwe_Cleanup = panelStatusBar__Cleanup;
    panelStatusBar__Extension.iwe_Set = panelStatusBar__OM_SET;
    panelStatusBar__Extension.iwe_Get = panelStatusBar__OM_GET;

    NewList(&panelStatusBar__StatusBars);

    Enqueue(&iconwindow_Extensions, (struct Node *)&panelStatusBar__Extension);

    D(bug("[IconWindow] %s: Added Extension '%s' @ %p to list @ %p\n", __PRETTY_FUNCTION__, panelStatusBar__Extension.iwe_Node.ln_Name, &panelStatusBar__Extension.iwe_Node, &iconwindow_Extensions));

    return TRUE;
}

ADD2INIT(panelStatusBar__Init, PANELSTATUSBAR_PRIORITY);
