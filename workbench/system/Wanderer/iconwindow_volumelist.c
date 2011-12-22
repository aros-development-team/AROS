/*
  Copyright  2004-2010, The AROS Development Team. All rights reserved.
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
#define WIWVLREV        2

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

    /* File System update handling */
    struct MsgPort              *iwvcd_FSNotifyPort;
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
        IPTR attrib_Current = 0, attrib_Prefs, prefs_Processing = 0;
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
        case MUIA_IconList_SortFlags:
            /* Generic code */
            GET(self, (ULONG)CHANGED_ATTRIB, &attrib_Current);

            if (((attrib_Prefs = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, (ULONG)CHANGED_ATTRIB)) != -1) &&
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
            D(bug("[Wanderer:VolumeList] %s: Unhandled change\n", __PRETTY_FUNCTION__));
            break;
        }

        if (options_changed)
        {
            if (!(prefs_Processing))
            {
                D(bug("[Wanderer:VolumeList] %s: IconList Options have changed, causing an update ..\n", __PRETTY_FUNCTION__));
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
MakeStaticHook(Hook_UpdateNetworkPrefsFunc,IconWindowVolumeList__HookFunc_UpdateNetworkPrefsFunc);
#endif

#define BDRPLINELEN_MAX 1024
BOOL IconWindowVolumeList__Func_ParseBackdrop(Object *self, struct IconEntry *bdrp_direntry, struct List* entryList)
{
    BPTR                bdrp_lock = (BPTR)NULL;
    char                *bdrp_file = NULL, *linebuf = NULL, *bdrp_fullfile = NULL, *bdrp_namepart = NULL;
    struct DiskObject   *bdrp_currfile_dob = NULL;
    BOOL                retVal = FALSE;
    char *bdrp_dir = bdrp_direntry->ie_IconNode.ln_Name;

    if ((bdrp_dir == NULL) || (bdrp_dir[strlen(bdrp_dir) - 1] != ':'))
        return retVal;

    D(bug("[Wanderer:VolumeList] %s('%s')\n", __PRETTY_FUNCTION__, bdrp_dir));

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
                        ULONG lofTYPE = 0;

                        sprintf(bdrp_fullfile, "%s%s", bdrp_dir, &linebuf[1]);
                        
                        struct FileInfoBlock        *lofFIB = AllocDosObject(DOS_FIB, NULL);
                        if (lofFIB)
                        {
                            BPTR                lofLock = BNULL;
                            if ((lofLock = Lock(bdrp_fullfile, SHARED_LOCK)) != BNULL)
                            {
                                char        *tmpbdrp_file = NULL;
                                int        tmpbdrp_len = strlen(bdrp_fullfile) + 128;
                                if ((tmpbdrp_file = AllocVec(tmpbdrp_len, MEMF_CLEAR|MEMF_PUBLIC)) != NULL)
                                {
                                    if (NameFromLock(lofLock, tmpbdrp_file, tmpbdrp_len) != 0)
                                    {
                                        FreeVec(bdrp_fullfile);
                                        bdrp_fullfile = tmpbdrp_file;
                                    }
                                    else
                                        FreeVec(tmpbdrp_file);
                                }
                                if (Examine(lofLock, lofFIB))
                                {
                                    if (lofFIB->fib_DirEntryType == ST_FILE)
                                    {
                                        lofTYPE = ST_LINKFILE;
                                    }
                                    else if (lofFIB->fib_DirEntryType == ST_USERDIR)
                                    {
                                        lofTYPE = ST_LINKDIR;
                                    }
                                }
                                UnLock(lofLock);
                            }
                            FreeDosObject(DOS_FIB, lofFIB);
                        }
                        
                        bdrp_namepart = FilePart(bdrp_fullfile);

                        struct IconEntry *this_entry = NULL, *iconNode = NULL, *tmpentry = NULL;

                        if (entryList != NULL)
                        {
                            D(bug("[Wanderer:VolumeList] %s: Checking for existing entry in list @ 0x%p\n", __PRETTY_FUNCTION__, entryList));
                            ForeachNodeSafe(entryList, iconNode, tmpentry)
                            {
                                if (strcmp(iconNode->ie_IconNode.ln_Name, bdrp_fullfile) == 0)
                                {
                                    this_entry = iconNode;
                                    Remove((struct Node*)&iconNode->ie_IconNode);
                                    iconNode->ie_IconListEntry.udata = bdrp_direntry;
                                    D(bug("[Wanderer:VolumeList] %s: Reinserting '%s'\n", __PRETTY_FUNCTION__, iconNode->ie_IconNode.ln_Name));
                                    DoMethod(self, MUIM_Family_AddTail, (struct Node*)&iconNode->ie_IconNode);
                                    /* retVal - this case is not considered a change */
                                    break;
                                }
                            }
                        }

                        if (this_entry == NULL)
                        {
                            //bug("[Wanderer:VolumeList] %s: Checking for existing entry in iconlist\n", __PRETTY_FUNCTION__);

                            bdrp_currfile_dob = GetIconTags
                              (
                                bdrp_fullfile, 
                                ICONGETA_Screen, _screen(self),
                                ICONGETA_FailIfUnavailable, FALSE,
                                ICONGETA_Label,             bdrp_namepart,
                                TAG_DONE
                              );

                            D(bug("[Wanderer:VolumeList] %s: LEAVEOUT Icon '%s' ('%s') DOB @ 0x%p\n", __PRETTY_FUNCTION__, bdrp_fullfile, bdrp_namepart, bdrp_currfile_dob));

                            if (bdrp_currfile_dob)
                            {
                                if ((this_entry = (struct IconEntry *)DoMethod(self, MUIM_IconList_CreateEntry, (IPTR)bdrp_fullfile, (IPTR)bdrp_namepart, (IPTR)NULL, (IPTR)bdrp_currfile_dob, 0)))
                                {
                                    this_entry->ie_IconNode.ln_Pri = 1;
                                    this_entry->ie_IconListEntry.type = lofTYPE;
                                    this_entry->ie_IconListEntry.udata = bdrp_direntry;
                                    DoMethod(self, MUIM_Family_AddTail, (struct Node*)&this_entry->ie_IconNode);

                                    retVal = TRUE;
                                }
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
    D(bug("[Wanderer:VolumeList]: %s()\n", __PRETTY_FUNCTION__));

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

        data->iwvcd_FSNotifyPort = (struct MsgPort *)GetTagData(MUIA_Wanderer_FileSysNotifyPort, (IPTR) NULL, message->ops_AttrList);

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
            data->iwcd_ViewPrefs_NotificationObject, MUIM_Notify, MUIA_IconList_SortFlags, MUIV_EveryTime,
            (IPTR) self, 3,
            MUIM_CallHook, &data->iwcd_ProcessIconListPrefs_hook, (IPTR)MUIA_IconList_SortFlags
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
            data->iwcd_ViewPrefs_NotificationObject, MUIM_KillNotifyObj, MUIA_IconList_SortFlags, (IPTR)self
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
        DoMethod(self, MUIM_IconList_Sort);
        return(MUI_EventHandlerRC_Eat);
    }
    else if (imsg->Class == IDCMP_DISKREMOVED) 
    {
        D(bug("[Wanderer:VolumeList] %s: IDCMP_DISKREMOVED\n", __PRETTY_FUNCTION__));
        DoMethod(self, MUIM_IconList_Update);
        DoMethod(self, MUIM_IconList_Sort);
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
        // struct IconList_Entry *icon_entry    = (IPTR)MUIV_IconList_NextIcon_Start;
        Object *prefs = NULL;
        struct Node *Obj_NetworkIcon = NULL;
        struct Node *Obj_UserFilesIcon = NULL;
        
        struct List                leftoutList, *iconList = NULL;
        struct IconEntry        *volentry = NULL, *entry = NULL, *tmpentry = NULL;

        NEWLIST(&leftoutList);
        
D(bug("[Wanderer:VolumeList] %s: left-out List @ %p\n", __PRETTY_FUNCTION__, &leftoutList));
        
        GET(self, MUIA_Family_List, &iconList);

        ForeachNodeSafe(iconList, entry, tmpentry)
        {
            if (entry->ie_IconListEntry.type == ST_ROOT)
            {
D(bug("[Wanderer:VolumeList] %s: Marking volume entry '%s' (pri = %d)\n", __PRETTY_FUNCTION__, entry->ie_IconNode.ln_Name, entry->ie_IconNode.ln_Pri));
                if (entry->ie_IconNode.ln_Pri == 5) entry->ie_IconNode.ln_Pri = -5;
                else entry->ie_IconNode.ln_Pri = -2;
            }
            if ((entry->ie_IconListEntry.type == ST_LINKFILE) || (entry->ie_IconListEntry.type == ST_LINKDIR))
            {
D(bug("[Wanderer:VolumeList] %s: Removing left out entry '%s'\n", __PRETTY_FUNCTION__, entry->ie_IconNode.ln_Name));
                Remove(&entry->ie_IconNode);
                AddTail(&leftoutList, &entry->ie_IconNode);
            }
            else if (strcmp(entry->ie_IconNode.ln_Name, "?wanderer.networkbrowse?") == 0)
            {
D(bug("[Wanderer:VolumeList] %s: Removing NetworkBrowser entry\n", __PRETTY_FUNCTION__));
                Remove(&entry->ie_IconNode);
                Obj_NetworkIcon = (struct Node *)entry;
            }
            /*else if (strcmp(, "User Files..") == 0)
            {
                Remove(&entry->ie_IconNode);
                Obj_UserFilesIcon = entry;
            }*/
        }

        D(bug("[Wanderer:VolumeList] %s: Causing parent to update\n", __PRETTY_FUNCTION__));
        retVal = DoSuperMethodA(CLASS, self, (Msg) message);

        GET(self, MUIA_Family_List, &iconList);

        ForeachNode(iconList, volentry)
        {
            if ((volentry->ie_IconListEntry.type == ST_ROOT)
                && ((volentry->ie_IconNode.ln_Pri == -2) || (volentry->ie_IconNode.ln_Pri == -5))
                && !(volentry->ie_IconListEntry.flags & ICONENTRY_VOL_OFFLINE))
            {
                if (volentry->ie_IconNode.ln_Pri == -5) volentry->ie_IconNode.ln_Pri = 5;
                else volentry->ie_IconNode.ln_Pri = 2;

                D(bug("[Wanderer:VolumeList] %s: Re-Parsing backdrop file for '%s'\n", __PRETTY_FUNCTION__, volentry->ie_IconNode.ln_Name));

                IconWindowVolumeList__Func_ParseBackdrop(self, volentry, &leftoutList);

                ForeachNodeSafe(&leftoutList, entry, tmpentry)
                {
                    if (((entry->ie_IconListEntry.type == ST_LINKFILE) || (entry->ie_IconListEntry.type == ST_LINKDIR))
                        && (entry->ie_IconListEntry.udata == volentry))
                    {
                        D(bug("[Wanderer:VolumeList] %s: Destroying orphaned left-out entry '%s'\n", __PRETTY_FUNCTION__, entry->ie_IconNode.ln_Name));
                        Remove(&entry->ie_IconNode);
                        DoMethod(self, MUIM_IconList_DestroyEntry, entry);
                    }
                }
            }
        }
        
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
                if (Obj_NetworkIcon == NULL)
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
                        if ((Obj_NetworkIcon = (struct Node *)DoMethod(self, MUIM_IconList_CreateEntry, (IPTR)"?wanderer.networkbrowse?", (IPTR)"Network Access..", (IPTR)NULL, (IPTR)_nb_dob, 0)))
                        {
                            Obj_NetworkIcon->ln_Pri = 4;   /// Network Access gets Priority 4 so its displayed after special dirs
//                            D(bug("[Wanderer:VolumeList] %s: NetworkBrowser Icon Entry @ 0x%p\n", __PRETTY_FUNCTION__, this_entry));
                        }
                    }
                }
            }
            else
            {
                if (Obj_NetworkIcon != NULL)
                {
                    DoMethod(self, MUIM_IconList_DestroyEntry, Obj_NetworkIcon);
                }
            }

            GET(prefs, MUIA_IconWindowExt_UserFiles_ShowFilesFolder, &volumel_data->iwvcd_ShowUserFolder);

#if defined(DEBUG_SHOWUSERFILES)
            volumel_data->iwvcd_ShowUserFolder = TRUE;
#endif
            if (volumel_data->iwvcd_ShowUserFolder)
            {
                if (Obj_UserFilesIcon == NULL)
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
                                if ((Obj_UserFilesIcon = (struct Node *)DoMethod(self, MUIM_IconList_CreateEntry, userfiles_path, (IPTR)"User Files..", (IPTR)NULL, (IPTR)_nb_dob, 0)))
                                {
                                    Obj_UserFilesIcon->ln_Pri = 5;   /// Special dirs get Priority 5
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                if (Obj_UserFilesIcon != NULL)
                {
                    DoMethod(self, MUIM_IconList_DestroyEntry, Obj_UserFilesIcon);
                }
            }
        }
    }
    else
    {
        retVal = TRUE;
        DoMethod(self, MUIM_IconList_Clear);
    }

    return retVal;
}

