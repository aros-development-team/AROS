/*
  Copyright  2004-2011, The AROS Development Team. All rights reserved.
  $Id$
*/

#define ZCC_QUIET

#include "portable_macros.h"

#ifdef __AROS__
#define MUIMASTER_YES_INLINE_STDARG
#endif

#define TXTBUFF_LEN 1024

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
#include <proto/icon.h>

#include <dos/dos.h>
#include <proto/dos.h>

#include <stdio.h>
#include <string.h>

#include <intuition/screens.h>
#include <datatypes/pictureclass.h>
#include <clib/macros.h>

#if defined(__AMIGA__) && !defined(__PPC__)
#define NO_INLINE_STDARG
#endif
#ifndef _PROTO_INTUITION_H
#include <proto/intuition.h>
#endif
#include <proto/muimaster.h>

#include "Classes/iconlist.h"
#include "Classes/iconlist_attributes.h"
#include "Classes/icon_attributes.h"

#include "wanderer.h"
#include "wandererprefs.h"
#include "iconwindow.h"
#include "iconwindow_iconlist.h"


#ifndef __AROS__
#define DEBUG 1

#ifdef DEBUG
  #define D(x) if (DEBUG) x
  #ifdef __amigaos4__
  #define bug DebugPrintF
  #else
  #define bug kprintf
  #endif
#else
  #define  D(...)
#endif
#endif

extern struct IconWindow_BackFill_Descriptor  *iconwindow_BackFill_Active;

#define WIWDLVERS       1
#define WIWDLREV        0

#define BG_DRAWFLAG     0xf00dd00f

/*** Instance Data **********************************************************/

struct IconWindowDrawerList_DATA
{
    Object                      *iwidld_IconWindow;
    struct RastPort             *iwidld_RastPort;
    struct MUI_EventHandlerNode iwidld_EventHandlerNode;
#ifdef __AROS__
    struct Hook                 iwidld_ProcessIconListPrefs_hook;
#else
    struct Hook                 *iwidld_ProcessIconListPrefs_hook;
#endif

    IPTR                        iwidld_ViewPrefs_ID;
    Object                      *iwidld_ViewPrefs_NotificationObject;

    /* File System update handling */
    struct Wanderer_FSHandler   iwidld_FSHandler;
    struct NotifyRequest        iwidld_DrawerNotifyRequest;
    ULONG                       iwidld_LastRefresh; /* In seconds */
    BOOL                        iwidld_FSChanged;
};

// static char __icwc_intern_TxtBuff[TXTBUFF_LEN];

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct IconWindowDrawerList_DATA *data = INST_DATA(CLASS, self)

