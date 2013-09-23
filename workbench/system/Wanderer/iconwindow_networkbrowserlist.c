/*
  Copyright  2004-2013, The AROS Development Team. All rights reserved.
  $Id$
*/

#include "portable_macros.h"

#ifdef __AROS__
#define MUIMASTER_YES_INLINE_STDARG
#endif

//#define DEBUG_NETWORKBROWSER
//#define DEBUG_SHOWUSERFILES
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

#define BG_DRAWFLAG   0xf00dd00f

/*** Instance Data **********************************************************/

struct IconWindowIconList_DATA
{
    Object                      *iwcd_IconWindow;
    struct RastPort             *iwcd_RastPort;
    struct MUI_EventHandlerNode iwcd_EventHandlerNode;
#ifdef __AROS__
    struct Hook                 iwcd_ProcessIconListPrefs_hook;
#else
    struct Hook                 *iwcd_ProcessIconListPrefs_hook;
#endif

    IPTR                        iwcd_ViewPrefs_ID;
    Object                      *iwcd_ViewPrefs_NotificationObject;
};

struct IconWindowIconDrawerList_DATA
{
    Object                      *iwcd_IconWindow;
    struct RastPort             *iwcd_RastPort;
    struct MUI_EventHandlerNode iwcd_EventHandlerNode;
#ifdef __AROS__
    struct Hook                 iwcd_ProcessIconListPrefs_hook;
#else
    struct Hook                 *iwcd_ProcessIconListPrefs_hook;
#endif

    IPTR                        iwcd_ViewPrefs_ID;
    Object                      *iwcd_ViewPrefs_NotificationObject;
    struct NotifyRequest        iwdcd_DrawerNotifyRequest;
};

struct IconWindowIconVolumeList_DATA
{
    Object                      *iwcd_IconWindow;
    struct RastPort             *iwcd_RastPort;
    struct MUI_EventHandlerNode iwcd_EventHandlerNode;
#ifdef __AROS__
    struct Hook                 iwcd_ProcessIconListPrefs_hook;
#else
    struct Hook                 *iwcd_ProcessIconListPrefs_hook;
#endif

    IPTR                        iwcd_ViewPrefs_ID;
    Object                      *iwcd_ViewPrefs_NotificationObject;
#ifdef __AROS__
    struct Hook                 iwvcd_UpdateNetworkPrefs_hook;
#else
    struct Hook                 *iwvcd_UpdateNetworkPrefs_hook;
#endif

    IPTR                        iwvcd_ShowNetworkBrowser;
    IPTR                        iwvcd_ShowUserFolder;
};

struct IconWindowIconNetworkBrowserList_DATA
{
    Object                      *iwcd_IconWindow;
    struct RastPort             *iwcd_RastPort;
    struct MUI_EventHandlerNode iwcd_EventHandlerNode;
#ifdef __AROS__
    struct Hook                 iwcd_ProcessIconListPrefs_hook;
#else
    struct Hook                 *iwcd_ProcessIconListPrefs_hook;
#endif

    IPTR                        iwcd_ViewPrefs_ID;
    Object                      *iwcd_ViewPrefs_NotificationObject;
    struct Hook                 iwnbcd_UpdateNetworkPrefs_hook;
    struct List                 iwnbcd_NetworkClasses;
};

static char __icwc_intern_TxtBuff[TXTBUFF_LEN];

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct IconWindowIconList_DATA *data = INST_DATA(CLASS, self)