/* Notes:
 * This handle only changes to .backdrop file. In theory we could just add notification on this file instead of whole root directory
 * but this file is not guaranteed to exist - meaning that leavout dune during "first" session would not be visible until reboot
 */
IPTR IconWindowVolumeList__HandleFSUpdate(Object *target, struct NotifyMessage *msg)
{
    struct List         *iconList = NULL, fsLeftOutList;
    struct IconEntry    *entry = NULL, *tmpEntry = NULL, *fsEntry = NULL;
    BOOL                changed = FALSE;

    D(bug("[Wanderer:VolumeList]: %s(NotifyMessage @ %p -> '%s')\n", __PRETTY_FUNCTION__, msg, msg->nm_NReq->nr_Name));

    NEWLIST(&fsLeftOutList);
    
    D(bug("[Wanderer:VolumeList] %s: IconWindowVolumeList, IconList @ %p\n", __PRETTY_FUNCTION__, target));

    GET(target, MUIA_Family_List, &iconList);
    /* Find out which icon matches the volument that notified us */
    ForeachNode(iconList, entry)
    {
        if ((entry->ie_IconListEntry.type == ST_ROOT)
            && (strncmp(entry->ie_IconNode.ln_Name, msg->nm_NReq->nr_Name, strlen(entry->ie_IconNode.ln_Name)) == 0))
        {
            fsEntry = entry;
            break;
        }
    }
    
    if (fsEntry != NULL)
    {
        D(bug("[Wanderer:VolumeList] %s: Processing .backdrop for entry @ %p '%s'\n", __PRETTY_FUNCTION__, fsEntry, fsEntry->ie_IconNode.ln_Name));

        /* Find other icons that are linked the the root icon */
        ForeachNodeSafe(iconList, entry,tmpEntry)
        {
            if (((entry->ie_IconListEntry.type == ST_LINKFILE) || (entry->ie_IconListEntry.type == ST_LINKDIR)) && (entry->ie_IconListEntry.udata == fsEntry))
            {
                D(bug("[Wanderer:VolumeList] %s: existing left-out entry @ %p '%s' for this volume\n", __PRETTY_FUNCTION__, entry, entry->ie_IconNode.ln_Name));
                Remove(&entry->ie_IconNode);
                AddTail(&fsLeftOutList, &entry->ie_IconNode);
            }
        }

        /* Parse .backdrop and add any of the existing icons back to family list */
        changed = IconWindowVolumeList__Func_ParseBackdrop(target, fsEntry, &fsLeftOutList);

        /* Destroy left-out entries which are no longer valid */
        ForeachNodeSafe(&fsLeftOutList, entry, tmpEntry)
        {
            D(bug("[Wanderer:VolumeList] %s: Destroying orphaned entry @ %p '%s'\n", __PRETTY_FUNCTION__, entry, entry->ie_IconNode.ln_Name));
            Remove(&entry->ie_IconNode);
            DoMethod(target, MUIM_IconList_DestroyEntry, entry);
            changed = TRUE;
        }

        /* Re-sort the list */
        if (changed)
            DoMethod(target, MUIM_IconList_Sort);
    }
    
    return 0;
}