/*** Hook functions *********************************************************/
///IconWindowDrawerList__HookFunc_ProcessIconListPrefsFunc()
#ifdef __AROS__
AROS_UFH3(
    void, IconWindowDrawerList__HookFunc_ProcessIconListPrefsFunc,
    AROS_UFHA(struct Hook *,    hook,   A0),
    AROS_UFHA(APTR *,           obj,    A2),
    AROS_UFHA(IPTR *,             param,  A1)
)
{
#else
HOOKPROTO(IconWindowDrawerList__HookFunc_ProcessIconListPrefsFunc, void, APTR *obj, IPTR *param)
{
#endif
    AROS_USERFUNC_INIT

    /* Get our private data */
    Object *self = ( Object *)obj;
    IPTR CHANGED_ATTRIB = *param;
    Class *CLASS = OCLASS(self);

    SETUP_INST_DATA;

    Object *prefs = NULL;

    D(bug("[Wanderer:DrawerList]: %s()\n", __PRETTY_FUNCTION__));

    GET(_app(self), MUIA_Wanderer_Prefs, &prefs);

    if (prefs)
    {
        IPTR attrib_Current = 0, attrib_Prefs, prefs_Processing = 0;
        BOOL options_changed = FALSE;

        D(bug("[Wanderer:DrawerList] %s: Setting IconList options ..\n", __PRETTY_FUNCTION__));

        GET(prefs, MUIA_WandererPrefs_Processing, &prefs_Processing);

        switch (CHANGED_ATTRIB)
        {
        case MUIA_IconList_IconListMode:
        case MUIA_IconList_LabelText_Mode:
        case MUIA_IconList_LabelText_MaxLineLen:
        case MUIA_IconList_LabelText_MultiLine:
        case MUIA_IconList_LabelText_MultiLineOnFocus:
        case MUIA_IconList_Icon_HorizontalSpacing:
        case MUIA_IconList_Icon_VerticalSpacing:
        case MUIA_IconList_Icon_ImageSpacing:
        case MUIA_IconList_LabelText_HorizontalPadding:
        case MUIA_IconList_LabelText_VerticalPadding:
        case MUIA_IconList_LabelText_BorderWidth:
        case MUIA_IconList_LabelText_BorderHeight:
        case MUIA_IconList_DragImageTransparent:
        case MUIA_IconList_SortFlags:
            /* Generic code */
            GET(self, (ULONG)CHANGED_ATTRIB, &attrib_Current);

            if (((attrib_Prefs = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwidld_ViewPrefs_ID, (ULONG)CHANGED_ATTRIB)) != -1) &&
                (attrib_Current != attrib_Prefs))
            {
                options_changed = TRUE;
                if (prefs_Processing)
                {
                    NNSET(self, (ULONG)CHANGED_ATTRIB, attrib_Prefs);
                }
                else
                {
                    SET(self, (ULONG)CHANGED_ATTRIB, attrib_Prefs);
                }
            }
            break;

        default:
            D(bug("[Wanderer:DrawerList] %s: Unhandled change\n", __PRETTY_FUNCTION__));
            break;
        }

        if (options_changed)
        {
            if (!(prefs_Processing))
            {
                D(bug("[Wanderer:DrawerList] %s: IconList Options have changed, causing an update ..\n", __PRETTY_FUNCTION__));
                DoMethod(self, MUIM_IconList_Update);
                DoMethod(self, MUIM_IconList_Sort);
            }
            else if (data->iwidld_IconWindow)
            {
                SET(data->iwidld_IconWindow, MUIA_IconWindow_Changed, TRUE);
            }
        }
    }
    AROS_USERFUNC_EXIT
}
#ifndef __AROS__
MakeStaticHook(Hook_ProcessIconListPrefsFunc, IconWindowDrawerList__HookFunc_ProcessIconListPrefsFunc);
#endif
///

///
/*** Methods ****************************************************************/
///OM_NEW()
Object *IconWindowDrawerList__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    IPTR                            _newIconList__FSNotifyPort = 0;

    D(bug("[Wanderer:DrawerList]: %s()\n", __PRETTY_FUNCTION__));

    _newIconList__FSNotifyPort = GetTagData(MUIA_Wanderer_FileSysNotifyPort, (IPTR) NULL, message->ops_AttrList);

    self = (Object *) DoSuperNewTags
      (
        CLASS, self, NULL,
        MUIA_CycleChain, 1,
        TAG_MORE, (IPTR) message->ops_AttrList
      );

    if (self != NULL)
    {
        SETUP_INST_DATA;
        D(bug("[Wanderer:DrawerList] %s: SELF @ 0x%p\n", __PRETTY_FUNCTION__, self));

#ifdef __AROS__
        data->iwidld_ProcessIconListPrefs_hook.h_Entry = ( HOOKFUNC )IconWindowDrawerList__HookFunc_ProcessIconListPrefsFunc;
#else
        data->iwidld_ProcessIconListPrefs_hook = &Hook_ProcessIconListPrefsFunc;
#endif

        if (_newIconList__FSNotifyPort != 0)
        {
            struct IconWindowDrawerList_DATA *drawerlist_data = (struct IconWindowDrawerList_DATA *)data;
            drawerlist_data->iwidld_DrawerNotifyRequest.nr_stuff.nr_Msg.nr_Port = (struct MsgPort *)_newIconList__FSNotifyPort;
            D(bug("[Wanderer:DrawerList] %s: FS Notify Port @ 0x%p\n", __PRETTY_FUNCTION__, _newIconList__FSNotifyPort));
        }
    }

    return self;
}
///

IPTR IconWindowDrawerList__HandleFSUpdate(Object * iconwindow, struct NotifyMessage *msg)
{
    return DoMethod(iconwindow, MUIM_IconWindowDrawerList_FileSystemChanged);
}

ULONG GetCurrentTimeInSeconds()
{
    struct DateStamp stamp;

    DateStamp(&stamp);

    return (stamp.ds_Days * 60 * 60 * 24) + (stamp.ds_Minute * 60) + (stamp.ds_Tick / 50);
}