/*** Hook functions *********************************************************/
///IconWindowIconList__HookFunc_ProcessIconListPrefsFunc()
#ifdef __AROS__
AROS_UFH3(
    void, IconWindowIconList__HookFunc_ProcessIconListPrefsFunc,
    AROS_UFHA(struct Hook *,    hook,   A0),
    AROS_UFHA(APTR *,           obj,    A2),
    AROS_UFHA(IPTR *,             param,  A1)
)
{
#else
HOOKPROTO(IconWindowIconList__HookFunc_ProcessIconListPrefsFunc, void, APTR *obj, IPTR *param)
{
#endif
    AROS_USERFUNC_INIT

    /* Get our private data */
    Object *self = ( Object *)obj;
    IPTR CHANGED_ATTRIB = *param;
    Class *CLASS = OCLASS(self);

    SETUP_INST_DATA;

    Object *prefs = NULL;

    D(bug("[IconWindowIconList] %s()\n", __PRETTY_FUNCTION__));

    GET(_app(self), MUIA_Wanderer_Prefs, &prefs);

    if (prefs)
    {
        IPTR attrib_Current, attrib_Prefs, prefs_Processing = 0;
        BOOL options_changed = FALSE;

        D(bug("[IconWindowIconList] %s: Setting IconList options ..\n", __PRETTY_FUNCTION__));

        GET(prefs, MUIA_WandererPrefs_Processing, &prefs_Processing);

        switch (CHANGED_ATTRIB)
        {
        case MUIA_IconList_IconListMode:
            GET(self, MUIA_IconList_IconListMode, &attrib_Current);

            if (((attrib_Prefs = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_IconListMode)) != -1) &&
                (attrib_Current != attrib_Prefs))
            {
                D(bug("[IconWindowIconList] %s: IconList ListMode changed - updating ..\n", __PRETTY_FUNCTION__));
                options_changed = TRUE;
                if (prefs_Processing)
                {
                    NNSET(self, MUIA_IconList_IconListMode, attrib_Prefs);
                }
                else
                {
                    SET(self, MUIA_IconList_IconListMode, attrib_Prefs);
                }
            }
            break;

        case MUIA_IconList_LabelText_Mode:
            GET(self, MUIA_IconList_LabelText_Mode, &attrib_Current);

            if (((attrib_Prefs = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_LabelText_Mode)) != -1) &&
                (attrib_Current != attrib_Prefs))
            {
                D(bug("[IconWindowIconList] %s: IconList TextRenderMode changed - updating ..\n", __PRETTY_FUNCTION__));
                options_changed = TRUE;
                if (prefs_Processing)
                {
                    NNSET(self, MUIA_IconList_LabelText_Mode, attrib_Prefs);
                }
                else
                {
                    SET(self, MUIA_IconList_LabelText_Mode, attrib_Prefs);
                }
            }
            break;

        case MUIA_IconList_LabelText_MaxLineLen:
            GET(self, MUIA_IconList_LabelText_MaxLineLen, &attrib_Current);

            if (((attrib_Prefs = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_LabelText_MaxLineLen)) != -1) &&
                (attrib_Current != attrib_Prefs))
            {
                D(bug("[IconWindowIconList] %s: IconList Max Text Length changed - updating ..\n", __PRETTY_FUNCTION__));
                options_changed = TRUE;
                if (prefs_Processing)
                {
                    NNSET(self, MUIA_IconList_LabelText_MaxLineLen, attrib_Prefs);
                }
                else
                {
                    SET(self, MUIA_IconList_LabelText_MaxLineLen, attrib_Prefs);
                }
            }
            break;

        case MUIA_IconList_LabelText_MultiLine:
            GET(self, MUIA_IconList_LabelText_MultiLine, &attrib_Current);

            if (((attrib_Prefs = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_LabelText_MultiLine)) != -1) &&
                (attrib_Current != attrib_Prefs))
            {
                D(bug("[IconWindowIconList] %s: IconList Multi-Line changed - updating ..\n", __PRETTY_FUNCTION__));
                options_changed = TRUE;
                if (prefs_Processing)
                {
                    NNSET(self, MUIA_IconList_LabelText_MultiLine, attrib_Prefs);
                }
                else
                {
                    SET(self, MUIA_IconList_LabelText_MultiLine, attrib_Prefs);
                }
            }
            break;

        case MUIA_IconList_LabelText_MultiLineOnFocus:
            GET(self, MUIA_IconList_LabelText_MultiLineOnFocus, &attrib_Current);

            if (((attrib_Prefs = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_LabelText_MultiLineOnFocus)) != -1) &&
                (attrib_Current != attrib_Prefs))
            {
                D(bug("[IconWindowIconList] %s: Multi-Line on Focus changed - updating ..\n", __PRETTY_FUNCTION__));
                options_changed = TRUE;
                if (prefs_Processing)
                {
                    NNSET(self, MUIA_IconList_LabelText_MultiLineOnFocus, attrib_Prefs);
                }
                else
                {
                    SET(self, MUIA_IconList_LabelText_MultiLineOnFocus, attrib_Prefs);
                }
            }
            break;

        case MUIA_IconList_Icon_HorizontalSpacing:
            GET(self, MUIA_IconList_Icon_HorizontalSpacing, &attrib_Current);

            if (((attrib_Prefs = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_Icon_HorizontalSpacing)) != -1) &&
                (attrib_Current != attrib_Prefs))
            {
                D(bug("[IconWindowIconList] %s: Icon Horizontal Spacing changed - updating ..\n", __PRETTY_FUNCTION__));
                options_changed = TRUE;
                if (prefs_Processing)
                {
                    NNSET(self, MUIA_IconList_Icon_HorizontalSpacing, attrib_Prefs);
                }
                else
                {
                    SET(self, MUIA_IconList_Icon_HorizontalSpacing, attrib_Prefs);
                }
            }
            break;

        case MUIA_IconList_Icon_VerticalSpacing:
            GET(self, MUIA_IconList_Icon_VerticalSpacing, &attrib_Current);

            if (((attrib_Prefs = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_Icon_VerticalSpacing)) != -1) &&
                (attrib_Current != attrib_Prefs))
            {
                D(bug("[IconWindowIconList] %s: Icon Vertical Spacing changed - updating ..\n", __PRETTY_FUNCTION__));
                options_changed = TRUE;
                if (prefs_Processing)
                {
                    NNSET(self, MUIA_IconList_Icon_VerticalSpacing, attrib_Prefs);
                }
                else
                {
                    SET(self, MUIA_IconList_Icon_VerticalSpacing, attrib_Prefs);
                }
            }
            break;

        case MUIA_IconList_Icon_ImageSpacing:
            GET(self, MUIA_IconList_Icon_ImageSpacing, &attrib_Current);

            if (((attrib_Prefs = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_Icon_ImageSpacing)) != -1) &&
                (attrib_Current != attrib_Prefs))
            {
                D(bug("[IconWindowIconList] %s: Icon Label Image Spacing changed - updating ..\n", __PRETTY_FUNCTION__));
                options_changed = TRUE;
                if (prefs_Processing)
                {
                    NNSET(self, MUIA_IconList_Icon_ImageSpacing, attrib_Prefs);
                }
                else
                {
                    SET(self, MUIA_IconList_Icon_ImageSpacing, attrib_Prefs);
                }
            }
            break;

        case MUIA_IconList_LabelText_HorizontalPadding:
            GET(self, MUIA_IconList_LabelText_HorizontalPadding, &attrib_Current);

            if (((attrib_Prefs = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_LabelText_HorizontalPadding)) != -1) &&
                (attrib_Current != attrib_Prefs))
            {
                D(bug("[IconWindowIconList] %s: Icon Label Horizontal Padding changed - updating ..\n", __PRETTY_FUNCTION__));
                options_changed = TRUE;
                if (prefs_Processing)
                {
                    NNSET(self, MUIA_IconList_LabelText_HorizontalPadding, attrib_Prefs);
                }
                else
                {
                    SET(self, MUIA_IconList_LabelText_HorizontalPadding, attrib_Prefs);
                }
            }
            break;

        case MUIA_IconList_LabelText_VerticalPadding:
            GET(self, MUIA_IconList_LabelText_VerticalPadding, &attrib_Current);

            if (((attrib_Prefs = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_LabelText_VerticalPadding)) != -1) &&
                (attrib_Current != attrib_Prefs))
            {
                D(bug("[IconWindowIconList] %s: Icon Label Vertical Padding changed - updating ..\n", __PRETTY_FUNCTION__));
                options_changed = TRUE;
                if (prefs_Processing)
                {
                    NNSET(self, MUIA_IconList_LabelText_VerticalPadding, attrib_Prefs);
                }
                else
                {
                    SET(self, MUIA_IconList_LabelText_VerticalPadding, attrib_Prefs);
                }
            }
            break;

        case MUIA_IconList_LabelText_BorderWidth:
            GET(self, MUIA_IconList_LabelText_BorderWidth, &attrib_Current);

            if (((attrib_Prefs = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_LabelText_BorderWidth)) != -1) &&
                (attrib_Current != attrib_Prefs))
            {
                D(bug("[IconWindowIconList] %s: Icon Label Border Width changed - updating ..\n", __PRETTY_FUNCTION__));
                options_changed = TRUE;
                if (prefs_Processing)
                {
                    NNSET(self, MUIA_IconList_LabelText_BorderWidth, attrib_Prefs);
                }
                else
                {
                    SET(self, MUIA_IconList_LabelText_BorderWidth, attrib_Prefs);
                }
            }
            break;

        case MUIA_IconList_LabelText_BorderHeight:
            GET(self, MUIA_IconList_LabelText_BorderHeight, &attrib_Current);

            if (((attrib_Prefs = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_LabelText_BorderHeight)) != -1) &&
                (attrib_Current != attrib_Prefs))
            {
                D(bug("[IconWindowIconList] %s: Icon Label Border Height changed - updating ..\n", __PRETTY_FUNCTION__));
                options_changed = TRUE;
                if (prefs_Processing)
                {
                    NNSET(self, MUIA_IconList_LabelText_BorderHeight, attrib_Prefs);
                }
                else
                {
                    SET(self, MUIA_IconList_LabelText_BorderHeight, attrib_Prefs);
                }
            }
            break;

        default:
            D(bug("[IconWindowIconList] %s: Unhandled change\n", __PRETTY_FUNCTION__));
            break;
        }

        if (options_changed)
        {
            if (!(prefs_Processing))
            {
                D(bug("[IconWindowIconList] %s: IconList Options have changed, causing an update ..\n", __PRETTY_FUNCTION__));
                DoMethod(self, MUIM_IconList_Update);
        DoMethod(self, MUIM_IconList_Sort);
            }
            else if (data->iwcd_IconWindow)
            {
                SET(data->iwcd_IconWindow, MUIA_IconWindow_Changed, TRUE);
            }
        }
    }
    AROS_USERFUNC_EXIT
}
#ifndef __AROS__
MakeStaticHook(Hook_ProcessIconListPrefsFunc,IconWindowIconList__HookFunc_ProcessIconListPrefsFunc);
#endif
///