IPTR IconWindowVolumeList__MUIM_IconList_CreateEntry(struct IClass *CLASS, Object *self, struct MUIP_IconList_CreateEntry *message)
{
    struct IconEntry                *this_Icon = NULL;
    struct VolumeIcon_Private       *volPrivate = NULL;
    struct Wanderer_FSHandler       *_volumeIcon__FSNotifyHandler = NULL;

    D(bug("[Wanderer:VolumeList]: %s()\n", __PRETTY_FUNCTION__));

    this_Icon = (struct IconEntry *)DoSuperMethodA(CLASS, self, (Msg) message);

    if (this_Icon)
    {
        volPrivate = this_Icon->ie_IconListEntry.udata;

        D(bug("[Wanderer:VolumeList] %s: IconEntry '%s' Allocated @ %p, volPrivate @ %p\n", __PRETTY_FUNCTION__, this_Icon->ie_IconNode.ln_Name, this_Icon, volPrivate));

        if ((this_Icon->ie_IconListEntry.type == ST_ROOT) && (volPrivate && ((volPrivate->vip_FLags & (ICONENTRY_VOL_OFFLINE|ICONENTRY_VOL_DISABLED)) == 0)))
        {
            if (((_volumeIcon__FSNotifyHandler = AllocMem(sizeof(struct Wanderer_FSHandler), MEMF_CLEAR)) != NULL))
            {
                SETUP_INST_DATA;

                _volumeIcon__FSNotifyHandler->target                        = self;
                _volumeIcon__FSNotifyHandler->HandleFSUpdate                = IconWindowVolumeList__HandleFSUpdate;
                volPrivate->vip_FSNotifyRequest.nr_Name                     = this_Icon->ie_IconNode.ln_Name;
                volPrivate->vip_FSNotifyRequest.nr_Flags                    = NRF_SEND_MESSAGE;
                volPrivate->vip_FSNotifyRequest.nr_stuff.nr_Msg.nr_Port     = data->iwvcd_FSNotifyPort;
                volPrivate->vip_FSNotifyRequest.nr_UserData                 = (IPTR)_volumeIcon__FSNotifyHandler;
                _volumeIcon__FSNotifyHandler->fshn_Node.ln_Name             = volPrivate->vip_FSNotifyRequest.nr_Name;

                if (StartNotify(&volPrivate->vip_FSNotifyRequest))
                {
                    D(bug("[Wanderer:VolumeList] %s: FSNotification setup", __PRETTY_FUNCTION__));
                }
                else
                {
                    D(bug("[Wanderer:VolumeList] %s: FAILED to setup FSNotification", __PRETTY_FUNCTION__));
                    FreeMem(_volumeIcon__FSNotifyHandler, sizeof(struct Wanderer_FSHandler));
                    volPrivate->vip_FSNotifyRequest.nr_Name = NULL;
                    volPrivate->vip_FSNotifyRequest.nr_UserData = (IPTR)NULL;
                 }
                D(bug(" for Volume '%s'\n", this_Icon->ie_IconNode.ln_Name));
            }
            IconWindowVolumeList__Func_ParseBackdrop(self, this_Icon, NULL);
        }
    }
    return (IPTR)this_Icon;
}