VOID RemoveFSNotification(struct IconWindowDrawerList_DATA *data)
{
    if (data->iwidld_DrawerNotifyRequest.nr_Name != NULL)
    {
        D(bug("[Wanderer:DrawerList] %s: Removing  Drawer FS Notification Request\n", __PRETTY_FUNCTION__));
        /* EndNotify also replies all already send messages, so they won't end up hitting dead space in handler->HandleFSUpdate */
        EndNotify(&data->iwidld_DrawerNotifyRequest);
        FreeVec(data->iwidld_DrawerNotifyRequest.nr_Name);
        data->iwidld_DrawerNotifyRequest.nr_Name = NULL;
        data->iwidld_FSHandler.fshn_Node.ln_Name = NULL;
    }
}

VOID UpdateFSNotification(STRPTR directory_path, struct IconWindowDrawerList_DATA *data, APTR target)
{
    /* Remove existing if present */
    RemoveFSNotification(data);

    /* Setup new */
    if (directory_path != NULL)
    {
        if (data->iwidld_DrawerNotifyRequest.nr_stuff.nr_Msg.nr_Port != NULL)
        {
            data->iwidld_FSHandler.target                   = target;
            data->iwidld_FSHandler.HandleFSUpdate           = IconWindowDrawerList__HandleFSUpdate;
            data->iwidld_DrawerNotifyRequest.nr_Name        = StrDup(directory_path);
            data->iwidld_DrawerNotifyRequest.nr_Flags       = NRF_SEND_MESSAGE;
            data->iwidld_DrawerNotifyRequest.nr_UserData    = (IPTR)&data->iwidld_FSHandler;
            data->iwidld_LastRefresh                        = GetCurrentTimeInSeconds();
            data->iwidld_FSChanged                          = FALSE;
            data->iwidld_FSHandler.fshn_Node.ln_Name        = data->iwidld_DrawerNotifyRequest.nr_Name;

            if (StartNotify(&data->iwidld_DrawerNotifyRequest))
            {
                D(bug("[Wanderer:DrawerList] %s: Drawer-notification setup on '%s'\n", __PRETTY_FUNCTION__, directory_path));
            }
            else
            {
                D(bug("[Wanderer:DrawerList] %s: FAILED to setup Drawer-notification on '%s'!\n", __PRETTY_FUNCTION__, directory_path));
                FreeVec(data->iwidld_DrawerNotifyRequest.nr_Name);
                data->iwidld_DrawerNotifyRequest.nr_Name = NULL;
                data->iwidld_FSHandler.fshn_Node.ln_Name = NULL;
            }
        }
    }
}

///OM_SET()
IPTR IconWindowDrawerList__OM_SET(Class *CLASS, Object *self, struct opSet *message)
{
    SETUP_INST_DATA;

    struct TagItem *tstate = message->ops_AttrList, *tag;

    while ((tag = NextTagItem((const struct TagItem**)&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Background:
            {
                D(bug("[Wanderer:DrawerList] %s: MUIA_Background\n", __PRETTY_FUNCTION__));
                break;
            }
        case MUIA_IconWindow_Window:
            {
                D(bug("[Wanderer:DrawerList] %s: MUIA_IconWindow_Window @ %p\n", __PRETTY_FUNCTION__, tag->ti_Data));
                data->iwidld_IconWindow = (Object *)tag->ti_Data;
                break;
            }
        case MUIA_IconList_BufferRastport:
            {
                D(bug("[Wanderer:DrawerList] %s: MUIA_IconList_BufferRastport @ %p\n", __PRETTY_FUNCTION__, tag->ti_Data));
                data->iwidld_RastPort = (struct RastPort *)tag->ti_Data;
                break;
            }
        case MUIA_IconDrawerList_Drawer:
            {
                D(bug("[Wanderer:DrawerList] %s: MUIA_IconDrawerList_Drawer @ %p\n", __PRETTY_FUNCTION__, tag->ti_Data));
                UpdateFSNotification((STRPTR)tag->ti_Data, data, self);
                break; /* Fallthrough and handle this in parent class as well */
            }
        }
    }
    return DoSuperMethodA(CLASS, self, (Msg) message);
}
///

///OM_GET()
IPTR IconWindowDrawerList__OM_GET(Class *CLASS, Object *self, struct opGet *message)
{
    //SETUP_INST_DATA;
    IPTR *store = message->opg_Storage;
    IPTR  rv    = TRUE;

    switch (message->opg_AttrID)
    {
    case MUIA_Version:
        *store = (IPTR)WIWDLVERS;
        break;

    case MUIA_Revision:
        *store = (IPTR)WIWDLREV;
        break;

    default:
        rv = DoSuperMethodA(CLASS, self, (Msg) message);
    }

    return rv;
}
///

#define SETFROMPREFS(tag)                                                                                           \
    attrib_Prefs = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwidld_ViewPrefs_ID, tag);   \
    if ((attrib_Prefs != (IPTR)-1)  && (attrib_Prefs != XGET(self, tag))) SET(self, tag, attrib_Prefs);