///IconWindowIconList__HookFunc_UpdateNetworkPrefsFunc()
#ifdef __AROS__
AROS_UFH3(
    void, IconWindowIconList__HookFunc_UpdateNetworkPrefsFunc,
    AROS_UFHA(struct Hook *,    hook,   A0),
    AROS_UFHA(APTR *,           obj,    A2),
    AROS_UFHA(APTR,             param,  A1)
)
{
#else
HOOKPROTO(IconWindowIconList__HookFunc_UpdateNetworkPrefsFunc, void, APTR *obj, APTR param)
{
#endif
    AROS_USERFUNC_INIT

    /* Get our private data */
    Object *self = ( Object *)obj;
    Object *prefs = NULL;
    Class *CLASS = *( Class **)param;

    SETUP_INST_DATA;

    D(bug("[IconWindowIconList] %s()\n", __PRETTY_FUNCTION__));

    GET(_app(self), MUIA_Wanderer_Prefs, &prefs);

    if (prefs)
    {
        BOOL    options_changed = FALSE;
        IPTR  prefs_Processing = 0;

        GET(prefs, MUIA_WandererPrefs_Processing, &prefs_Processing);

        if ((BOOL)XGET(_win(self), MUIA_IconWindow_IsRoot))
        {
            IPTR   current_ShowNetwork = 0;
            IPTR   prefs_ShowNetwork = 0;

            D(bug("[IconWindowIconList] %s: Setting ROOT view Network options ..\n", __PRETTY_FUNCTION__));

            GET(self, MUIA_IconWindowExt_NetworkBrowser_Show, &current_ShowNetwork);

            D(bug("[IconWindowIconList] %s: Current = %d\n", __PRETTY_FUNCTION__, current_ShowNetwork));

            GET(prefs, MUIA_IconWindowExt_NetworkBrowser_Show, &prefs_ShowNetwork);

            D(bug("[IconWindowIconList] %s: Prefs = %d\n", __PRETTY_FUNCTION__, prefs_ShowNetwork));

            if ((BOOL)current_ShowNetwork != (BOOL)prefs_ShowNetwork)
            {
                D(bug("[IconWindowIconList] %s: ROOT view Network prefs changed - updating ..\n", __PRETTY_FUNCTION__));
                options_changed = TRUE;
                ((struct IconWindowIconVolumeList_DATA *)data)->iwvcd_ShowNetworkBrowser = prefs_ShowNetwork;
            }
        }
        if ((options_changed) && !(prefs_Processing))
        {
            D(bug("[IconWindowIconList] %s: Network prefs changed, causing an update ..\n", __PRETTY_FUNCTION__));
            DoMethod(self, MUIM_IconList_Update);
        DoMethod(self, MUIM_IconList_Sort);
        }
        else if (data->iwcd_IconWindow)
        {
            SET(data->iwcd_IconWindow, MUIA_IconWindow_Changed, TRUE);
        }
    }
    AROS_USERFUNC_EXIT
}
#ifndef __AROS__
MakeStaticHook(Hook_UpdateNetworkPrefsFunc,IconWindowIconList__HookFunc_UpdateNetworkPrefsFunc);
#endif

#define BDRPLINELEN_MAX 1024
BOOL IconWindowIconList__Func_ParseBackdrop(Class *CLASS, Object *self, char *bdrp_dir)
{
    BPTR                bdrp_lock = (BPTR)NULL;
    char                *bdrp_file = NULL, *linebuf = NULL, *bdrp_fullfile = NULL, *bdrp_namepart = NULL;
    struct DiskObject   *bdrp_currfile_dob = NULL;
    BOOL                retVal = FALSE;

    if ((bdrp_dir == NULL) || (bdrp_dir[strlen(bdrp_dir) - 1] != ':'))
        return retVal;

    if ((bdrp_file = AllocVec(strlen(bdrp_dir) + 9 + 1, MEMF_CLEAR|MEMF_PUBLIC)) != NULL)
    {
        sprintf(bdrp_file, "%s.backdrop", bdrp_dir);
        if ((bdrp_lock = Open(bdrp_file, MODE_OLDFILE)))
        {
            D(bug("[IconWindowIconList] IconWindowIconList__Func_ParseBackdrop: Loading backdrop file: '%s'\n", bdrp_file));

            if ((linebuf = AllocMem(BDRPLINELEN_MAX, MEMF_PUBLIC)) != NULL)
            {
                while (FGets(bdrp_lock, linebuf, BDRPLINELEN_MAX))
                {
                    int linelen = 0;
                    if (*linebuf != ':')
                        continue;

                    linelen = strlen(linebuf) - 1; /* drop the newline char */
                    linebuf[linelen] = '\0';

                    if ((bdrp_fullfile = AllocVec(linelen + strlen(bdrp_dir), MEMF_CLEAR|MEMF_PUBLIC)) != NULL)
                    {
                        sprintf(bdrp_fullfile, "%s%s", bdrp_dir, &linebuf[1]);
                        bdrp_namepart = FilePart(bdrp_fullfile);
                        bdrp_currfile_dob = GetIconTags
                          (
                            bdrp_fullfile, 
                            ICONGETA_FailIfUnavailable, FALSE,
                            ICONGETA_Label,             bdrp_namepart,
                            TAG_DONE
                          );

                        D(bug("[IconWindowIconList] IconWindowIconList__Func_ParseBackdrop: LEAVEOUT Icon '%s' ('%s') DOB @ 0x%p\n", bdrp_fullfile, bdrp_namepart, bdrp_currfile_dob));

                        if (bdrp_currfile_dob)
                        {
                            struct IconEntry *this_entry = NULL;
                            if ((this_entry = (struct IconEntry *)DoMethod(self, MUIM_IconList_CreateEntry, (IPTR)bdrp_fullfile, (IPTR)bdrp_namepart, (IPTR)NULL, (IPTR)bdrp_currfile_dob, 0)))
                            {
                                struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
                                if (fib)
                                {
                                    BPTR                 fib_lock = (BPTR)NULL;
                                    if ((fib_lock = Lock(bdrp_fullfile, SHARED_LOCK)) != NULL)
                                    {
                                        if (Examine(fib_lock, fib))
                                        {
                                            if (fib->fib_DirEntryType == ST_FILE)
                                            {
                                                this_entry->ie_IconListEntry.type = ST_LINKFILE;
                                                D(bug("[IconWindowIconList] %s: LEAVEOUT ST_LINKFILE Entry @ 0x%p\n", __PRETTY_FUNCTION__, this_entry));
                                            }
                                            else if (fib->fib_DirEntryType == ST_USERDIR)
                                            {
                                                this_entry->ie_IconListEntry.type = ST_LINKDIR;
                                                D(bug("[IconWindowIconList] %s: LEAVEOUT ST_LINKDIR Entry @ 0x%p\n", __PRETTY_FUNCTION__, this_entry));
                                            }
                                            else
                                            {
                                                D(bug("[IconWindowIconList] %s: LEAVEOUT Unknown Entry Type @ 0x%p\n", __PRETTY_FUNCTION__, this_entry));
                                            }
                                        }
                                        UnLock(fib_lock);
                                    }
                                    FreeDosObject(DOS_FIB, fib);
                                }
                                retVal = TRUE;
                            }
                        }
                        FreeVec(bdrp_fullfile);
                    }
                }
                FreeMem(linebuf, BDRPLINELEN_MAX);
            }
            Close(bdrp_lock);
        }
        FreeVec(bdrp_file);
    }
    return retVal;
}

///
/*** Methods ****************************************************************/
///OM_NEW()
Object *IconWindowIconList__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    IPTR                            _newIconList__FSNotifyPort = 0;

    D(bug("[IconWindowIconList] IconWindowIconList__OM_NEW()\n"));

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
        D(bug("[IconWindowIconList] IconWindowIconList__OM_NEW: SELF = 0x%p\n", self));

#ifdef __AROS__
        data->iwcd_ProcessIconListPrefs_hook.h_Entry = ( HOOKFUNC )IconWindowIconList__HookFunc_ProcessIconListPrefsFunc;
#else
        data->iwcd_ProcessIconListPrefs_hook = &Hook_ProcessIconListPrefsFunc;
#endif

        if (_newIconList__FSNotifyPort != 0)
        {
            struct IconWindowIconDrawerList_DATA *drawerlist_data = (struct IconWindowIconDrawerList_DATA *)data;
            drawerlist_data->iwdcd_DrawerNotifyRequest.nr_stuff.nr_Msg.nr_Port = _newIconList__FSNotifyPort;
            D(bug("[IconWindowIconList] IconWindowIconList__OM_NEW: FS Notify Port @ 0x%p\n", _newIconList__FSNotifyPort));
        }
    }
    D(bug("[IconWindowIconList] obj = %ld\n", self));

    return self;
}
///

