/*
  Copyright  2004-2009, The AROS Development Team. All rights reserved.
  $Id$
*/

#define ZCC_QUIET

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

#define WIWVLVERS       1
#define WIWVLREV        0

#define BG_DRAWFLAG     0xf00dd00f

/*** Instance Data **********************************************************/

struct IconWindowVolumeList_DATA
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
    char                        *iwvcd_UserFolderPath;
};

static char __icwc_intern_TxtBuff[TXTBUFF_LEN];

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct IconWindowVolumeList_DATA *data = INST_DATA(CLASS, self)

/*** Hook functions *********************************************************/
///IconWindowVolumeList__HookFunc_ProcessIconListPrefsFunc()
#ifdef __AROS__
AROS_UFH3(
    void, IconWindowVolumeList__HookFunc_ProcessIconListPrefsFunc,
    AROS_UFHA(struct Hook *,    hook,   A0),
    AROS_UFHA(APTR *,           obj,    A2),
    AROS_UFHA(IPTR *,             param,  A1)
)
{
#else
HOOKPROTO(IconWindowVolumeList__HookFunc_ProcessIconListPrefsFunc, void, APTR *obj, IPTR *param)
{
#endif
    AROS_USERFUNC_INIT

    /* Get our private data */
    Object *self = ( Object *)obj;
    IPTR CHANGED_ATTRIB = *param;
    Class *CLASS = OCLASS(self);

    SETUP_INST_DATA;

    Object *prefs = NULL;

    D(bug("[Wanderer:VolumeList]: %s()\n", __PRETTY_FUNCTION__));

    GET(_app(self), MUIA_Wanderer_Prefs, &prefs);

    if (prefs)
    {
        IPTR attrib_Current, attrib_Prefs, prefs_Processing = 0;
        BOOL options_changed = FALSE;

        D(bug("[Wanderer:VolumeList] %s: Setting IconList options ..\n", __PRETTY_FUNCTION__));

        GET(prefs, MUIA_WandererPrefs_Processing, &prefs_Processing);

        switch (CHANGED_ATTRIB)
        {
        case MUIA_IconList_IconListMode:
            GET(self, MUIA_IconList_IconListMode, &attrib_Current);

            if (((attrib_Prefs = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_IconListMode)) != -1) &&
                (attrib_Current != attrib_Prefs))
            {
                D(bug("[Wanderer:VolumeList] %s: IconList ListMode changed - updating ..\n", __PRETTY_FUNCTION__));
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
                D(bug("[Wanderer:VolumeList] %s: IconList TextRenderMode changed - updating ..\n", __PRETTY_FUNCTION__));
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
                D(bug("[Wanderer:VolumeList] %s: IconList Max Text Length changed - updating ..\n", __PRETTY_FUNCTION__));
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
                D(bug("[Wanderer:VolumeList] %s: IconList Multi-Line changed - updating ..\n", __PRETTY_FUNCTION__));
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
                D(bug("[Wanderer:VolumeList] %s: Multi-Line on Focus changed - updating ..\n", __PRETTY_FUNCTION__));
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
                D(bug("[Wanderer:VolumeList] %s: Icon Horizontal Spacing changed - updating ..\n", __PRETTY_FUNCTION__));
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
                D(bug("[Wanderer:VolumeList] %s: Icon Vertical Spacing changed - updating ..\n", __PRETTY_FUNCTION__));
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
                D(bug("[Wanderer:VolumeList] %s: Icon Label Image Spacing changed - updating ..\n", __PRETTY_FUNCTION__));
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
                D(bug("[Wanderer:VolumeList] %s: Icon Label Horizontal Padding changed - updating ..\n", __PRETTY_FUNCTION__));
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
                D(bug("[Wanderer:VolumeList] %s: Icon Label Vertical Padding changed - updating ..\n", __PRETTY_FUNCTION__));
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
                D(bug("[Wanderer:VolumeList] %s: Icon Label Border Width changed - updating ..\n", __PRETTY_FUNCTION__));
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
                D(bug("[Wanderer:VolumeList] %s: Icon Label Border Height changed - updating ..\n", __PRETTY_FUNCTION__));
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
            D(bug("[Wanderer:VolumeList] %s: Unhandled change\n", __PRETTY_FUNCTION__));
            break;
        }

        if (options_changed)
        {
            if (!(prefs_Processing))
            {
                D(bug("[Wanderer:VolumeList] %s: IconList Options have changed, causing an update ..\n", __PRETTY_FUNCTION__));
                DoMethod(self, MUIM_IconList_Update);
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
MakeStaticHook(Hook_ProcessIconListPrefsFunc,IconWindowVolumeList__HookFunc_ProcessIconListPrefsFunc);
#endif
///

///IconWindowVolumeList__HookFunc_UpdateNetworkPrefsFunc()
#ifdef __AROS__
AROS_UFH3(
    void, IconWindowVolumeList__HookFunc_UpdateNetworkPrefsFunc,
    AROS_UFHA(struct Hook *,    hook,   A0),
    AROS_UFHA(APTR *,           obj,    A2),
    AROS_UFHA(APTR,             param,  A1)
)
{
#else
HOOKPROTO(IconWindowVolumeList__HookFunc_UpdateNetworkPrefsFunc, void, APTR *obj, APTR param)
{
#endif
    AROS_USERFUNC_INIT

    /* Get our private data */
    Object *self = ( Object *)obj;
    Object *prefs = NULL;
    Class *CLASS = *( Class **)param;

    SETUP_INST_DATA;

    D(bug("[Wanderer:VolumeList]: %s()\n", __PRETTY_FUNCTION__));

    GET(_app(self), MUIA_Wanderer_Prefs, &prefs);

    if (prefs)
    {
        BOOL    options_changed = FALSE;
        IPTR  prefs_Processing = 0;

        GET(prefs, MUIA_WandererPrefs_Processing, &prefs_Processing);

        IPTR   current_ShowNetwork = 0;
        IPTR   prefs_ShowNetwork = 0;

        D(bug("[Wanderer:VolumeList] %s: Setting ROOT view Network options ..\n", __PRETTY_FUNCTION__));

        GET(self, MUIA_IconWindowExt_NetworkBrowser_Show, &current_ShowNetwork);

        D(bug("[Wanderer:VolumeList] %s: Current = %d\n", __PRETTY_FUNCTION__, current_ShowNetwork));

        GET(prefs, MUIA_IconWindowExt_NetworkBrowser_Show, &prefs_ShowNetwork);

        D(bug("[Wanderer:VolumeList] %s: Prefs = %d\n", __PRETTY_FUNCTION__, prefs_ShowNetwork));

        if ((BOOL)current_ShowNetwork != (BOOL)prefs_ShowNetwork)
        {
            D(bug("[Wanderer:VolumeList] %s: ROOT view Network prefs changed - updating ..\n", __PRETTY_FUNCTION__));
            options_changed = TRUE;
            ((struct IconWindowVolumeList_DATA *)data)->iwvcd_ShowNetworkBrowser = prefs_ShowNetwork;
        }
        if ((options_changed) && !(prefs_Processing))
        {
            D(bug("[Wanderer:VolumeList] %s: Network prefs changed, causing an update ..\n", __PRETTY_FUNCTION__));
            DoMethod(self, MUIM_IconList_Update);
        }
        else if (data->iwcd_IconWindow)
        {
            SET(data->iwcd_IconWindow, MUIA_IconWindow_Changed, TRUE);
        }
    }
    AROS_USERFUNC_EXIT
}
#ifndef __AROS__
MakeStaticHook(Hook_UpdateNetworkPrefsFunc,IconWindowVolumeList__HookFunc_UpdateNetworkPrefsFunc);
#endif

#define BDRPLINELEN_MAX 1024
BOOL IconWindowVolumeList__Func_ParseBackdrop(Class *CLASS, Object *self, char *bdrp_dir)
{
    BPTR                bdrp_lock = (BPTR)NULL;
    char                *bdrp_file = NULL, *linebuf = NULL, *bdrp_fullfile = NULL, *bdrp_namepart = NULL;
    struct DiskObject   *bdrp_currfile_dob = NULL;
    BOOL                retVal = FALSE;

    if ((bdrp_dir == NULL) || (bdrp_dir[strlen(bdrp_dir) - 1] != ':'))
        return retVal;

    D(bug("[Wanderer:VolumeList] %s: Checking '%s' for .backdrop file .. \n", __PRETTY_FUNCTION__, bdrp_dir));

    if ((bdrp_file = AllocVec(strlen(bdrp_dir) + 9 + 1, MEMF_CLEAR|MEMF_PUBLIC)) != NULL)
    {
        sprintf(bdrp_file, "%s.backdrop", bdrp_dir);
        if ((bdrp_lock = Open(bdrp_file, MODE_OLDFILE)))
        {
            D(bug("[Wanderer:VolumeList] %s: Loading backdrop file: '%s'\n", __PRETTY_FUNCTION__, bdrp_file));

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

                        D(bug("[Wanderer:VolumeList] %s: LEAVEOUT Icon '%s' ('%s') DOB @ 0x%p\n", __PRETTY_FUNCTION__, bdrp_fullfile, bdrp_namepart, bdrp_currfile_dob));

                        if (bdrp_currfile_dob)
                        {
                            struct IconEntry *this_entry = NULL;
                            if ((this_entry = (struct IconEntry *)DoMethod(self, MUIM_IconList_CreateEntry, (IPTR)bdrp_fullfile, (IPTR)bdrp_namepart, (IPTR)NULL, (IPTR)bdrp_currfile_dob, 0)))
                            {
                                struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
                                if (fib)
                                {
                                    BPTR 				fib_lock = (BPTR)NULL;
                                    if ((fib_lock = Lock(bdrp_fullfile, SHARED_LOCK)) != NULL)
                                    {
                                        if (Examine(fib_lock, fib))
                                        {
                                            if (fib->fib_DirEntryType == ST_FILE)
                                            {
                                                this_entry->ie_IconListEntry.type = ST_LINKFILE;
                                                D(bug("[Wanderer:VolumeList] %s: LEAVEOUT ST_LINKFILE Entry @ 0x%p\n", __PRETTY_FUNCTION__, this_entry));
                                            }
                                            else if (fib->fib_DirEntryType == ST_USERDIR)
                                            {
                                                this_entry->ie_IconListEntry.type = ST_LINKDIR;
                                                D(bug("[Wanderer:VolumeList] %s: LEAVEOUT ST_LINKDIR Entry @ 0x%p\n", __PRETTY_FUNCTION__, this_entry));
                                            }
                                            else
                                            {
                                                D(bug("[Wanderer:VolumeList] %s: LEAVEOUT Unknown Entry Type @ 0x%p\n", __PRETTY_FUNCTION__, this_entry));
                                            }
					    DoMethod(self, MUIM_Family_AddTail, (struct Node*)&this_entry->ie_IconNode);
                                        }
                                        UnLock(fib_lock);
                                    }
                                    FreeDosObject(DOS_FIB, fib);
                                }
                                retVal = TRUE;
                            }
                        }
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
Object *IconWindowVolumeList__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    IPTR                            _newIconList__FSNotifyPort = 0;

    D(bug("[Wanderer:VolumeList]: %s()\n", __PRETTY_FUNCTION__));

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
        D(bug("[Wanderer:VolumeList] %s: SELF = 0x%p\n", __PRETTY_FUNCTION__, self));

#ifdef __AROS__
        data->iwcd_ProcessIconListPrefs_hook.h_Entry = ( HOOKFUNC )IconWindowVolumeList__HookFunc_ProcessIconListPrefsFunc;
#else
        data->iwcd_ProcessIconListPrefs_hook = &Hook_ProcessIconListPrefsFunc;
#endif
    }

    return self;
}
///

///OM_SET()
IPTR IconWindowVolumeList__OM_SET(Class *CLASS, Object *self, struct opSet *message)
{
    SETUP_INST_DATA;

    struct TagItem *tstate = message->ops_AttrList, *tag;

    while ((tag = NextTagItem((const struct TagItem**)&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Background:
            {
                D(bug("[Wanderer:VolumeList] %s: MUIA_Background\n", __PRETTY_FUNCTION__));
                break;
            }
        case MUIA_IconWindow_Window:
            {
                D(bug("[Wanderer:VolumeList] %s: MUIA_IconWindow_Window @ %p\n", __PRETTY_FUNCTION__, tag->ti_Data));
                data->iwcd_IconWindow = (Object *)tag->ti_Data;
                break;
            }
        case MUIA_IconList_BufferRastport:
            {
                D(bug("[Wanderer:VolumeList] %s: MUIA_IconList_BufferRastport @ %p\n", __PRETTY_FUNCTION__, tag->ti_Data));
                data->iwcd_RastPort = (struct RastPort *)tag->ti_Data;
                break;
            }
        }
    }
    return DoSuperMethodA(CLASS, self, (Msg) message);
}
///

///OM_GET()
IPTR IconWindowVolumeList__OM_GET(Class *CLASS, Object *self, struct opGet *message)
{
    //SETUP_INST_DATA;
    IPTR *store = message->opg_Storage;
    IPTR  rv    = TRUE;

    switch (message->opg_AttrID)
    {
    case MUIA_Version:
        *store = (IPTR)WIWVLVERS;
        break;

    case MUIA_Revision:
        *store = (IPTR)WIWVLREV;
        break;

    default:
        rv = DoSuperMethodA(CLASS, self, (Msg) message);
    }

    return rv;
}
///

///IconWindowVolumeList__MUIM_Setup()
IPTR IconWindowVolumeList__MUIM_Setup
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
        D(bug("[Wanderer:VolumeList] %s: Window Background = '%s'\n", __PRETTY_FUNCTION__, data->iwcd_ViewPrefs_ID));
        data->iwcd_ViewPrefs_NotificationObject = (Object *)DoMethod(prefs,
                                MUIM_WandererPrefs_ViewSettings_GetNotifyObject,
                                data->iwcd_ViewPrefs_ID);

        D(bug("[Wanderer:VolumeList] %s: Background Notification Obj @ 0x%p\n", __PRETTY_FUNCTION__, data->iwcd_ViewPrefs_NotificationObject));

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
  
    if (prefs)
    {
#ifdef __AROS__
        ((struct IconWindowVolumeList_DATA *)data)->iwvcd_UpdateNetworkPrefs_hook.h_Entry = ( HOOKFUNC )IconWindowVolumeList__HookFunc_UpdateNetworkPrefsFunc;
#else
        ((struct IconWindowVolumeList_DATA *)data)->iwvcd_UpdateNetworkPrefs_hook = &Hook_UpdateNetworkPrefsFunc;
#endif

        DoMethod
          (
            prefs, MUIM_Notify, MUIA_IconWindowExt_NetworkBrowser_Show, MUIV_EveryTime,
            (IPTR) self, 3, 
            MUIM_CallHook, &((struct IconWindowVolumeList_DATA *)data)->iwvcd_UpdateNetworkPrefs_hook, (IPTR)CLASS
          );
    }

    if (muiRenderInfo(self))
    {
        D(bug("[Wanderer:VolumeList] %s: Setting up EventHandler for (IDCMP_DISKINSERTED | IDCMP_DISKREMOVED)\n", __PRETTY_FUNCTION__));

        data->iwcd_EventHandlerNode.ehn_Priority = 1;
        data->iwcd_EventHandlerNode.ehn_Flags    = MUI_EHF_GUIMODE;
        data->iwcd_EventHandlerNode.ehn_Object   = self;
        data->iwcd_EventHandlerNode.ehn_Class    = CLASS;
        data->iwcd_EventHandlerNode.ehn_Events   = IDCMP_DISKINSERTED | IDCMP_DISKREMOVED;

        DoMethod(_win(self), MUIM_Window_AddEventHandler, &data->iwcd_EventHandlerNode);
    }
    else
    {
        D(bug("[Wanderer:VolumeList] %s: Couldnt add IDCMP EventHandler!\n", __PRETTY_FUNCTION__));
    }

    D(bug("[Wanderer:VolumeList] %s: Setup complete!\n", __PRETTY_FUNCTION__));
  
    return TRUE;
}
///

///IconWindowVolumeList__MUIM_Cleanup()
IPTR IconWindowVolumeList__MUIM_Cleanup
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;

    Object *prefs = NULL;

    D(bug("[Wanderer:VolumeList]: %s()\n", __PRETTY_FUNCTION__));
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

    if (prefs)
    {
        DoMethod
          (
            prefs,
            MUIM_KillNotifyObj, MUIA_IconWindowExt_NetworkBrowser_Show, (IPTR) self
          );
    }
    D(bug("[Wanderer:VolumeList] %s: Removing Disk Event Handler\n", __PRETTY_FUNCTION__));
    DoMethod(_win(self), MUIM_Window_RemEventHandler, &data->iwcd_EventHandlerNode);

    return DoSuperMethodA(CLASS, self, message);
}
///

///IconWindowVolumeList__MUIM_HandleEvent()
IPTR IconWindowVolumeList__MUIM_HandleEvent
(
    Class *CLASS, Object *self, struct MUIP_HandleEvent *message
)
{
    //SETUP_INST_DATA;

    struct IntuiMessage *imsg = message->imsg;

    D(bug("[Wanderer:VolumeList]: %s()\n", __PRETTY_FUNCTION__));

    if(imsg->Class == IDCMP_DISKINSERTED) 
    {
        D(bug("[Wanderer:VolumeList] %s: IDCMP_DISKINSERTED\n", __PRETTY_FUNCTION__));
        DoMethod(self, MUIM_IconList_Update);
        return(MUI_EventHandlerRC_Eat);
    }
    else if (imsg->Class == IDCMP_DISKREMOVED) 
    {
        D(bug("[Wanderer:VolumeList] %s: IDCMP_DISKREMOVED\n", __PRETTY_FUNCTION__));
        DoMethod(self, MUIM_IconList_Update);
        return(MUI_EventHandlerRC_Eat);
    }
    return 0;
}
///

///IconWindowVolumeList__MUIM_DrawBackground()
IPTR IconWindowVolumeList__MUIM_DrawBackground
(
  Class *CLASS, Object *self, struct MUIP_DrawBackground *message
)
{
    SETUP_INST_DATA;

    IPTR        retVal = (IPTR)TRUE;
    IPTR                clip = 0;
    struct RastPort           *DrawBackGround_RastPort;
    struct IconWindowBackFillMsg  DrawBackGround_BackFillMsg;

    D(bug("[Wanderer:VolumeList]: %s()\n", __PRETTY_FUNCTION__));

    if ((iconwindow_BackFill_Active == NULL) ||
      (data->iwcd_IconWindow == NULL))
    {
        D(bug("[Wanderer:VolumeList] %s: No Backfill support/Window not set .. causing parent class to render\n", __PRETTY_FUNCTION__));
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

    D(bug("[Wanderer:VolumeList] %s: RastPort @ 0x%p\n", __PRETTY_FUNCTION__, DrawBackGround_RastPort));
  
    if ((retVal = DoMethod(data->iwcd_IconWindow, MUIM_IconWindow_BackFill_DrawBackground, XGET(data->iwcd_IconWindow, MUIA_IconWindow_BackFillData), &DrawBackGround_BackFillMsg, DrawBackGround_RastPort)) == (IPTR)TRUE)
    {
        D(bug("[Wanderer:VolumeList] %s: Backfill module rendered background ..\n", __PRETTY_FUNCTION__));
        return retVal;
    }
    D(bug("[Wanderer:VolumeList] %s: Backfill module failed to render background ..\n", __PRETTY_FUNCTION__));

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

///IconWindowVolumeList__MUIM_IconList_Update()
IPTR IconWindowVolumeList__MUIM_IconList_Update
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

        D(bug("[Wanderer:VolumeList] %s: Causing parent to update\n", __PRETTY_FUNCTION__));
        retVal = DoSuperMethodA(CLASS, self, (Msg) message);

        D(bug("[Wanderer:VolumeList] %s: Check if we should show NetworkBrowser Icon ..\n", __PRETTY_FUNCTION__));

        GET(_app(self), MUIA_Wanderer_Prefs, &prefs);

        if (prefs)
        {
            struct IconWindowVolumeList_DATA *volumel_data = (struct IconWindowVolumeList_DATA *)data;

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

                D(bug("[Wanderer:VolumeList] %s: NetworkBrowser Icon DOB @ 0x%p\n", __PRETTY_FUNCTION__, _nb_dob));

                if (_nb_dob)
                {
                    struct Node *this_entry = NULL;
                    if ((this_entry = (struct Node *)DoMethod(self, MUIM_IconList_CreateEntry, (IPTR)"?wanderer.networkbrowse?", (IPTR)"Network Access..", (IPTR)NULL, (IPTR)_nb_dob, 0)))
                    {
                        this_entry->ln_Pri = 3;   /// Network Access gets Priority 3 so its displayed after special dirs
                        sort_list = TRUE;
                        D(bug("[Wanderer:VolumeList] %s: NetworkBrowser Icon Entry @ 0x%p\n", __PRETTY_FUNCTION__, this_entry));
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

                    D(bug("[Wanderer:VolumeList] %s: SYS/UserFilesLocation = '%s'\n", __PRETTY_FUNCTION__, __icwc_intern_TxtBuff));

                    if ((userfiles_path = AllocVec(strlen(__icwc_intern_TxtBuff) + 1, MEMF_CLEAR|MEMF_PUBLIC)) != NULL)
                    {
                        struct DiskObject    *_nb_dob = NULL;

                        volumel_data->iwvcd_UserFolderPath = userfiles_path;

                        D(bug("[Wanderer:VolumeList] %s: UserFilesLocation Path storage @ 0x%p\n", __PRETTY_FUNCTION__, userfiles_path));

                        strcpy(userfiles_path, __icwc_intern_TxtBuff);

                        D(bug("[Wanderer:VolumeList] %s: UserFilesLocation Path storage contains '%s'\n", __PRETTY_FUNCTION__, userfiles_path));

                        _nb_dob = GetIconTags
                          (
                            "ENV:SYS/def_UserHome", 
                            ICONGETA_FailIfUnavailable, FALSE,
                            ICONGETA_Label,             (IPTR)"User Files..",
                            TAG_DONE
                          );

                        D(bug("[Wanderer:VolumeList] %s: UserFiles Icon DOB @ 0x%p\n", __PRETTY_FUNCTION__, _nb_dob));

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


IPTR IconWindowVolumeList__MUIM_IconList_CreateEntry(struct IClass *CLASS, Object *obj, struct MUIP_IconList_CreateEntry *message)
{
    struct IconEntry  		*this_Icon = NULL;
    struct VolumeIcon_Private   *volPrivate = NULL;

    D(bug("[Wanderer:VolumeList]: %s()\n", __PRETTY_FUNCTION__));

    this_Icon = DoSuperMethodA(CLASS, obj, (Msg) message);

    if (this_Icon)
    {
        D(bug("[Wanderer:VolumeList] %s: IconEntry Allocated @ %p\n", __PRETTY_FUNCTION__, this_Icon));

        volPrivate = this_Icon->ie_IconListEntry.udata;

        if ((this_Icon->ie_IconListEntry.type == ST_ROOT) && (volPrivate && ((volPrivate->vip_FLags & (ICONENTRY_VOL_OFFLINE|ICONENTRY_VOL_DISABLED)) == 0)))
        {
            IconWindowVolumeList__Func_ParseBackdrop(CLASS, obj, this_Icon->ie_IconListEntry.label);
        }
    }
    return this_Icon;
}

IPTR IconWindowVolumeList__MUIM_IconList_UpdateEntry(struct IClass *CLASS, Object *obj, struct MUIP_IconList_UpdateEntry *message)
{
    struct IconEntry  		*this_Icon = NULL;
    struct VolumeIcon_Private   *volPrivate = NULL;

    D(bug("[Wanderer:VolumeList]: %s()\n", __PRETTY_FUNCTION__));

    volPrivate = message->icon->ie_IconListEntry.udata;

    this_Icon = DoSuperMethodA(CLASS, obj, (Msg) message);

    return this_Icon;
}

IPTR IconWindowVolumeList__MUIM_IconList_DestroyEntry(struct IClass *CLASS, Object *obj, struct MUIP_IconList_DestroyEntry *message)
{
    struct VolumeIcon_Private   *volPrivate = NULL;
    IPTR                        rv = NULL;

    D(bug("[Wanderer:VolumeList]: %s()\n", __PRETTY_FUNCTION__));

    volPrivate = message->icon->ie_IconListEntry.udata;

    rv = DoSuperMethodA(CLASS, obj, (Msg) message);
    
    return rv;
}

///
/*** Setup ******************************************************************/
#ifdef __AROS__
ICONWINDOWICONVOLUMELIST_CUSTOMCLASS
(
    IconWindowVolumeList, NULL, MUIC_IconVolumeList, NULL,
    OM_NEW,                     struct opSet *,
    OM_SET,                     struct opSet *,
    OM_GET,                     struct opGet *,
    MUIM_Setup,                 Msg,
    MUIM_Cleanup,               Msg,
    MUIM_DrawBackground,        Msg,
    MUIM_HandleEvent,           Msg,
    MUIM_IconList_Update,       struct MUIP_IconList_Update *,
    MUIM_IconList_CreateEntry,  struct MUIP_IconList_CreateEntry *,
    MUIM_IconList_UpdateEntry,  struct MUIP_IconList_UpdateEntry *,
    MUIM_IconList_DestroyEntry, struct MUIP_IconList_DestroyEntry *
);
#else
ICONWINDOWICONVOLUMELIST_CUSTOMCLASS
(
    IconWindowVolumeList, NULL,  NULL, IconVolumeList_Class,
    OM_NEW,                     struct opSet *,
    OM_SET,                     struct opSet *,
    OM_GET,                     struct opGet *,
    MUIM_Setup,                 Msg,
    MUIM_Cleanup,               Msg,
    MUIM_DrawBackground,        Msg,
    MUIM_HandleEvent,           Msg,
    MUIM_IconList_Update,       struct MUIP_IconList_Update *,
    MUIM_IconList_CreateEntry,  struct MUIP_IconList_CreateEntry *,
    MUIM_IconList_UpdateEntry,  struct MUIP_IconList_UpdateEntry *,
    MUIM_IconList_DestroyEntry, struct MUIP_IconList_DestroyEntry *
);
#endif