#define ADDPREFSNTF(tag)                                                                                            \
    DoMethod(data->iwidld_ViewPrefs_NotificationObject, MUIM_Notify, tag, MUIV_EveryTime, (IPTR) self, 3,           \
             MUIM_CallHook, &data->iwidld_ProcessIconListPrefs_hook, (IPTR)tag);

#define REMPREFSNTF(tag)                                                                                            \
    DoMethod(data->iwidld_ViewPrefs_NotificationObject, MUIM_KillNotifyObj, tag, (IPTR)self);

///IconWindowDrawerList__MUIM_Setup()
IPTR IconWindowDrawerList__MUIM_Setup
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
  
    Object *prefs = NULL;

    if (!DoSuperMethodA(CLASS, self, message)) return FALSE;

    GET(_app(self), MUIA_Wanderer_Prefs, &prefs);

    if (prefs)
    {
        /* Set our initial options */
        IPTR    attrib_Prefs;

        GET(_win(self), MUIA_IconWindow_BackgroundAttrib, &data->iwidld_ViewPrefs_ID);
        D(bug("[Wanderer:DrawerList] %s: Window Background = '%s'\n", __PRETTY_FUNCTION__, data->iwidld_ViewPrefs_ID));
        data->iwidld_ViewPrefs_NotificationObject = (Object *)DoMethod(prefs,
                                MUIM_WandererPrefs_ViewSettings_GetNotifyObject,
                                data->iwidld_ViewPrefs_ID);

        D(bug("[Wanderer:DrawerList] %s: Background Notification Obj @ 0x%p\n", __PRETTY_FUNCTION__, data->iwidld_ViewPrefs_NotificationObject));

        SETFROMPREFS(MUIA_IconList_IconListMode);
        SETFROMPREFS(MUIA_IconList_LabelText_Mode);
        SETFROMPREFS(MUIA_IconList_SortFlags);
        SETFROMPREFS(MUIA_IconList_DragImageTransparent);
        SETFROMPREFS(MUIA_IconList_LabelText_MaxLineLen);
        SETFROMPREFS(MUIA_IconList_LabelText_MultiLine);
        SETFROMPREFS(MUIA_IconList_LabelText_MultiLineOnFocus);
        SETFROMPREFS(MUIA_IconList_Icon_HorizontalSpacing);
        SETFROMPREFS(MUIA_IconList_Icon_VerticalSpacing);
        SETFROMPREFS(MUIA_IconList_Icon_ImageSpacing);
        SETFROMPREFS(MUIA_IconList_LabelText_HorizontalPadding);
        SETFROMPREFS(MUIA_IconList_LabelText_VerticalPadding);
        SETFROMPREFS(MUIA_IconList_LabelText_BorderWidth);
        SETFROMPREFS(MUIA_IconList_LabelText_BorderHeight);

        /* Configure notifications incase they get updated =) */
        ADDPREFSNTF(MUIA_IconList_IconListMode);
        ADDPREFSNTF(MUIA_IconList_LabelText_Mode);
        ADDPREFSNTF(MUIA_IconList_SortFlags);
        ADDPREFSNTF(MUIA_IconList_DragImageTransparent);
        ADDPREFSNTF(MUIA_IconList_LabelText_MaxLineLen);
        ADDPREFSNTF(MUIA_IconList_LabelText_MultiLine);
        ADDPREFSNTF(MUIA_IconList_LabelText_MultiLineOnFocus);
        ADDPREFSNTF(MUIA_IconList_Icon_HorizontalSpacing);
        ADDPREFSNTF(MUIA_IconList_Icon_VerticalSpacing);
        ADDPREFSNTF(MUIA_IconList_Icon_ImageSpacing);
        ADDPREFSNTF(MUIA_IconList_LabelText_HorizontalPadding);
        ADDPREFSNTF(MUIA_IconList_LabelText_VerticalPadding);
        ADDPREFSNTF(MUIA_IconList_LabelText_BorderWidth);
        ADDPREFSNTF(MUIA_IconList_LabelText_BorderHeight);
    }

    /* Setup notification on the directory -------------------------------- */
    STRPTR directory_path = NULL;
    GET(self, MUIA_IconDrawerList_Drawer, &directory_path);

    UpdateFSNotification(directory_path, data, self);

    D(bug("[Wanderer:DrawerList] %s: Setup complete!\n", __PRETTY_FUNCTION__));
  
    return TRUE;
}
///