///OM_SET()
IPTR IconWindowIconList__OM_SET(Class *CLASS, Object *self, struct opSet *message)
{
    SETUP_INST_DATA;

    struct TagItem *tstate = message->ops_AttrList, *tag;

    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Background:
            {
                D(bug("[IconWindowIconList] %s: MUIA_Background\n", __PRETTY_FUNCTION__));
                break;
            }
        case MUIA_IconWindow_Window:
            {
                D(bug("[IconWindowIconList] %s: MUIA_IconWindow_Window @ %p\n", __PRETTY_FUNCTION__, tag->ti_Data));
                data->iwcd_IconWindow = (Object *)tag->ti_Data;
                break;
            }
        case MUIA_IconList_BufferRastport:
            {
                D(bug("[IconWindowIconList] %s: MUIA_IconList_BufferRastport @ %p\n", __PRETTY_FUNCTION__, tag->ti_Data));
                data->iwcd_RastPort = (struct RastPort *)tag->ti_Data;
                break;
            }
        }
    }
    return DoSuperMethodA(CLASS, self, (Msg) message);
}
///

///OM_GET()
IPTR IconWindowIconList__OM_GET(Class *CLASS, Object *self, struct opGet *message)
{
    //SETUP_INST_DATA;
    //IPTR *store = message->opg_Storage;
    IPTR  rv    = TRUE;

    switch (message->opg_AttrID)
    {
    default:
        rv = DoSuperMethodA(CLASS, self, (Msg) message);
    }

    return rv;
}
///

///IconWindowIconList__MUIM_Setup()
IPTR IconWindowIconList__MUIM_Setup
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

        GET(_win(self), MUIA_IconWindow_BackgroundAttrib, &data->iwcd_ViewPrefs_ID);
        D(bug("[IconWindowIconList] IconWindowIconList__MUIM_Setup: Window Background = '%s'\n", data->iwcd_ViewPrefs_ID));
        data->iwcd_ViewPrefs_NotificationObject = (Object *)DoMethod(prefs,
                                MUIM_WandererPrefs_ViewSettings_GetNotifyObject,
                                data->iwcd_ViewPrefs_ID);