IPTR IconWindowVolumeList__MUIM_IconList_UpdateEntry(struct IClass *CLASS, Object *obj, struct MUIP_IconList_UpdateEntry *message)
{
    struct VolumeIcon_Private   *volPrivate = NULL;
    struct IconEntry                  *this_Icon = NULL;

    D(bug("[Wanderer:VolumeList]: %s()\n", __PRETTY_FUNCTION__));

    volPrivate = message->entry->ie_IconListEntry.udata;

    if (message->entry->ie_IconListEntry.type == ST_ROOT
        && (message->entry->ie_IconListEntry.flags &
        (ICONENTRY_VOL_OFFLINE|ICONENTRY_VOL_DISABLED)) != 0)
    {
        if (volPrivate->vip_FSNotifyRequest.nr_Name != NULL)
        {
            EndNotify(&volPrivate->vip_FSNotifyRequest);

            FreeMem((struct Wanderer_FSHandler *)volPrivate->vip_FSNotifyRequest.nr_UserData, sizeof(struct Wanderer_FSHandler));
            volPrivate->vip_FSNotifyRequest.nr_Name = NULL;
            volPrivate->vip_FSNotifyRequest.nr_UserData = (IPTR)NULL;
        }
    }

    this_Icon = (struct IconEntry *)DoSuperMethodA(CLASS, obj, (Msg) message);

    return (IPTR)this_Icon;
}