///IconWindowDrawerList__MUIM_Cleanup()
IPTR IconWindowDrawerList__MUIM_Cleanup
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;

    Object *prefs = NULL;

    D(bug("[Wanderer:DrawerList]: %s()\n", __PRETTY_FUNCTION__));

    GET(_app(self), MUIA_Wanderer_Prefs, &prefs);

    if (prefs)
    {
        REMPREFSNTF(MUIA_IconList_IconListMode);
        REMPREFSNTF(MUIA_IconList_LabelText_Mode);
        REMPREFSNTF(MUIA_IconList_SortFlags);
        REMPREFSNTF(MUIA_IconList_DragImageTransparent);
        REMPREFSNTF(MUIA_IconList_LabelText_MaxLineLen);
        REMPREFSNTF(MUIA_IconList_LabelText_MultiLine);
        REMPREFSNTF(MUIA_IconList_LabelText_MultiLineOnFocus);
        REMPREFSNTF(MUIA_IconList_Icon_HorizontalSpacing);
        REMPREFSNTF(MUIA_IconList_Icon_VerticalSpacing);
        REMPREFSNTF(MUIA_IconList_Icon_ImageSpacing);
        REMPREFSNTF(MUIA_IconList_LabelText_HorizontalPadding);
        REMPREFSNTF(MUIA_IconList_LabelText_VerticalPadding);
        REMPREFSNTF(MUIA_IconList_LabelText_BorderWidth);
        REMPREFSNTF(MUIA_IconList_LabelText_BorderHeight);
    }

    RemoveFSNotification(data);

    return DoSuperMethodA(CLASS, self, message);
}
///

///IconWindowDrawerList__MUIM_DrawBackground()
IPTR IconWindowDrawerList__MUIM_DrawBackground
(
  Class *CLASS, Object *self, struct MUIP_DrawBackground *message
)
{
    SETUP_INST_DATA;

    IPTR        retVal = (IPTR)TRUE;
    IPTR                clip = 0;
    struct RastPort           *DrawBackGround_RastPort;
    struct IconWindowBackFillMsg  DrawBackGround_BackFillMsg;

    D(bug("[Wanderer:DrawerList]: %s()\n", __PRETTY_FUNCTION__));

    if ((iconwindow_BackFill_Active == NULL) ||
      (data->iwidld_IconWindow == NULL))
    {
        D(bug("[Wanderer:DrawerList] %s: No Backfill support/Window not set .. causing parent class to render\n", __PRETTY_FUNCTION__));
        goto iwc_ParentBackground;
    }

    DrawBackGround_RastPort = _rp(self);

    if ((data->iwidld_RastPort != NULL) && (DrawBackGround_RastPort != data->iwidld_RastPort))
    {
        DrawBackGround_RastPort = data->iwidld_RastPort;

        DrawBackGround_BackFillMsg.AreaBounds.MinX = 0;
        DrawBackGround_BackFillMsg.AreaBounds.MinY = 0;
        DrawBackGround_BackFillMsg.AreaBounds.MaxX = _mwidth(self) - 1;
        DrawBackGround_BackFillMsg.AreaBounds.MaxY = _mheight(self) - 1;

        DrawBackGround_BackFillMsg.DrawBounds.MinX = message->left - _mleft(self);
        DrawBackGround_BackFillMsg.DrawBounds.MinY = message->top - _mtop(self);
        DrawBackGround_BackFillMsg.DrawBounds.MaxX = DrawBackGround_BackFillMsg.DrawBounds.MinX + message->width - 1;
        DrawBackGround_BackFillMsg.DrawBounds.MaxY = DrawBackGround_BackFillMsg.DrawBounds.MinY + message->height - 1;
    }
    else
    {
        DrawBackGround_BackFillMsg.AreaBounds.MinX = _mleft(self);
        DrawBackGround_BackFillMsg.AreaBounds.MinY = _mtop(self);
        DrawBackGround_BackFillMsg.AreaBounds.MaxX = _mleft(self) + _mwidth(self) - 1;
        DrawBackGround_BackFillMsg.AreaBounds.MaxY = _mtop(self) + _mheight(self) - 1;

        DrawBackGround_BackFillMsg.DrawBounds.MinX = message->left;
        DrawBackGround_BackFillMsg.DrawBounds.MinY = message->top;
        DrawBackGround_BackFillMsg.DrawBounds.MaxX = message->left + message->width - 1;
        DrawBackGround_BackFillMsg.DrawBounds.MaxY = message->top + message->height - 1;
    }

    DrawBackGround_BackFillMsg.Layer = DrawBackGround_RastPort->Layer;

    /* Offset into source image (ala scroll bar position) */
    DrawBackGround_BackFillMsg.OffsetX = message->xoffset;
    DrawBackGround_BackFillMsg.OffsetY = message->yoffset;

    D(bug("[Wanderer:DrawerList] %s: RastPort @ 0x%p\n", __PRETTY_FUNCTION__, DrawBackGround_RastPort));
  
    if ((retVal = DoMethod(data->iwidld_IconWindow, MUIM_IconWindow_BackFill_DrawBackground, XGET(data->iwidld_IconWindow, MUIA_IconWindow_BackFillData), &DrawBackGround_BackFillMsg, DrawBackGround_RastPort)) == (IPTR)TRUE)
    {
        D(bug("[Wanderer:DrawerList] %s: Backfill module rendered background ..\n", __PRETTY_FUNCTION__));
        return retVal;
    }
    D(bug("[Wanderer:DrawerList] %s: Backfill module failed to render background ..\n", __PRETTY_FUNCTION__));

iwc_ParentBackground:

    clip = (IPTR)MUI_AddClipping(muiRenderInfo(self), message->left, message->top, message->width, message->height);

    message->width = _mwidth(self);
    message->height = _mheight(self);
    message->left = _mleft(self);
    message->top = _mtop(self);

    retVal = DoSuperMethodA(CLASS, self, (Msg) message);

    MUI_RemoveClipping(muiRenderInfo(self), (APTR)clip);

    return retVal;
}
///