D(bug("[IconWindowIconList] IconWindowIconList__MUIM_Setup: Background Notification Obj @ 0x%p\n", data->iwcd_ViewPrefs_NotificationObject));

        attrib_Prefs = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_IconListMode);
        if ((attrib_Prefs != (IPTR)-1)  && (attrib_Prefs != XGET(self, MUIA_IconList_IconListMode))) SET(self, MUIA_IconList_IconListMode, attrib_Prefs);

        attrib_Prefs = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_LabelText_Mode);
        if ((attrib_Prefs != (IPTR)-1)  && (attrib_Prefs != XGET(self, MUIA_IconList_LabelText_Mode)))  SET(self, MUIA_IconList_LabelText_Mode, attrib_Prefs);

        attrib_Prefs = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_LabelText_MaxLineLen);
        if ((attrib_Prefs != (IPTR)-1)  && (attrib_Prefs != XGET(self, MUIA_IconList_LabelText_MaxLineLen)))  SET(self, MUIA_IconList_LabelText_MaxLineLen, attrib_Prefs);

        attrib_Prefs = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_LabelText_MultiLine);
        if ((attrib_Prefs != (IPTR)-1)  && (attrib_Prefs != XGET(self, MUIA_IconList_LabelText_MultiLine)))  SET(self, MUIA_IconList_LabelText_MultiLine, attrib_Prefs);

        attrib_Prefs = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_LabelText_MultiLineOnFocus);
        if ((attrib_Prefs != (IPTR)-1)  && (attrib_Prefs != XGET(self, MUIA_IconList_LabelText_MultiLineOnFocus)))  SET(self, MUIA_IconList_LabelText_MultiLineOnFocus, attrib_Prefs);

        attrib_Prefs = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_Icon_HorizontalSpacing);
        if ((attrib_Prefs != (IPTR)-1)  && (attrib_Prefs != XGET(self, MUIA_IconList_Icon_HorizontalSpacing)))  SET(self, MUIA_IconList_Icon_HorizontalSpacing, attrib_Prefs);

        attrib_Prefs = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_Icon_VerticalSpacing);
        if ((attrib_Prefs != (IPTR)-1)  && (attrib_Prefs != XGET(self, MUIA_IconList_Icon_VerticalSpacing)))  SET(self, MUIA_IconList_Icon_VerticalSpacing, attrib_Prefs);

        attrib_Prefs = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_Icon_ImageSpacing);
        if ((attrib_Prefs != (IPTR)-1)  && (attrib_Prefs != XGET(self, MUIA_IconList_Icon_ImageSpacing)))  SET(self, MUIA_IconList_Icon_ImageSpacing, attrib_Prefs);

        attrib_Prefs = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_LabelText_HorizontalPadding);
        if ((attrib_Prefs != (IPTR)-1)  && (attrib_Prefs != XGET(self, MUIA_IconList_LabelText_HorizontalPadding)))  SET(self, MUIA_IconList_LabelText_HorizontalPadding, attrib_Prefs);

        attrib_Prefs = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_LabelText_VerticalPadding);
        if ((attrib_Prefs != (IPTR)-1)  && (attrib_Prefs != XGET(self, MUIA_IconList_LabelText_VerticalPadding)))  SET(self, MUIA_IconList_LabelText_VerticalPadding, attrib_Prefs);

        attrib_Prefs = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_LabelText_BorderWidth);
        if ((attrib_Prefs != (IPTR)-1)  && (attrib_Prefs != XGET(self, MUIA_IconList_LabelText_BorderWidth)))  SET(self, MUIA_IconList_LabelText_BorderWidth, attrib_Prefs);

        attrib_Prefs = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_LabelText_BorderHeight);
        if ((attrib_Prefs != (IPTR)-1)  && (attrib_Prefs != XGET(self, MUIA_IconList_LabelText_BorderHeight)))  SET(self, MUIA_IconList_LabelText_BorderHeight, attrib_Prefs);

        /* Configure notifications incase they get updated =) */
        DoMethod
          (
            data->iwcd_ViewPrefs_NotificationObject, MUIM_Notify, MUIA_IconList_IconListMode, MUIV_EveryTime,
            (IPTR) self, 3, 
            MUIM_CallHook, &data->iwcd_ProcessIconListPrefs_hook, (IPTR)MUIA_IconList_IconListMode
          );

        DoMethod
          (
            data->iwcd_ViewPrefs_NotificationObject, MUIM_Notify, MUIA_IconList_LabelText_Mode, MUIV_EveryTime,
            (IPTR) self, 3, 
            MUIM_CallHook, &data->iwcd_ProcessIconListPrefs_hook, (IPTR)MUIA_IconList_LabelText_Mode
          );

        DoMethod
          (
            data->iwcd_ViewPrefs_NotificationObject, MUIM_Notify, MUIA_IconList_LabelText_MaxLineLen, MUIV_EveryTime,
            (IPTR) self, 3, 
            MUIM_CallHook, &data->iwcd_ProcessIconListPrefs_hook, (IPTR)MUIA_IconList_LabelText_MaxLineLen
          );

        DoMethod
          (
            data->iwcd_ViewPrefs_NotificationObject, MUIM_Notify, MUIA_IconList_LabelText_MultiLine, MUIV_EveryTime,
            (IPTR) self, 3, 
            MUIM_CallHook, &data->iwcd_ProcessIconListPrefs_hook, (IPTR)MUIA_IconList_LabelText_MultiLine
          );

        DoMethod
          (
            data->iwcd_ViewPrefs_NotificationObject, MUIM_Notify, MUIA_IconList_LabelText_MultiLineOnFocus, MUIV_EveryTime,
            (IPTR) self, 3, 
            MUIM_CallHook, &data->iwcd_ProcessIconListPrefs_hook, (IPTR)MUIA_IconList_LabelText_MultiLineOnFocus
          );

        DoMethod
          (
            data->iwcd_ViewPrefs_NotificationObject, MUIM_Notify, MUIA_IconList_Icon_HorizontalSpacing, MUIV_EveryTime,
            (IPTR) self, 3, 
            MUIM_CallHook, &data->iwcd_ProcessIconListPrefs_hook, (IPTR)MUIA_IconList_Icon_HorizontalSpacing
          );

        DoMethod
          (
            data->iwcd_ViewPrefs_NotificationObject, MUIM_Notify, MUIA_IconList_Icon_VerticalSpacing, MUIV_EveryTime,
            (IPTR) self, 3, 
            MUIM_CallHook, &data->iwcd_ProcessIconListPrefs_hook, (IPTR)MUIA_IconList_Icon_VerticalSpacing
          );

        DoMethod
          (
            data->iwcd_ViewPrefs_NotificationObject, MUIM_Notify, MUIA_IconList_Icon_ImageSpacing, MUIV_EveryTime,
            (IPTR) self, 3, 
            MUIM_CallHook, &data->iwcd_ProcessIconListPrefs_hook, (IPTR)MUIA_IconList_Icon_ImageSpacing
          );

        DoMethod
          (
            data->iwcd_ViewPrefs_NotificationObject, MUIM_Notify, MUIA_IconList_LabelText_HorizontalPadding, MUIV_EveryTime,
            (IPTR) self, 3, 
            MUIM_CallHook, &data->iwcd_ProcessIconListPrefs_hook, (IPTR)MUIA_IconList_LabelText_HorizontalPadding
          );

        DoMethod
          (
            data->iwcd_ViewPrefs_NotificationObject, MUIM_Notify, MUIA_IconList_LabelText_VerticalPadding, MUIV_EveryTime,
            (IPTR) self, 3, 
            MUIM_CallHook, &data->iwcd_ProcessIconListPrefs_hook, (IPTR)MUIA_IconList_LabelText_VerticalPadding
          );

        DoMethod
          (
            data->iwcd_ViewPrefs_NotificationObject, MUIM_Notify, MUIA_IconList_LabelText_BorderWidth, MUIV_EveryTime,
            (IPTR) self, 3, 
            MUIM_CallHook, &data->iwcd_ProcessIconListPrefs_hook, (IPTR)MUIA_IconList_LabelText_BorderWidth
          );

        DoMethod
          (
            data->iwcd_ViewPrefs_NotificationObject, MUIM_Notify, MUIA_IconList_LabelText_BorderHeight, MUIV_EveryTime,
            (IPTR) self, 3, 
            MUIM_CallHook, &data->iwcd_ProcessIconListPrefs_hook, (IPTR)MUIA_IconList_LabelText_BorderHeight
          );
    }
  
    if ((BOOL)XGET(_win(self), MUIA_IconWindow_IsRoot))
    {
        if (prefs)
        {
#ifdef __AROS__
            ((struct IconWindowIconVolumeList_DATA *)data)->iwvcd_UpdateNetworkPrefs_hook.h_Entry = ( HOOKFUNC )IconWindowIconList__HookFunc_UpdateNetworkPrefsFunc;
#else
            ((struct IconWindowIconVolumeList_DATA *)data)->iwvcd_UpdateNetworkPrefs_hook = &Hook_UpdateNetworkPrefsFunc;
#endif

            DoMethod
              (
                prefs, MUIM_Notify, MUIA_IconWindowExt_NetworkBrowser_Show, MUIV_EveryTime,
                (IPTR) self, 3, 
                MUIM_CallHook, &((struct IconWindowIconVolumeList_DATA *)data)->iwvcd_UpdateNetworkPrefs_hook, (IPTR)CLASS
              );
        }

        if (muiRenderInfo(self))
        {
            D(bug("[IconWindowIconList] IconWindowIconList__MUIM_Window_Setup: Setting up EventHandler for (IDCMP_DISKINSERTED | IDCMP_DISKREMOVED)\n"));

            data->iwcd_EventHandlerNode.ehn_Priority = 1;
            data->iwcd_EventHandlerNode.ehn_Flags    = MUI_EHF_GUIMODE;
            data->iwcd_EventHandlerNode.ehn_Object   = self;
            data->iwcd_EventHandlerNode.ehn_Class    = CLASS;
            data->iwcd_EventHandlerNode.ehn_Events   = IDCMP_DISKINSERTED | IDCMP_DISKREMOVED;

            DoMethod(_win(self), MUIM_Window_AddEventHandler, &data->iwcd_EventHandlerNode);
        }
        else
        {
            D(bug("[IconWindowIconList] IconWindowIconList__MUIM_Window_Setup: Couldnt add IDCMP EventHandler!\n"));
        }
    }
    else
    {
        /* Setup notification on the directory -------------------------------- */
        STRPTR directory_path = NULL;
        GET(self, MUIA_IconDrawerList_Drawer, &directory_path);

        if (directory_path != NULL)
        {
            struct IconWindowIconDrawerList_DATA *drawerlist_data = (struct IconWindowIconDrawerList_DATA *)data;

            if (drawerlist_data->iwdcd_DrawerNotifyRequest.nr_stuff.nr_Msg.nr_Port != NULL)
            {
                drawerlist_data->iwdcd_DrawerNotifyRequest.nr_Name                 = directory_path;
                drawerlist_data->iwdcd_DrawerNotifyRequest.nr_Flags                = NRF_SEND_MESSAGE;
                drawerlist_data->iwdcd_DrawerNotifyRequest.nr_UserData             = self;

                if (StartNotify(&drawerlist_data->iwdcd_DrawerNotifyRequest))
                {
D(bug("[IconWindowIconList] IconWindowIconList__MUIM_Window_Setup: Drawer-notification setup on '%s'\n", drawerlist_data->iwdcd_DrawerNotifyRequest.nr_Name));
                }
                else
                {
D(bug("[IconWindowIconList] IconWindowIconList__MUIM_Window_Setup: FAILED to setup Drawer-notification!\n"));
                    drawerlist_data->iwdcd_DrawerNotifyRequest.nr_Name = NULL;
                }
            }
        }
    }

    D(bug("[IconWindowIconList] IconWindowIconList__MUIM_Window_Setup: Setup complete!\n"));
  
    return TRUE;
}
///