IPTR IconWindowVolumeList__MUIM_IconList_DestroyEntry(struct IClass *CLASS, Object *obj, struct MUIP_IconList_DestroyEntry *message)
{
    struct VolumeIcon_Private   *volPrivate = NULL;
    IPTR                        rv = 0;

    D(bug("[Wanderer:VolumeList]: %s()\n", __PRETTY_FUNCTION__));

    volPrivate = message->entry->ie_IconListEntry.udata;

    if ((message->entry->ie_IconListEntry.type == ST_ROOT))
    {
        if (volPrivate->vip_FSNotifyRequest.nr_Name != NULL)
        {
            EndNotify(&volPrivate->vip_FSNotifyRequest);
            FreeMem((struct Wanderer_FSHandler *)volPrivate->vip_FSNotifyRequest.nr_UserData, sizeof(struct Wanderer_FSHandler));
            volPrivate->vip_FSNotifyRequest.nr_Name = NULL;
            volPrivate->vip_FSNotifyRequest.nr_UserData = (IPTR)NULL;
        }

        // Remove all the icons left out for this volume ..
        struct List                *iconList = NULL;
        struct IconEntry        *entry = NULL;

        GET(obj, MUIA_Family_List, &iconList);
        ForeachNode(iconList, entry)
        {
            if (((entry->ie_IconListEntry.type == ST_LINKFILE) || (entry->ie_IconListEntry.type == ST_LINKDIR)) && (entry->ie_IconListEntry.udata == message->entry))
            {
                D(bug("[Wanderer:VolumeList] %s: Removing child entry '%s'\n", __PRETTY_FUNCTION__, entry->ie_IconNode.ln_Name));
                DoMethod(obj, MUIM_IconList_DestroyEntry, entry);
            }
        }
    }
    else if ((message->entry->ie_IconListEntry.type == ST_LINKFILE) || (message->entry->ie_IconListEntry.type == ST_LINKDIR))
        message->entry->ie_IconListEntry.udata = NULL;

    D(bug("[Wanderer:VolumeList] %s: causing parent class to dispose of '%s'\n", __PRETTY_FUNCTION__, message->entry->ie_IconNode.ln_Name));
    
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
    MUIM_DrawBackground,        struct MUIP_DrawBackground *,
    MUIM_HandleEvent,           struct MUIP_HandleEvent *,
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