/// IconWindowDrawerList__MUIM_IconWindowDrawerList_FileSystemChanged
IPTR IconWindowDrawerList__MUIM_IconWindowDrawerList_FileSystemChanged
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;

    data->iwidld_FSChanged = TRUE;

    return DoMethod(self, MUIM_IconWindowIconList_RateLimitRefresh );
}
///

/// IconWindowDrawerList__MUIM_IconWindowIconList_RateLimitRefresh
IPTR IconWindowDrawerList__MUIM_IconWindowIconList_RateLimitRefresh
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    ULONG current = 0;

    if (!data->iwidld_FSChanged)
        return (IPTR)FALSE;

    current = GetCurrentTimeInSeconds();

    if (data->iwidld_LastRefresh <= current - 3) /* At most every 3 seconds */
    {
        DoMethod(self, MUIM_IconList_Update);
        DoMethod(self, MUIM_IconList_Sort);
        /* Record finish time */
        data->iwidld_LastRefresh = GetCurrentTimeInSeconds();
        data->iwidld_FSChanged = FALSE;
        return (IPTR)TRUE;
    }

    return (IPTR)FALSE;
}
///

/*** Setup ******************************************************************/
#ifdef __AROS__
ICONWINDOWICONDRAWERLIST_CUSTOMCLASS
(
    IconWindowDrawerList, NULL, MUIC_IconDrawerList, NULL,
    OM_NEW,                                         struct opSet *,
    OM_SET,                                         struct opSet *,
    OM_GET,                                         struct opGet *,
    MUIM_Setup,                                     Msg,
    MUIM_Cleanup,                                   Msg,
    MUIM_DrawBackground,                            struct MUIP_DrawBackground *,
    MUIM_IconWindowDrawerList_FileSystemChanged,    Msg,
    MUIM_IconWindowIconList_RateLimitRefresh,       Msg
);
#else
ICONWINDOWICONDRAWERLIST_CUSTOMCLASS
(
    IconWindowDrawerList, NULL,  NULL, IconDrawerList_Class,
    OM_NEW,                                         struct opSet *,
    OM_SET,                                         struct opSet *,
    OM_GET,                                         struct opGet *,
    MUIM_Setup,                                     Msg,
    MUIM_Cleanup,                                   Msg,
    MUIM_DrawBackground,                            struct MUIP_DrawBackground *,
    MUIM_IconWindowDrawerList_FileSystemChanged,    Msg,
    MUIM_IconWindowIconList_RateLimitRefresh,       Msg
);
#endif