///IconWindowIconList__MUIM_Cleanup()
IPTR IconWindowIconList__MUIM_Cleanup
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;

    Object *prefs = NULL;

    D(bug("[IconWindowIconList] IconWindowIconList__MUIM_Cleanup()\n"));
    GET(_app(self), MUIA_Wanderer_Prefs, &prefs);

    if (prefs)
    {
        DoMethod
          (
            data->iwcd_ViewPrefs_NotificationObject, MUIM_KillNotifyObj, MUIA_IconList_IconListMode, (IPTR)self
          );

        DoMethod
          (
            data->iwcd_ViewPrefs_NotificationObject, MUIM_KillNotifyObj, MUIA_IconList_LabelText_Mode, (IPTR)self
          );

        DoMethod
          (
            data->iwcd_ViewPrefs_NotificationObject, MUIM_KillNotifyObj, MUIA_IconList_LabelText_MaxLineLen, (IPTR)self
          );

        DoMethod
          (
            data->iwcd_ViewPrefs_NotificationObject, MUIM_KillNotifyObj, MUIA_IconList_LabelText_MultiLine, (IPTR)self
          );

        DoMethod
          (
            data->iwcd_ViewPrefs_NotificationObject, MUIM_KillNotifyObj, MUIA_IconList_LabelText_MultiLineOnFocus, (IPTR)self
          );

        DoMethod
          (
            data->iwcd_ViewPrefs_NotificationObject, MUIM_KillNotifyObj, MUIA_IconList_Icon_HorizontalSpacing, (IPTR)self
          );

        DoMethod
          (
            data->iwcd_ViewPrefs_NotificationObject, MUIM_KillNotifyObj, MUIA_IconList_Icon_VerticalSpacing, (IPTR)self
          );

        DoMethod
          (
            data->iwcd_ViewPrefs_NotificationObject, MUIM_KillNotifyObj, MUIA_IconList_Icon_ImageSpacing, (IPTR)self
          );

        DoMethod
          (
            data->iwcd_ViewPrefs_NotificationObject, MUIM_KillNotifyObj, MUIA_IconList_LabelText_HorizontalPadding, (IPTR)self
          );

        DoMethod
          (
            data->iwcd_ViewPrefs_NotificationObject, MUIM_KillNotifyObj, MUIA_IconList_LabelText_VerticalPadding, (IPTR)self
          );

        DoMethod
          (
            data->iwcd_ViewPrefs_NotificationObject, MUIM_KillNotifyObj, MUIA_IconList_LabelText_BorderWidth, (IPTR)self
          );

        DoMethod
          (
            data->iwcd_ViewPrefs_NotificationObject, MUIM_KillNotifyObj, MUIA_IconList_LabelText_BorderHeight, (IPTR)self
          );
    }

    if ((BOOL)XGET(_win(self), MUIA_IconWindow_IsRoot))
    {
        if (prefs)
        {
            DoMethod
              (
                prefs,
                MUIM_KillNotifyObj, MUIA_IconWindowExt_NetworkBrowser_Show, (IPTR) self
              );
        }
        D(bug("[IconWindowIconList] IconWindowIconList__MUIM_Cleanup: (ROOT WINDOW) Removing our Disk Event Handler\n"));
        DoMethod(_win(self), MUIM_Window_RemEventHandler, &data->iwcd_EventHandlerNode);
    }
    else
    {
        struct IconWindowIconDrawerList_DATA *drawerlist_data = (struct IconWindowIconDrawerList_DATA *)data;
        if (drawerlist_data->iwdcd_DrawerNotifyRequest.nr_Name != NULL)
        {
D(bug("[IconWindowIconList] IconWindowIconList__MUIM_Cleanup: (DRAWER WINDOW) Removing our Drawer Notification Request\n"));
            EndNotify(&drawerlist_data->iwdcd_DrawerNotifyRequest);
        }
    }

    return DoSuperMethodA(CLASS, self, message);
}
///

///IconWindowIconList__MUIM_HandleEvent()
IPTR IconWindowIconList__MUIM_HandleEvent
(
    Class *CLASS, Object *self, struct MUIP_HandleEvent *message
)
{
    //SETUP_INST_DATA;

    struct IntuiMessage *imsg = message->imsg;

    D(bug("[IconWindowIconList] IconWindowIconList__MUIM_HandleEvent()\n"));

    if(imsg->Class == IDCMP_DISKINSERTED) 
    {
        D(bug("[IconWindowIconList] IconWindowIconList__MUIM_HandleEvent: IDCMP_DISKINSERTED\n"));
        DoMethod(self, MUIM_IconList_Update);
    DoMethod(self, MUIM_IconList_Sort);
        return(MUI_EventHandlerRC_Eat);
    }
    else if (imsg->Class == IDCMP_DISKREMOVED) 
    {
        D(bug("[IconWindowIconList] IconWindowIconList__MUIM_HandleEvent: IDCMP_DISKREMOVED\n"));
        DoMethod(self, MUIM_IconList_Update);
    DoMethod(self, MUIM_IconList_Sort);
        return(MUI_EventHandlerRC_Eat);
    }
    return 0;
}
///

///IconWindowIconList__MUIM_DrawBackground()
IPTR IconWindowIconList__MUIM_DrawBackground
(
  Class *CLASS, Object *self, struct MUIP_DrawBackground *message
)
{
    SETUP_INST_DATA;

    IPTR        retVal = (IPTR)TRUE;
    IPTR                clip = 0;
    struct RastPort           *DrawBackGround_RastPort;
    struct IconWindowBackFillMsg  DrawBackGround_BackFillMsg;

    D(bug("[IconWindow] IconWindowIconList__MUIM_DrawBackground()\n"));

    if ((iconwindow_BackFill_Active == NULL) ||
      (data->iwcd_IconWindow == NULL))
    {
        D(bug("[IconWindow] IconWindowIconList__MUIM_DrawBackground: No Backfill support/Window not set .. causing parent class to render\n"));
        goto iwc_ParentBackground;
    }

    DrawBackGround_RastPort = _rp(self);

    if ((data->iwcd_RastPort != NULL) && (DrawBackGround_RastPort != data->iwcd_RastPort))
    {
        DrawBackGround_RastPort = data->iwcd_RastPort;

        DrawBackGround_BackFillMsg.AreaBounds.MinX = 0;
        DrawBackGround_BackFillMsg.AreaBounds.MinY = 0;
        DrawBackGround_BackFillMsg.AreaBounds.MaxX = _mwidth(self);
        DrawBackGround_BackFillMsg.AreaBounds.MaxY = _mheight(self);

        DrawBackGround_BackFillMsg.DrawBounds.MinX = message->left - _mleft(self);
        DrawBackGround_BackFillMsg.DrawBounds.MinY = message->top - _mtop(self);
        DrawBackGround_BackFillMsg.DrawBounds.MaxX = message->width;
        DrawBackGround_BackFillMsg.DrawBounds.MaxY = message->height;
    }
    else
    {
        DrawBackGround_BackFillMsg.AreaBounds.MinX = _mleft(self);
        DrawBackGround_BackFillMsg.AreaBounds.MinY = _mtop(self);
        DrawBackGround_BackFillMsg.AreaBounds.MaxX = (_mleft(self) + _mwidth(self));
        DrawBackGround_BackFillMsg.AreaBounds.MaxY = (_mtop(self) + _mheight(self));

        DrawBackGround_BackFillMsg.DrawBounds.MinX = message->left;
        DrawBackGround_BackFillMsg.DrawBounds.MinY = message->top;
        DrawBackGround_BackFillMsg.DrawBounds.MaxX = (message->left + message->width);
        DrawBackGround_BackFillMsg.DrawBounds.MaxY = (message->top + message->height);
    }

    DrawBackGround_BackFillMsg.Layer = DrawBackGround_RastPort->Layer;

    /* Offset into source image (ala scroll bar position) */
    DrawBackGround_BackFillMsg.OffsetX = message->xoffset;
    DrawBackGround_BackFillMsg.OffsetY = message->yoffset;

    D(bug("[IconWindow] IconWindowIconList__MUIM_DrawBackground: RastPort @ 0x%p\n", DrawBackGround_RastPort));
  
    if ((retVal = DoMethod(data->iwcd_IconWindow, MUIM_IconWindow_BackFill_DrawBackground, XGET(data->iwcd_IconWindow, MUIA_IconWindow_BackFillData), &DrawBackGround_BackFillMsg, DrawBackGround_RastPort)) == (IPTR)TRUE)
    {
        D(bug("[IconWindow] IconWindowIconList__MUIM_DrawBackground: Backfill module rendered background ..\n"));
        return retVal;
    }
    D(bug("[IconWindow] IconWindowIconList__MUIM_DrawBackground: Backfill module failed to render background ..\n"));

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

///IconWindowIconList__MUIM_IconList_Update()
IPTR IconWindowIconList__MUIM_IconList_Update
(
    Class *CLASS, Object *self, struct MUIP_IconList_Update *message
)
{
    SETUP_INST_DATA;

    IPTR        retVal = (IPTR)TRUE;

    if ((BOOL)XGET(_win(self), MUIA_IconWindow_IsRoot))
    {
    struct IconList_Entry *icon_entry    = (IPTR)MUIV_IconList_NextIcon_Start;
        Object *prefs = NULL;
        BOOL    sort_list = FALSE;

        D(bug("[IconWindowIconList] IconWindowIconList__MUIM_IconList_Update: (ROOT WINDOW) Causing parent to update\n"));
        retVal = DoSuperMethodA(CLASS, self, (Msg) message);

        D(bug("[IconWindowIconList] IconWindowIconList__MUIM_IconList_Update: Checking for '.backdrop' files\n"));
        do
        {
            DoMethod(self, MUIM_IconList_NextIcon, MUIV_IconList_NextIcon_Visible, (IPTR)&icon_entry);
            if (
                ((IPTR)icon_entry != MUIV_IconList_NextIcon_End) &&
                ((icon_entry->type == ST_ROOT) && !(icon_entry->flags & ICONENTRY_VOL_OFFLINE))
              )
            {
                D(bug("[IconWindowIconList] IconWindowIconList__MUIM_IconList_Update: checking entry '%s'\n", icon_entry->label));
                if (IconWindowIconList__Func_ParseBackdrop(CLASS, self, icon_entry->label))
                    sort_list = TRUE;
            }
            else
            {
                break;
            }
        } while (TRUE);

        D(bug("[IconWindowIconList] IconWindowIconList__MUIM_IconList_Update: Check if we should show NetworkBrowser Icon ..\n"));

        GET(_app(self), MUIA_Wanderer_Prefs, &prefs);

        if (prefs)
        {
            struct IconWindowIconVolumeList_DATA *volumel_data = (struct IconWindowIconVolumeList_DATA *)data;

            GET(prefs, MUIA_IconWindowExt_NetworkBrowser_Show, &volumel_data->iwvcd_ShowNetworkBrowser);

#if defined(DEBUG_NETWORKBROWSER)
            volumel_data->iwvcd_ShowNetworkBrowser = TRUE;
#endif

            if (volumel_data->iwvcd_ShowNetworkBrowser)
            {
                struct DiskObject    *_nb_dob = NULL;
                _nb_dob = GetIconTags
                  (
                    "ENV:SYS/def_NetworkHost", 
                    ICONGETA_FailIfUnavailable, FALSE,
                    ICONGETA_Label,             (IPTR)"Network Access..",
                    TAG_DONE
                  );

                D(bug("[IconWindowIconList] IconWindowIconList__MUIM_IconList_Update: NetworkBrowser Icon DOB @ 0x%p\n", _nb_dob));

                if (_nb_dob)
                {
                    struct Node *this_entry = NULL;
                    if ((this_entry = (struct Node *)DoMethod(self, MUIM_IconList_CreateEntry, (IPTR)"?wanderer.networkbrowse?", (IPTR)"Network Access..", (IPTR)NULL, (IPTR)_nb_dob, 0)))
                    {
                        this_entry->ln_Pri = 3;   /// Network Access gets Priority 3 so its displayed after special dirs
                        sort_list = TRUE;
                        D(bug("[IconWindowIconList] IconWindowIconList__MUIM_IconList_Update: NetworkBrowser Icon Entry @ 0x%p\n", this_entry));
                    }
                }
            }

            GET(prefs, MUIA_IconWindowExt_UserFiles_ShowFilesFolder, &volumel_data->iwvcd_ShowUserFolder);

#if defined(DEBUG_SHOWUSERFILES)
            volumel_data->iwvcd_ShowUserFolder = TRUE;
#endif
            if (volumel_data->iwvcd_ShowUserFolder)
            {
                if (GetVar("SYS/Wanderer/userfiles.prefs", __icwc_intern_TxtBuff, TXTBUFF_LEN, GVF_GLOBAL_ONLY) != -1)
                {
                    char * userfiles_path = NULL;

                    D(bug("[IconWindowIconList] IconWindowIconList__MUIM_IconList_Update: SYS/UserFilesLocation = '%s'\n", __icwc_intern_TxtBuff));

                    if ((userfiles_path = AllocVec(strlen(__icwc_intern_TxtBuff) + 1, MEMF_CLEAR|MEMF_PUBLIC)) != NULL)
                    {
                        struct DiskObject    *_nb_dob = NULL;

                        D(bug("[IconWindowIconList] IconWindowIconList__MUIM_IconList_Update: UserFilesLocation Path storage @ 0x%p\n", userfiles_path));

                        strcpy(userfiles_path, __icwc_intern_TxtBuff);

                        D(bug("[IconWindowIconList] IconWindowIconList__MUIM_IconList_Update: UserFilesLocation Path storage contains '%s'\n", userfiles_path));

                        _nb_dob = GetIconTags
                          (
                            "ENV:SYS/def_UserHome", 
                            ICONGETA_FailIfUnavailable, FALSE,
                            ICONGETA_Label,             (IPTR)"User Files..",
                            TAG_DONE
                          );

                        D(bug("[IconWindowIconList] IconWindowIconList__MUIM_Window_Setup: UserFiles Icon DOB @ 0x%p\n", _nb_dob));

                        if (_nb_dob)
                        {
                            struct Node *this_entry = NULL;
                            if ((this_entry = (struct Node *)DoMethod(self, MUIM_IconList_CreateEntry, userfiles_path, (IPTR)"User Files..", (IPTR)NULL, (IPTR)_nb_dob, 0)))
                            {
                                this_entry->ln_Pri = 5;   /// Special dirs get Priority 5
                                sort_list = TRUE;
                            }
                        }
                    }
                }
            }
            if (sort_list) DoMethod(self, MUIM_IconList_Sort);
        }
    }
    else
    {
        retVal = TRUE;
        DoMethod(self, MUIM_IconList_Clear);
    }

    return retVal;
}
///
/*** Setup ******************************************************************/
#ifdef __AROS__
ICONWINDOWICONDRAWERLIST_CUSTOMCLASS
(
    IconWindowIconDrawerList, IconWindowIconList, NULL, MUIC_IconDrawerList, NULL,
    OM_NEW,                        struct opSet *,
    OM_SET,                        struct opSet *,
    OM_GET,                        struct opGet *,
    MUIM_Setup,                    Msg,
    MUIM_Cleanup,                  Msg,
    MUIM_DrawBackground,           Msg
);

ICONWINDOWICONVOLUMELIST_CUSTOMCLASS
(
    IconWindowIconVolumeList, IconWindowIconList, NULL, MUIC_IconVolumeList, NULL,
    OM_NEW,                        struct opSet *,
    OM_SET,                        struct opSet *,
    OM_GET,                        struct opGet *,
    MUIM_Setup,                    Msg,
    MUIM_Cleanup,                  Msg,
    MUIM_DrawBackground,           Msg,
    MUIM_HandleEvent,              Msg,
    MUIM_IconList_Update,          struct MUIP_IconList_Update *
);

ICONWINDOWICONNETWORKBROWSERLIST_CUSTOMCLASS
(
    IconWindowIconNetworkBrowserList, IconWindowIconList, NULL, MUIC_IconList, NULL,
    OM_NEW,                        struct opSet *,
    OM_SET,                        struct opSet *,
    OM_GET,                        struct opGet *,
    MUIM_Setup,                    Msg,
    MUIM_Cleanup,                  Msg,
    MUIM_DrawBackground,           Msg,
    MUIM_HandleEvent,              Msg,
    MUIM_IconList_Update,          struct MUIP_IconList_Update *
);

#else
ICONWINDOWICONDRAWERLIST_CUSTOMCLASS
(
    IconWindowIconDrawerList, IconWindowIconList, NULL,  NULL, IconDrawerList_Class,
    OM_NEW,                        struct opSet *,
    OM_SET,                        struct opSet *,
    OM_GET,                        struct opGet *,
    MUIM_Setup,                    Msg,
    MUIM_Cleanup,                  Msg,
    MUIM_DrawBackground,           Msg
);

ICONWINDOWICONVOLUMELIST_CUSTOMCLASS
(
    IconWindowIconVolumeList, IconWindowIconList, NULL,  NULL, IconVolumeList_Class,
    OM_NEW,                        struct opSet *,
    OM_SET,                        struct opSet *,
    OM_GET,                        struct opGet *,
    MUIM_Setup,                    Msg,
    MUIM_Cleanup,                  Msg,
    MUIM_DrawBackground,           Msg,
    MUIM_HandleEvent,              Msg,
    MUIM_IconList_Update,          struct MUIP_IconList_Update *
);

ICONWINDOWICONNETWORKBROWSERLIST_CUSTOMCLASS
(
    IconWindowIconNetworkBrowserList, IconWindowIconList, NULL, NULL, IconList_Class,
    OM_NEW,                        struct opSet *,
    OM_SET,                        struct opSet *,
    OM_GET,                        struct opGet *,
    MUIM_Setup,                    Msg,
    MUIM_Cleanup,                  Msg,
    MUIM_DrawBackground,           Msg,
    MUIM_HandleEvent,              Msg,
    MUIM_IconList_Update,          struct MUIP_IconList_Update *
);
#endif
