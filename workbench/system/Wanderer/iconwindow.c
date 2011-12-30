/*
  Copyright © 2004-2011, The AROS Development Team. All rights reserved.
  $Id$
*/

#define DEBUG 0
#define DOPENWINDOW(x) /* Window positioning and size */

#define ZCC_QUIET

#include "portable_macros.h"
#ifdef __AROS__
#define MUIMASTER_YES_INLINE_STDARG
#endif

#define ICONWINDOW_NODETAILVIEWCLASS
//#define ICONWINDOW_BUFFERLIST

#include <exec/types.h>
#include <libraries/mui.h>

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
#include <aros/debug.h>
#include <clib/alib_protos.h>
#include <prefs/wanderer.h>
#include <zune/customclasses.h>
#else
#include <prefs_AROS/wanderer.h>
#include <zune_AROS/customclasses.h>
#endif

#if defined(__AMIGA__) && !defined(__PPC__)
#define NO_INLINE_STDARG
#endif
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "Classes/iconlist.h"
#include "Classes/iconlistview.h"
#include "Classes/iconlist_attributes.h"
#include "Classes/icon_attributes.h"
#include "wanderer.h"
#include "wandererprefs.h"

#include "iconwindow.h"
#include "iconwindow_attributes.h"
#include "iconwindow_iconlist.h"
#include "iconwindowbackfill.h"

#ifndef __AROS__

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

#if defined(ICONWINDOW_NODETAILVIEWCLASS)
struct MUI_CustomClass *IconWindowDetailDrawerList_CLASS;
#endif

#define WIWVERS       1
#define WIWREV        0

#ifdef __AROS__
#define DoSuperNew(cl, obj, ...) DoSuperNewTags(cl, obj, NULL, __VA_ARGS__)
#else
#define IconListviewObject NewObject(IconListview_Class->mcc_Class
#endif

/*** Private Global Data *********************************************************/

static struct List                      iconwindow_BackFillNodes;
struct List                             iconwindow_Extensions;
struct IconWindow_BackFill_Descriptor  *iconwindow_BackFill_Active;

static char __intern_wintitle_wanderer[] = "Wanderer";

#define TT_WNDRRSRT     "WNDRRSRT"
#define TT_COUNT        1

/*** Helper functions ***********************************************************/
STATIC VOID IconWindow_RestoreSettings(struct DiskObject * dskobj, Object * iconwindowiconlist, BOOL forceshowall)
{
    if (dskobj->do_ToolTypes)
    {
        STRPTR hexFlags = FindToolType(dskobj->do_ToolTypes, TT_WNDRRSRT);
        if (hexFlags != NULL)
        {
            ULONG sortFlags = strtol(hexFlags, NULL, 16);
            SET(iconwindowiconlist, MUIA_IconWindowIconList_RestoredSortFlags, (ULONG)sortFlags);
        }
    }

    if (dskobj->do_Gadget.UserData)
    {
        D(bug("[Wanderer:IconWindow] %s: Drawer Icon has OS 2.x/3.x data: FLAGS %x\n", __PRETTY_FUNCTION__, dskobj->do_DrawerData->dd_Flags));

        if (dskobj->do_DrawerData->dd_Flags == 1)
            SET(iconwindowiconlist, MUIA_IconWindowIconList_RestoredDisplayFlags, ICONLIST_DISP_SHOWINFO);
        if (dskobj->do_DrawerData->dd_Flags == 2)
            SET(iconwindowiconlist, MUIA_IconWindowIconList_RestoredDisplayFlags, 0);
    }

    if (forceshowall)
        SET(iconwindowiconlist, MUIA_IconWindowIconList_RestoredDisplayFlags, 0);
}

STATIC VOID IconWindow_StoreSettings(Object * iconwindow)
{
    Object              *iconList = (Object *) XGET(iconwindow, MUIA_IconWindow_IconList);
    STRPTR              dir_name = (char *)XGET(iconwindow, MUIA_IconWindow_Location);
    struct DiskObject   *drawericon = NULL;
    IPTR                geticon_error = 0;
    IPTR                display_bits = 0, sort_bits = 0;

    drawericon = GetIconTags(dir_name,
                            ICONGETA_FailIfUnavailable, FALSE,
                            ICONA_ErrorCode, &geticon_error,
                            TAG_DONE);

    if (drawericon != NULL)
    {
        if (drawericon->do_DrawerData == NULL)
        {
            D(bug("[Wanderer:IconWindow] %s: Icon for '%s' has no DRAWER data!\n", __PRETTY_FUNCTION__, dir_name));
            drawericon->do_DrawerData = AllocMem(sizeof(struct DrawerData), MEMF_CLEAR|MEMF_PUBLIC);
        }

        drawericon->do_Gadget.UserData = (APTR)1;

        drawericon->do_DrawerData->dd_NewWindow.TopEdge = XGET(iconwindow, MUIA_Window_TopEdge);
        drawericon->do_DrawerData->dd_NewWindow.LeftEdge = XGET(iconwindow, MUIA_Window_LeftEdge);
        drawericon->do_DrawerData->dd_NewWindow.Width = XGET(iconwindow, MUIA_Window_Width);
        drawericon->do_DrawerData->dd_NewWindow.Height = XGET(iconwindow, MUIA_Window_Height);

        GET(iconList, MUIA_IconList_DisplayFlags, &display_bits);
        if (display_bits & ICONLIST_DISP_SHOWINFO)
        {
            D(bug("[Wanderer:IconWindow] %s: ICONLIST_DISP_SHOWINFO\n", __PRETTY_FUNCTION__));
            drawericon->do_DrawerData->dd_Flags = DDFLAGS_SHOWICONS;
        }
        else
        {
            drawericon->do_DrawerData->dd_Flags = DDFLAGS_SHOWALL;
        }

        drawericon->do_DrawerData->dd_ViewModes = DDVM_BYICON;

        /* Save settings into Tool Types */
        {
            LONG i = 0, inew = 0, tocopy = 0;
            TEXT buffer[128] = {0};
            UBYTE ** oldtooltypes = drawericon->do_ToolTypes;
            UBYTE ** newtooltypes = NULL;

            /* Find how many need to be copied */
            if (oldtooltypes != NULL)
            {
                for (i = 0; oldtooltypes[i]; i++)
                {
                    if (Strnicmp(oldtooltypes[i], TT_WNDRRSRT, strlen(TT_WNDRRSRT)) == 0)
                        continue;
                    tocopy++;
                }
            }

            /* Create new pointer array */
            newtooltypes = AllocVec(sizeof(STRPTR) * (tocopy + TT_COUNT + 1), MEMF_CLEAR);

            /* Copy, inew will have last index value */
            if (oldtooltypes != NULL)
            {
                for (i = 0, inew = 0; oldtooltypes[i]; i++)
                {
                    if (Strnicmp(oldtooltypes[i], TT_WNDRRSRT, strlen(TT_WNDRRSRT)) == 0)
                        continue;
                    newtooltypes[inew++] = StrDup(oldtooltypes[i]);
                }
            }

            /* Add TT_WNDRRSRT value */
            GET(iconList, MUIA_IconList_SortFlags, &sort_bits);
            __sprintf(buffer, TT_WNDRRSRT"=0x%lx", sort_bits);
            newtooltypes[inew++] = StrDup(buffer);

            drawericon->do_ToolTypes = newtooltypes;

            PutDiskObject(dir_name, drawericon);

            drawericon->do_ToolTypes = oldtooltypes;

            /* Free memory */
            for (i = 0; i < inew; i++)
                FreeVec(newtooltypes[i]);
            FreeVec(newtooltypes);
        }
    }
}


/*** Hook functions *********************************************************/

///IconWindow__HookFunc_PrefsUpdatedFunc()
#ifdef __AROS__
AROS_UFH3(
    void, IconWindow__HookFunc_PrefsUpdatedFunc,
    AROS_UFHA(struct Hook *,    hook,   A0),
    AROS_UFHA(APTR *,           obj,    A2),
    AROS_UFHA(APTR,             param,  A1)
)
{
#else
HOOKPROTO(IconWindow__HookFunc_PrefsUpdatedFunc, void, APTR *obj, APTR param)
{
#endif
    AROS_USERFUNC_INIT
  
    /* Get our private data */
    Object *self = ( Object *)obj;
    Class *CLASS = *( Class **)param;
    IPTR changed_state;

    SETUP_ICONWINDOW_INST_DATA;

    D(bug("[Wanderer:IconWindow]: %s()\n", __PRETTY_FUNCTION__));

    changed_state = 0;
    GET(self, MUIA_IconWindow_Changed, &changed_state);

    if ((changed_state) && (data->iwd_IconListObj))
    {
        D(bug("[Wanderer:IconWindow] %s: Window contents have changed .. updating display ..\n", __PRETTY_FUNCTION__));
        DoMethod(data->iwd_IconListObj, MUIM_IconList_Update);
        DoMethod(data->iwd_IconListObj, MUIM_IconList_Sort);
        SET(self, MUIA_IconWindow_Changed, FALSE);
    }

    AROS_USERFUNC_EXIT
}
#ifndef __AROS__
MakeStaticHook(iwd_PrefsUpdated_hook,IconWindow__HookFunc_PrefsUpdatedFunc);
#endif
///

///IconWindow__HookFunc_ProcessBackgroundFunc()
#ifdef __AROS__
AROS_UFH3(
    void, IconWindow__HookFunc_ProcessBackgroundFunc,
    AROS_UFHA(struct Hook *,    hook,   A0),
    AROS_UFHA(APTR *,           obj,    A2),
    AROS_UFHA(APTR,             param,  A1)
)
{
#else
HOOKPROTO(IconWindow__HookFunc_ProcessBackgroundFunc, void, APTR *obj, APTR param)
{
#endif
    AROS_USERFUNC_INIT
  
    /* Get our private data */
    Object      *self = ( Object *)obj,
                *prefs = NULL;
    Class *CLASS = *( Class **)param;

    SETUP_ICONWINDOW_INST_DATA;

    D(bug("[Wanderer:IconWindow]: %s()\n", __PRETTY_FUNCTION__));

    DoMethod(self, MUIM_IconWindow_BackFill_ProcessBackground, data->iwd_BackFillInfo, data->iwd_RootViewObj);

    GET(_app(self), MUIA_Wanderer_Prefs, &prefs);

    if (prefs)
    {
        //BOOL    options_changed = FALSE;
        IPTR  prefs_Processing = 0;

        GET(prefs, MUIA_WandererPrefs_Processing, &prefs_Processing);
        if (!prefs_Processing)
        {
            /* TODO: We arent in prefs-processing so cause an update! */
        }
    }

    AROS_USERFUNC_EXIT
}
#ifndef __AROS__
MakeStaticHook(iwd_ProcessBackground_hook,IconWindow__HookFunc_ProcessBackgroundFunc);
#endif
///

///IconWindow__HookFunc_WandererBackFillFunc()
#ifdef __AROS__
AROS_UFH3(
    void, IconWindow__HookFunc_WandererBackFillFunc,
    AROS_UFHA(struct Hook *,        hook,   A0),
    AROS_UFHA(struct RastPort *,    RP,    A2),
    AROS_UFHA(struct BackFillMsg *, BFM,  A1)
)
{
#else
HOOKPROTO(IconWindow__HookFunc_WandererBackFillFunc, void, struct RastPort *RP, struct BackFillMsg *BFM)
{
#endif
    AROS_USERFUNC_INIT
  
    struct IconWindow_BackFillHookData *HookData = NULL;

D(bug("[Wanderer:IconWindow]: %s()\n", __PRETTY_FUNCTION__));
    
    if ((HookData = hook->h_Data) && (iconwindow_BackFill_Active != NULL))
    {
        Class                              *CLASS = HookData->bfhd_IWClass;
        Object                 *self = HookData->bfhd_IWObject;

        SETUP_ICONWINDOW_INST_DATA;

        struct RastPort           *DrawBackGround_RastPort = NULL;
        struct IconWindowBackFillMsg  DrawBackGround_BackFillMsg;

        GET(data->iwd_IconListObj, MUIA_IconList_BufferRastport, &DrawBackGround_RastPort);

        if (DrawBackGround_RastPort != NULL)
        {
            if ((DrawBackGround_BackFillMsg.Layer = DrawBackGround_RastPort->Layer) == NULL)
            {
  D(bug("[Wanderer:IconWindow] %s: Rastport Layer = NULL!\n", __PRETTY_FUNCTION__));
            }

            GET(data->iwd_IconListObj,MUIA_IconList_BufferLeft, &DrawBackGround_BackFillMsg.AreaBounds.MinX);
            GET(data->iwd_IconListObj,MUIA_IconList_BufferTop, &DrawBackGround_BackFillMsg.AreaBounds.MinY);
            GET(data->iwd_IconListObj,MUIA_IconList_BufferWidth, &DrawBackGround_BackFillMsg.AreaBounds.MaxX);
            GET(data->iwd_IconListObj,MUIA_IconList_BufferHeight, &DrawBackGround_BackFillMsg.AreaBounds.MaxY);

            DrawBackGround_BackFillMsg.AreaBounds.MaxX += DrawBackGround_BackFillMsg.AreaBounds.MinX - 1;
            DrawBackGround_BackFillMsg.AreaBounds.MaxY += DrawBackGround_BackFillMsg.AreaBounds.MinY - 1;

            DrawBackGround_BackFillMsg.DrawBounds.MinX = BFM->Bounds.MinX;
            DrawBackGround_BackFillMsg.DrawBounds.MinY = BFM->Bounds.MinY;
            DrawBackGround_BackFillMsg.DrawBounds.MaxX = BFM->Bounds.MaxX;
            DrawBackGround_BackFillMsg.DrawBounds.MaxY = BFM->Bounds.MaxY;

            /* Offset into source image (ala scroll bar position) */
            DrawBackGround_BackFillMsg.OffsetX = BFM->OffsetX;
            DrawBackGround_BackFillMsg.OffsetY = BFM->OffsetY;
            DoMethod(self, MUIM_IconWindow_BackFill_DrawBackground, data->iwd_BackFillInfo, &DrawBackGround_BackFillMsg, DrawBackGround_RastPort);
        }

    }

    AROS_USERFUNC_EXIT
}
#ifndef __AROS__
MakeStaticHook(Hook_WandererBackFillFunc,IconWindow__HookFunc_WandererBackFillFunc);
#endif
///

///OM_NEW()
Object *IconWindow__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    struct iconWindow_Extension *iw_Extension = NULL;

    struct Screen               *_newIconWin__Screen = NULL;
    Object                      *_newIconWin__IconListObj = NULL,
                                *_newIconWin__RootViewObj = NULL,

                                *_newIconWin__TopPanelContainerObj = NULL,
                                *_newIconWin__TopPanelRootGroupObj = NULL,
                                *_newIconWin__TopPanelSpacerObj = NULL,

                                *_newIconWin__LeftPanelContainerObj = NULL,
                                *_newIconWin__LeftPanelRootGroupObj = NULL,
                                *_newIconWin__LeftPanelSpacerObj = NULL,

                                *_newIconWin__BottomPanelContainerObj = NULL,
                                *_newIconWin__BottomPanelRootGroupObj = NULL,
                                *_newIconWin__BottomPanelSpacerObj = NULL,

                                *prefs = NULL;

    char                        *_newIconWin__Title = NULL;

    UBYTE                       _newIconWin__VOLVIEWMODE = MUIV_IconWindow_VolumeInfoMode_ShowAllIfNoInfo;

    BOOL                        isRoot = FALSE,
                                isBackdrop = FALSE;

    struct Hook                 *actionHook = NULL;

    struct Hook                 *_newIconWin__BackFillHook = NULL;

    IPTR                        WindowBF_TAG = (IPTR)TAG_IGNORE;

    IPTR                        _newIconWin__WindowWidth = 0;
    IPTR                        _newIconWin__WindowHeight = 0;
    IPTR                        _newIconWin__WindowLeft = 0;
    IPTR                        _newIconWin__WindowTop = 0;

    IPTR                        _newIconWin__FSNotifyPort = (IPTR)NULL;
    struct MUI_CustomClass      *iconviewclass = NULL;
#if defined(ICONWINDOW_NODETAILVIEWCLASS)
    IconWindowDetailDrawerList_CLASS = IconWindowDrawerList_CLASS;
#endif

D(bug("[Wanderer:IconWindow]: %s()\n", __PRETTY_FUNCTION__));

    /* More than one GetTagData is not very efficient, however since this isn't called very often... */
    isBackdrop = (BOOL)GetTagData(MUIA_IconWindow_IsBackdrop, (IPTR)FALSE, message->ops_AttrList);

    isRoot = (BOOL)GetTagData(MUIA_IconWindow_IsRoot, (IPTR)FALSE, message->ops_AttrList);

    actionHook = (struct Hook *)GetTagData(MUIA_IconWindow_ActionHook, (IPTR) NULL, message->ops_AttrList);
    prefs = (Object *)GetTagData(MUIA_Wanderer_Prefs, (IPTR) NULL, message->ops_AttrList);

    _newIconWin__FSNotifyPort = (IPTR)GetTagData(MUIA_Wanderer_FileSysNotifyPort, (IPTR) NULL, message->ops_AttrList);

    /* Request the screen we should use .. */
    if (!(_newIconWin__Screen = (struct Screen *)GetTagData(MUIA_Wanderer_Screen, (IPTR) NULL, message->ops_AttrList)))
    {
            D(bug("[Wanderer:IconWindow] %s: NO SCREEN SET!\n", __PRETTY_FUNCTION__));
            return NULL;
    }
D(bug("[Wanderer:IconWindow] %s: Screen @ 0x%p\n", __PRETTY_FUNCTION__, _newIconWin__Screen));

    if ((_newIconWin__BackFillHook = AllocVec(sizeof(struct Hook), MEMF_CLEAR|MEMF_PUBLIC))!=NULL)
    {
D(bug("[Wanderer:IconWindow] %s: Allocated WindowBackFillHook @ 0x%p\n", __PRETTY_FUNCTION__, _newIconWin__BackFillHook));

#ifdef __AROS__
            _newIconWin__BackFillHook->h_Entry = ( HOOKFUNC )IconWindow__HookFunc_WandererBackFillFunc;
#else
            _newIconWin__BackFillHook = &Hook_WandererBackFillFunc;
#endif

//#if defined(__MORPHOS__)
//        WindowBF_TAG = MUIA_Window_BackFillHook;
//#else
        WindowBF_TAG = WA_BackFill;
//#endif
    }

    if (isRoot)
    {
        iconviewclass = IconWindowVolumeList_CLASS;
        _newIconWin__IconListObj = (Object *)NewObject(iconviewclass->mcc_Class, NULL,
                                                    MUIA_Wanderer_FileSysNotifyPort, _newIconWin__FSNotifyPort,
                                                    TAG_DONE);

        /*
         * Use MUI built-in support to keep window aligned to size of screen
         * Remember: sizes of screen can change without window beeing recreated
         */
        _newIconWin__WindowWidth = MUIV_Window_Width_Screen(100);
        _newIconWin__WindowHeight = MUIV_Window_Height_Screen(100);
        _newIconWin__WindowTop = MUIV_Window_TopEdge_Delta(0);
        _newIconWin__WindowLeft = 0;

        DOPENWINDOW(bug("[Wanderer:IconWindow] %s: Screen dimensions ..  %ld x %ld\n", __PRETTY_FUNCTION__, _newIconWin__WindowWidth, _newIconWin__WindowHeight));

        if (isBackdrop)
        {
            DOPENWINDOW(bug("[Wanderer:IconWindow] %s: BACKDROP ROOT Window\n", __PRETTY_FUNCTION__));
            _newIconWin__Title = NULL;
        }
        else
        {
            DOPENWINDOW(bug("[Wanderer:IconWindow] %s: Plain ROOT Window\n", __PRETTY_FUNCTION__));
            _newIconWin__Title = __intern_wintitle_wanderer;
        }
    }
    else
    {
        struct DiskObject   *drawericon = NULL;
        IPTR                geticon_error = 0, geticon_isdefault = 0;
        IPTR                _newIconWin__TitleLen = 0;
        BOOL                forceshowall = FALSE;
        BOOL                isVolume;

        _newIconWin__WindowTop = MUIV_Window_TopEdge_Centered;
        _newIconWin__WindowLeft = MUIV_Window_LeftEdge_Centered;
        _newIconWin__WindowWidth = 300;
        _newIconWin__WindowHeight = 300;

        _newIconWin__Title = (STRPTR) GetTagData(MUIA_IconWindow_Location, (IPTR)NULL, message->ops_AttrList);
        _newIconWin__TitleLen = strlen(_newIconWin__Title);
        isVolume = (_newIconWin__Title[_newIconWin__TitleLen - 1] == ':');

        D(bug("[Wanderer:IconWindow] %s: Opening %s Window '%s'\n", __PRETTY_FUNCTION__, isVolume ? "Volume Root" : "Drawer", _newIconWin__Title));

        drawericon = GetIconTags(_newIconWin__Title,
                                ICONGETA_Screen, _newIconWin__Screen,
                                ICONGETA_FailIfUnavailable, FALSE,
                                ICONGETA_IsDefaultIcon, &geticon_isdefault,
                                ICONA_ErrorCode, &geticon_error,
                                TAG_DONE);

        if ((drawericon) && (drawericon->do_DrawerData))
        {
D(bug("[Wanderer:IconWindow] %s: Directory Icon has DRAWER data!\n", __PRETTY_FUNCTION__));
            _newIconWin__WindowTop = drawericon->do_DrawerData->dd_NewWindow.TopEdge;
            _newIconWin__WindowLeft = drawericon->do_DrawerData->dd_NewWindow.LeftEdge;
            _newIconWin__WindowWidth = drawericon->do_DrawerData->dd_NewWindow.Width;
            _newIconWin__WindowHeight = drawericon->do_DrawerData->dd_NewWindow.Height;
        }

        iconviewclass = IconWindowDrawerList_CLASS;
        if ((drawericon) && (drawericon->do_Gadget.UserData > 0))
        {
D(bug("[Wanderer:IconWindow] %s: Directory Icons has OS 2.x/3.x data: FLAGS %x [\n", __PRETTY_FUNCTION__, drawericon->do_DrawerData->dd_Flags));
D(bug("] VIEWMODES %x [", drawericon->do_DrawerData->dd_ViewModes));

            switch (drawericon->do_DrawerData->dd_ViewModes)
            {
                case 0:
                {
                    D(bug("Default (inherit from parent)"));
                    break;
                }
                case 1:
                {
                    D(bug("View as icons"));
                    break;
                }
                case 2:
                {
                    D(bug("View as text, sorted by name"));
                    iconviewclass = IconWindowDetailDrawerList_CLASS;
                    break;
                }
                case 3:
                {
                    D(bug("View as text, sorted by date"));
                    iconviewclass = IconWindowDetailDrawerList_CLASS;
                    break;
                }
                case 4:
                {
                    D(bug("View as text, sorted by size"));
                    iconviewclass = IconWindowDetailDrawerList_CLASS;
                    break;
                }
                case 5:
                {
                    D(bug("View as text, sorted by type"));
                    iconviewclass = IconWindowDetailDrawerList_CLASS;
                    break;
                }
                default:
                {
                    D(bug("INVALID"));
                }
            }
D(bug("]\n"));
        }

        if (isVolume &&
                (((geticon_isdefault) && (_newIconWin__VOLVIEWMODE == MUIV_IconWindow_VolumeInfoMode_ShowAllIfNoInfo)) ||
                (_newIconWin__VOLVIEWMODE == MUIV_IconWindow_VolumeInfoMode_ShowAll)))
        {
            D(bug("[Wanderer:IconWindow] %s: setting 'SHOW ALL FILES'\n", __PRETTY_FUNCTION__));
            forceshowall = TRUE;
        }

        _newIconWin__IconListObj = (Object *) NewObject(iconviewclass->mcc_Class, NULL,
                                                                 MUIA_IconDrawerList_Drawer, (IPTR) _newIconWin__Title,
                                                                 MUIA_Wanderer_FileSysNotifyPort, _newIconWin__FSNotifyPort,
                                                                TAG_DONE);

        if (drawericon)
            IconWindow_RestoreSettings(drawericon, _newIconWin__IconListObj, forceshowall);

        _newIconWin__TopPanelRootGroupObj = MUI_NewObject(MUIC_Group,
                MUIA_InnerLeft,(0),
                MUIA_InnerRight,(0),
                MUIA_InnerTop,(0),
                MUIA_InnerBottom,(0),
                MUIA_Frame, MUIV_Frame_None,
                MUIA_Group_Spacing, 0,
                Child, (_newIconWin__TopPanelSpacerObj = HSpace(0)),
        TAG_DONE);

        if (_newIconWin__TopPanelRootGroupObj)
        {
          _newIconWin__TopPanelContainerObj = MUI_NewObject(MUIC_Group, MUIA_Group_Horiz, TRUE,
                MUIA_ShowMe,            FALSE,
                InnerSpacing(0,0),
                MUIA_HorizWeight,       100,
                MUIA_VertWeight,        0,
                MUIA_Frame,             MUIV_Frame_None,
                MUIA_Group_Spacing,     3,
                Child, (IPTR)_newIconWin__TopPanelRootGroupObj,
          TAG_DONE);
        }

        _newIconWin__BottomPanelRootGroupObj = MUI_NewObject(MUIC_Group,
                MUIA_InnerLeft,(0),
                MUIA_InnerRight,(0),
                MUIA_InnerTop,(0),
                MUIA_InnerBottom,(0),
                MUIA_Frame, MUIV_Frame_None,
                MUIA_Group_Spacing, 0,
                Child, (_newIconWin__BottomPanelSpacerObj = HSpace(0)),
        TAG_DONE);

        if (_newIconWin__BottomPanelRootGroupObj)
        {
          _newIconWin__BottomPanelContainerObj = MUI_NewObject(MUIC_Group, MUIA_Group_Horiz, TRUE,
                MUIA_ShowMe,            FALSE,
                InnerSpacing(0,0),
                MUIA_HorizWeight,       100,
                MUIA_VertWeight,        0,
                MUIA_Frame,             MUIV_Frame_None,
                MUIA_Group_Spacing,     3,
                Child, (IPTR)_newIconWin__BottomPanelRootGroupObj,
          TAG_DONE);
        }

        _newIconWin__LeftPanelRootGroupObj = MUI_NewObject(MUIC_Group,
                MUIA_InnerLeft,(0),
                MUIA_InnerRight,(0),
                MUIA_InnerTop,(0),
                MUIA_InnerBottom,(0),
                MUIA_Frame, MUIV_Frame_None,
                MUIA_Group_Spacing, 0,
                Child, (_newIconWin__LeftPanelSpacerObj = HVSpace),
        TAG_DONE);

        if (_newIconWin__LeftPanelRootGroupObj)
        {
          _newIconWin__LeftPanelContainerObj = MUI_NewObject(MUIC_Group, MUIA_Group_Horiz, TRUE,
                MUIA_ShowMe,            FALSE,
                InnerSpacing(0,0),
                MUIA_HorizWeight,       0,
                MUIA_VertWeight,        100,
                MUIA_Frame,             MUIV_Frame_None,
                MUIA_Group_Spacing,     3,
                Child, (IPTR)_newIconWin__LeftPanelRootGroupObj,
          TAG_DONE);
        }
    }

    _newIconWin__RootViewObj = (Object *) IconListviewObject,
                                            MUIA_Weight,                    100,
                                            MUIA_IconListview_UseWinBorder, TRUE,
                                            MUIA_IconListview_IconList,     (IPTR) _newIconWin__IconListObj,
                                        End;
                            
    DOPENWINDOW(bug("[Wanderer:IconWindow] %s: Window Co-ords %d,%d [%d x %d]\n", __PRETTY_FUNCTION__, _newIconWin__WindowLeft, _newIconWin__WindowTop, _newIconWin__WindowWidth, _newIconWin__WindowHeight));
    D(bug("[Wanderer:IconWindow] %s: Font @ 0x%p\n", __PRETTY_FUNCTION__, _newIconWin__WindowFont));
    D(bug("[Wanderer:IconWindow] %s: TopPanelContainerObj 0x%p RootViewObj 0x%p\n", __PRETTY_FUNCTION__, _newIconWin__TopPanelContainerObj, _newIconWin__RootViewObj));

    self = (Object *) DoSuperNew(CLASS, self, 
        MUIA_Window_Screen,                                    _newIconWin__Screen,
        MUIA_Window_Backdrop,                                  isBackdrop ? TRUE : FALSE,
        MUIA_Window_Borderless,                                isBackdrop ? TRUE : FALSE,
        MUIA_Window_Width,                                     _newIconWin__WindowWidth,
        MUIA_Window_Height,                                    _newIconWin__WindowHeight,
        MUIA_Window_LeftEdge,                                  _newIconWin__WindowLeft,
        MUIA_Window_TopEdge,                                   _newIconWin__WindowTop,
        (!isBackdrop) ? MUIA_Window_AltWidth : TAG_IGNORE,     100,
        (!isBackdrop) ? MUIA_Window_AltHeight : TAG_IGNORE,    80,
        MUIA_Window_Title,                                   (IPTR)_newIconWin__Title,

        MUIA_Window_DragBar,                                   (!isBackdrop) ? TRUE : FALSE,
        MUIA_Window_CloseGadget,                               (!isBackdrop) ? TRUE : FALSE,
        MUIA_Window_SizeGadget,                                (!isBackdrop) ? TRUE : FALSE,
        MUIA_Window_DepthGadget,                               (!isBackdrop) ? TRUE : FALSE,
#if defined(MUIA_Window_ZoomGadget)
        MUIA_Window_ZoomGadget,                                (!isBackdrop) ? TRUE : FALSE,
#endif
        MUIA_Window_UseBottomBorderScroller,                   (!isBackdrop) ? TRUE : FALSE,
        MUIA_Window_UseRightBorderScroller,                    (!isBackdrop) ? TRUE : FALSE,
        MUIA_Window_IsSubWindow,                             TRUE,

        WindowBF_TAG,                                        _newIconWin__BackFillHook,

        MUIA_Window_ScreenTitle,                             (IPTR) "",

        WindowContents, (IPTR) MUI_NewObject(MUIC_Group,
            MUIA_Group_Spacing,  0,
            MUIA_Group_SameSize, FALSE,
            InnerSpacing(0,0),

            /* "Extension" group */
            _newIconWin__TopPanelContainerObj ? Child : TAG_IGNORE, (IPTR)_newIconWin__TopPanelContainerObj,

            Child, HGroup,
                _newIconWin__LeftPanelContainerObj ? Child : TAG_IGNORE, (IPTR)_newIconWin__LeftPanelContainerObj,
                /* icon list */
                Child, (IPTR) _newIconWin__RootViewObj,
            End,

            _newIconWin__BottomPanelContainerObj ? Child : TAG_IGNORE, (IPTR)_newIconWin__BottomPanelContainerObj,

        TAG_DONE),

        TAG_MORE, (IPTR) message->ops_AttrList
    );

    if (self != NULL)
    {
        SETUP_ICONWINDOW_INST_DATA;

        D(bug("[Wanderer:IconWindow] %s: SELF = 0x%p\n", __PRETTY_FUNCTION__, self));

        data->iwd_VolViewMode                           = _newIconWin__VOLVIEWMODE;

        data->iwd_Screen                                = _newIconWin__Screen;
        data->iwd_Title                                 = _newIconWin__Title;

        data->iwd_RootViewObj                           = _newIconWin__RootViewObj;
        data->iwd_IconListObj                           = _newIconWin__IconListObj;

        SET(data->iwd_RootViewObj, MUIA_IconWindow_Window,      self);

        data->iwd_ActionHook                            = actionHook;

        data->iwd_TopPanel.iwp_PanelGroupObj            = _newIconWin__TopPanelRootGroupObj;
        data->iwd_TopPanel.iwp_PanelContainerObj        = _newIconWin__TopPanelContainerObj;
        data->iwd_TopPanel.iwp_PanelGroupSpacerObj      = _newIconWin__TopPanelSpacerObj;

        data->iwd_LeftPanel.iwp_PanelGroupObj           = _newIconWin__LeftPanelRootGroupObj;
        data->iwd_LeftPanel.iwp_PanelContainerObj       = _newIconWin__LeftPanelContainerObj;
        data->iwd_LeftPanel.iwp_PanelGroupSpacerObj     = _newIconWin__LeftPanelSpacerObj;

        data->iwd_BottomPanel.iwp_PanelGroupObj         = _newIconWin__BottomPanelRootGroupObj;
        data->iwd_BottomPanel.iwp_PanelContainerObj     = _newIconWin__BottomPanelContainerObj;
        data->iwd_BottomPanel.iwp_PanelGroupSpacerObj   = _newIconWin__BottomPanelSpacerObj;

        data->iwd_Flags                                 = 0;

        data->iwd_Flags                                 |= (isRoot) ? IWDFLAG_ISROOT : 0;
        data->iwd_Flags                                 |= (isBackdrop) ? IWDFLAG_ISBACKDROP : 0;

        data->iwd_ViewSettings_Attrib                   = (data->iwd_Flags & IWDFLAG_ISROOT) 
                                                          ? "Workbench"
                                                          : "Drawer";

        data->iwd_FSNotifyPort                          = _newIconWin__FSNotifyPort;

        if (prefs)
        {
#ifdef __AROS__
            data->iwd_PrefsUpdated_hook.h_Entry = ( HOOKFUNC )IconWindow__HookFunc_PrefsUpdatedFunc;
#else
            data->iwd_PrefsUpdated_hook = &iwd_PrefsUpdated_hook;
#endif

            DoMethod
              (
                prefs, MUIM_Notify, MUIA_WandererPrefs_Processing, FALSE,
                (IPTR) self, 3, 
                MUIM_CallHook, &data->iwd_PrefsUpdated_hook, (IPTR)CLASS
              );

            data->iwd_ViewSettings_PrefsNotificationObject = (Object *) DoMethod(prefs,
                                    MUIM_WandererPrefs_ViewSettings_GetNotifyObject,
                                    data->iwd_ViewSettings_Attrib);
            data->iwd_WindowFont  = (struct TextFont*)DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute,
                    data->iwd_ViewSettings_Attrib, MUIA_IconWindow_Font);
            SET(data->iwd_IconListObj, MUIA_Font, data->iwd_WindowFont);
        }

#ifdef __AROS__
        data->iwd_ProcessBackground_hook.h_Entry = ( HOOKFUNC )IconWindow__HookFunc_ProcessBackgroundFunc;
#else
        data->iwd_ProcessBackground_hook = &iwd_ProcessBackground_hook;
#endif

        if ((data->iwd_BackFill_hook = _newIconWin__BackFillHook))
        {
            data->iwd_BackFillHookData.bfhd_IWClass = CLASS;
            data->iwd_BackFillHookData.bfhd_IWObject = self;
            data->iwd_BackFill_hook->h_Data = &data->iwd_BackFillHookData;
        }

        ForeachNode(&iconwindow_Extensions, iw_Extension)
        {
            D(bug("[Wanderer:IconWindow] %s: Setting up '%s' @ %p\n", __PRETTY_FUNCTION__, iw_Extension->iwe_Node.ln_Name, iw_Extension));
            iw_Extension->iwe_Setup(CLASS, self, message);
        }

        data->iwd_Flags |= IWDFLAG_SETUP;

        /* If double clicked then we call our own private methods, that's 
           easier then using Hooks */
        DoMethod
          (
            _newIconWin__IconListObj, MUIM_Notify, MUIA_IconList_DoubleClick, TRUE, 
            (IPTR) self, 1, MUIM_IconWindow_DoubleClicked
          );

        /* notify when icons dropped on another (wanderer) window */
        DoMethod
          (
            _newIconWin__IconListObj, MUIM_Notify, MUIA_IconList_IconsDropped, MUIV_EveryTime,
            (IPTR) self, 1, MUIM_IconWindow_IconsDropped
          );

        /* notify when icons dropped on custom application */
        DoMethod
          (
            _newIconWin__IconListObj, MUIM_Notify, MUIA_IconList_AppWindowDrop, MUIV_EveryTime,
            (IPTR) self, 1, MUIM_IconWindow_AppWindowDrop
          );

        DoMethod
          (
            _newIconWin__IconListObj, MUIM_Notify, MUIA_IconList_Clicked, MUIV_EveryTime,
            (IPTR) self, 1, MUIM_IconWindow_Clicked
          );
    }
D(bug("[Wanderer:IconWindow] obj = %ld\n", self));
    return self;
}
///

///OM_DISPOSE()
IPTR IconWindow__OM_DISPOSE(Class *CLASS, Object *self, Msg message)
{
    SETUP_ICONWINDOW_INST_DATA;

    struct iconWindow_Extension *iw_Extension = NULL;
    Object *prefs = NULL;

    GET(_app(self), MUIA_Wanderer_Prefs, &prefs);

    if (prefs)
    {
        DoMethod
          (
            prefs,
            MUIM_KillNotifyObj, MUIA_WandererPrefs_Processing, (IPTR) self
          );

        if (data->iwd_Flags & IWDFLAG_SETUP)
        {
            ForeachNode(&iconwindow_Extensions, iw_Extension)
            {
                D(bug("[Wanderer:IconWindow] %s: Cleanup '%s'\n", __PRETTY_FUNCTION__, iw_Extension->iwe_Node.ln_Name));
                iw_Extension->iwe_Cleanup(CLASS, self, message);
            }
        }
    }
  
    if (data->iwd_BackFill_hook)
        FreeVec(data->iwd_BackFill_hook);
  
    return DoSuperMethodA(CLASS, self, message);
}
///

///OM_SET()
IPTR IconWindow__OM_SET(Class *CLASS, Object *self, struct opSet *message)
{
    SETUP_ICONWINDOW_INST_DATA;

    struct iconWindow_Extension *iw_Extension = NULL;

    struct TagItem  *tstate = message->ops_AttrList, *tag;
    BOOL      UpdateIconlist = FALSE;
    IPTR      focusicon = (IPTR) NULL;
    IPTR        rv = TRUE;

    while ((tag = NextTagItem((TAGITEM)&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Window_Screen:
            D(bug("[Wanderer:IconWindow] %s: MUIA_Window_Screen [screen @ %p]\n", __PRETTY_FUNCTION__, tag->ti_Data));
            data->iwd_Screen = (struct Screen *) tag->ti_Tag;
            break;

        case MUIA_ShowMe:
            D(bug("[Wanderer:IconWindow] %s: MUIA_ShowMe [%x]\n", __PRETTY_FUNCTION__, tag->ti_Data));
            if (tag->ti_Data)
            {
                struct Screen *__Wanderer__Screen = NULL;
                GET(_app(self), MUIA_Wanderer_Screen, &__Wanderer__Screen);
                if (__Wanderer__Screen != data->iwd_Screen)
                {
                    D(bug("[Wanderer:IconWindow] %s: Screen Changed [old = %p, new = %p]\n", __PRETTY_FUNCTION__, data->iwd_Screen, __Wanderer__Screen));
                    SET(self, MUIA_Window_Screen, __Wanderer__Screen);
                    if (((data->iwd_Flags & IWDFLAG_ISROOT)) && ((data->iwd_Flags & IWDFLAG_ISBACKDROP)))
                    {
                        IPTR                            _IconWin__NewWindowWidth = 0;
                        IPTR                            _IconWin__NewWindowHeight = 0;
#if 0 /* unused */
                        IPTR                            _IconWin__NewWindowLeft = 0;
#endif
                        IPTR                            _IconWin__NewWindowTop = 0;

                        D(bug("[Wanderer:IconWindow] %s: Updating Backdrop Window Dimensions\n", __PRETTY_FUNCTION__));

                        _IconWin__NewWindowWidth = GetBitMapAttr(__Wanderer__Screen->RastPort.BitMap, BMA_WIDTH);
                        _IconWin__NewWindowHeight = GetBitMapAttr(__Wanderer__Screen->RastPort.BitMap, BMA_HEIGHT);
                        D(bug("[Wanderer:IconWindow] %s: Screen dimensions ..  %d x %d\n", __PRETTY_FUNCTION__, _IconWin__NewWindowWidth, _IconWin__NewWindowHeight));

                        _IconWin__NewWindowTop = __Wanderer__Screen->BarHeight + 1;
#if 0 /* unused */
                        _IconWin__NewWindowLeft = 0;
#endif

                        _IconWin__NewWindowHeight -= _IconWin__NewWindowTop;

                        D(bug("[Wanderer:IconWindow] %s: New Window dimensions ..  %d x %d @ %d, %d\n", __PRETTY_FUNCTION__, _IconWin__NewWindowWidth, _IconWin__NewWindowHeight, 0, _IconWin__NewWindowTop));

                        SET(self, MUIA_Window_Width, _IconWin__NewWindowWidth);
                        SET(self, MUIA_Window_Height, _IconWin__NewWindowHeight);
                    }
                }
            }
            break;

        case MUIA_IconWindow_Changed:
            data->iwd_Flags |= (tag->ti_Data) ? IWDFLAG_NEEDSUPDATE : 0;
            break;

        case MUIA_Window_Open:
            D(bug("[Wanderer:IconWindow] %s: MUIA_Window_Open [%ld]\n", __PRETTY_FUNCTION__, tag->ti_Data));
            if (tag->ti_Data)
            {
                DoMethod(data->iwd_IconListObj, MUIM_IconList_Clear);

                rv = DoSuperMethodA(CLASS, self, (Msg) message);
#if defined(ICONWINDOW_BUFFERLIST)
                if (!((data->iwd_Flags & IWDFLAG_ISROOT)))
                {
                    IPTR        CURDISPFLAGS = NULL;
                    GET(data->iwd_IconListObj, MUIA_IconList_DisplayFlags, &CURDISPFLAGS);
                    CURDISPFLAGS |= ICONLIST_DISP_BUFFERED;
                    SET(data->iwd_IconListObj, MUIA_IconList_DisplayFlags, CURDISPFLAGS);
                }
#endif
                D(bug("[Wanderer:IconWindow] %s: Process the background ..\n", __PRETTY_FUNCTION__));
                DoMethod(self, MUIM_IconWindow_BackFill_ProcessBackground, data->iwd_BackFillInfo, data->iwd_RootViewObj);

                D(bug("[Wanderer:IconWindow] %s: Force an update of the list ..\n", __PRETTY_FUNCTION__));
                DoMethod(data->iwd_IconListObj, MUIM_IconList_Update);
                DoMethod(data->iwd_IconListObj, MUIM_IconList_Sort);
                return rv;
            }
            break;

        case MUIA_Window_Activate:
            if (data->iwd_IconListObj)
                GET(data->iwd_IconListObj, MUIA_IconList_FocusIcon, &focusicon);

            break;

        case MUIA_IconWindow_Font:
            data->iwd_WindowFont = (struct TextFont  *)tag->ti_Data;
            D(bug("[Wanderer:IconWindow] %s: MUIA_IconWindow_Font [font @ 0x%p]\n", __PRETTY_FUNCTION__, data->iwd_WindowFont));

            if (!data->iwd_WindowFont)
                SetFont(_rp(self), data->iwd_WindowFont);

            if (data->iwd_IconListObj)
                SET(data->iwd_IconListObj, MUIA_Font, data->iwd_WindowFont);

            break;

        case MUIA_IconWindow_Location:
            D(bug("[Wanderer:IconWindow] %s: MUIA_IconWindow_Location [drawer '%s']\n", __PRETTY_FUNCTION__, data->iwd_DirectoryPath));

            if (!(data->iwd_Flags & IWDFLAG_ISROOT))
            {
                strcpy(data->iwd_DirectoryPath, (STRPTR)tag->ti_Data);
                SET(self, MUIA_Window_Title, (IPTR)data->iwd_DirectoryPath);
                SET(data->iwd_IconListObj, MUIA_IconDrawerList_Drawer, (IPTR)data->iwd_DirectoryPath);
            }
            break;

        case MUIA_IconWindow_BackgroundAttrib:
            D(bug("[Wanderer:IconWindow] %s: MUIA_IconWindow_BackgroundAttrib (not implemented)\n", __PRETTY_FUNCTION__));
            break;

        }
    }

    rv = DoSuperMethodA(CLASS, self, (Msg) message);

    if (data->iwd_Flags & IWDFLAG_SETUP)
    {
        ForeachNode(&iconwindow_Extensions, iw_Extension)
        {
            D(bug("[Wanderer:IconWindow] %s: Checking '%s'\n", __PRETTY_FUNCTION__, iw_Extension->iwe_Node.ln_Name));
            iw_Extension->iwe_Set(CLASS, self, message);
        }
    }

    if (UpdateIconlist)
    {
        DoMethod(data->iwd_IconListObj, MUIM_IconList_Update);
        DoMethod(data->iwd_IconListObj, MUIM_IconList_Sort);
    }

    if (focusicon)
    {
        D(bug("[Wanderer:IconWindow] %s: Updating focused icon (@ 0x%p)\n", __PRETTY_FUNCTION__, focusicon));
        //MUI_Redraw(data->iwd_IconListObj, MADF_DRAWOBJECT);
    }

    return rv;
}
///

///OM_GET()
IPTR IconWindow__OM_GET(Class *CLASS, Object *self, struct opGet *message)
{
    SETUP_ICONWINDOW_INST_DATA;
    struct iconWindow_Extension *iw_Extension = NULL;
    IPTR *store = message->opg_Storage;
    IPTR  rv    = TRUE;

    switch (message->opg_AttrID)
    {
    case MUIA_IconWindow_Changed:
        *store = (IPTR)(data->iwd_Flags & IWDFLAG_NEEDSUPDATE);
        break;

    case MUIA_IconWindow_Window:
        *store = (IPTR)self;
        break;

    case MUIA_IconWindow_Location:
        *store = !(data->iwd_Flags & IWDFLAG_ISROOT)
            ? XGET(data->iwd_IconListObj, MUIA_IconDrawerList_Drawer)
            : (IPTR)NULL;
        break;

    case MUIA_IconWindow_IconList:
        *store = (IPTR)data->iwd_IconListObj;
        break;

    case MUIA_IconWindow_IsRoot:
        *store = (IPTR)(data->iwd_Flags & IWDFLAG_ISROOT);
        break;

    case MUIA_IconWindow_IsBackdrop:
        *store = (IPTR)(data->iwd_Flags & IWDFLAG_ISBACKDROP);
        break;

    case MUIA_IconWindow_BackFillData:
        *store = (IPTR)data->iwd_BackFillInfo;
        break;

    case MUIA_IconWindow_BackgroundAttrib:
        *store = (IPTR)data->iwd_ViewSettings_Attrib;
        break;

    case MUIA_Version:
        *store = (IPTR)WIWVERS;
        break;

    case MUIA_Revision:
        *store = (IPTR)WIWREV;
        break;

    default:
        rv = FALSE;
    }

    if (!rv)
        rv = DoSuperMethodA(CLASS, self, (Msg) message);

    if (!rv)
    {
        ForeachNode(&iconwindow_Extensions, iw_Extension)
        {
            D(bug("[Wanderer:IconWindow] %s: Checking '%s'\n", __PRETTY_FUNCTION__, iw_Extension->iwe_Node.ln_Name));
            if ((rv = iw_Extension->iwe_Get(CLASS, self, message)))
                break;
        }
    }

    return rv;
}
///

///IconWindow__MUIM_Window_Setup()
IPTR IconWindow__MUIM_Window_Setup
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_ICONWINDOW_INST_DATA;

    Object *prefs = NULL;

    D(bug("[Wanderer:IconWindow]: %s()\n", __PRETTY_FUNCTION__));

    if (!DoSuperMethodA(CLASS, self, message)) return FALSE;

    if (iconwindow_BackFill_Active)
    {
        data->iwd_BackFillInfo =(struct BackFillInfo *) DoMethod(self, MUIM_IconWindow_BackFill_Setup);
        D(bug("[Wanderer:IconWindow] %s: Window BackFill_Data @ 0x%p for '%s'\n", __PRETTY_FUNCTION__, data->iwd_BackFillInfo, iconwindow_BackFill_Active->bfd_BackFillID));
    }

    GET(_app(self), MUIA_Wanderer_Prefs, &prefs);

    D(bug("[Wanderer:IconWindow] %s: App PrefsObj @ 0x%p\n", __PRETTY_FUNCTION__, prefs));

    if ((prefs) && (data->iwd_ViewSettings_PrefsNotificationObject))
    {
        D(bug("[Wanderer:IconWindow] %s: Setting up window prefs change hooks\n", __PRETTY_FUNCTION__));

        /* Set-up a hook to call ProcessBackground on prefs notification */
        DoMethod
          (
            data->iwd_ViewSettings_PrefsNotificationObject, MUIM_Notify, MUIA_Background, MUIV_EveryTime,
            (IPTR) self, 3, 
            MUIM_CallHook, &data->iwd_ProcessBackground_hook, (IPTR)CLASS
          );

        if ((data->iwd_Flags & IWDFLAG_ISROOT))
        {
            DoMethod
              (
                data->iwd_ViewSettings_PrefsNotificationObject, MUIM_Notify, MUIA_IconWindowExt_ImageBackFill_BGRenderMode, MUIV_EveryTime,
                (IPTR) self, 3, 
                MUIM_CallHook, &data->iwd_ProcessBackground_hook, (IPTR)CLASS
              );
        }

        DoMethod
          (
            data->iwd_ViewSettings_PrefsNotificationObject, MUIM_Notify, MUIA_IconWindowExt_ImageBackFill_BGTileMode, MUIV_EveryTime,
            (IPTR) self, 3, 
            MUIM_CallHook, &data->iwd_ProcessBackground_hook, (IPTR)CLASS
          );

        DoMethod
          (
            data->iwd_ViewSettings_PrefsNotificationObject, MUIM_Notify, MUIA_IconWindowExt_ImageBackFill_BGXOffset, MUIV_EveryTime,
            (IPTR) self, 3, 
            MUIM_CallHook, &data->iwd_ProcessBackground_hook, (IPTR)CLASS
          );

        DoMethod
          (
            data->iwd_ViewSettings_PrefsNotificationObject, MUIM_Notify, MUIA_IconWindowExt_ImageBackFill_BGYOffset, MUIV_EveryTime,
            (IPTR) self, 3, 
            MUIM_CallHook, &data->iwd_ProcessBackground_hook, (IPTR)CLASS
          );

        /* React to notification on font change */
        DoMethod
          (
            data->iwd_ViewSettings_PrefsNotificationObject, MUIM_Notify, MUIA_IconWindow_Font, MUIV_EveryTime,
            (IPTR) self, 3,
            MUIM_Set, MUIA_IconWindow_Font, MUIV_TriggerValue
          );

    }

    D(bug("[Wanderer:IconWindow] %s: Setup complete!\n", __PRETTY_FUNCTION__));

    return TRUE;
}
///

///IconWindow__MUIM_Window_Cleanup()
IPTR IconWindow__MUIM_Window_Cleanup
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_ICONWINDOW_INST_DATA;

    D(bug("[Wanderer:IconWindow]: %s()\n", __PRETTY_FUNCTION__));

    if (data->iwd_BackFillInfo)
    {
        DoMethod(self, MUIM_IconWindow_BackFill_Cleanup, data->iwd_BackFillInfo);
        data->iwd_BackFillInfo = NULL;
    }

    if (data->iwd_ViewSettings_PrefsNotificationObject)
    {
        DoMethod
          (
            data->iwd_ViewSettings_PrefsNotificationObject,
            MUIM_KillNotifyObj, MUIA_IconWindowExt_ImageBackFill_BGYOffset, (IPTR) self
          );

        DoMethod
          (
            data->iwd_ViewSettings_PrefsNotificationObject,
            MUIM_KillNotifyObj, MUIA_IconWindowExt_ImageBackFill_BGXOffset, (IPTR) self
          );

        DoMethod
          (
            data->iwd_ViewSettings_PrefsNotificationObject,
            MUIM_KillNotifyObj, MUIA_IconWindowExt_ImageBackFill_BGTileMode, (IPTR) self
          );

        if ((data->iwd_Flags & IWDFLAG_ISROOT))
        {
            DoMethod
              (
                data->iwd_ViewSettings_PrefsNotificationObject,
                MUIM_KillNotifyObj, MUIA_IconWindowExt_ImageBackFill_BGRenderMode, (IPTR) self
              );
        }

        DoMethod
          (
            data->iwd_ViewSettings_PrefsNotificationObject,
            MUIM_KillNotifyObj, MUIA_Background, (IPTR) self
          );

        DoMethod
          (
            data->iwd_ViewSettings_PrefsNotificationObject,
            MUIM_KillNotifyObj, MUIA_IconWindow_Font, (IPTR) self
          );
    }
    return DoSuperMethodA(CLASS, self, message);
}
///

///IconWindow__MUIM_IconWindow_DoubleClicked()
IPTR IconWindow__MUIM_IconWindow_DoubleClicked
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_ICONWINDOW_INST_DATA;

    D(bug("[Wanderer:IconWindow]: %s()\n", __PRETTY_FUNCTION__));

    if (data->iwd_ActionHook)
    {
        struct IconWindow_ActionMsg msg;
        msg.type     = ICONWINDOW_ACTION_OPEN;
        msg.iconlist = data->iwd_IconListObj;
        msg.isroot   = (data->iwd_Flags & IWDFLAG_ISROOT);
        msg.click    = NULL;
        CallHookPkt(data->iwd_ActionHook, self, &msg);
    }

    return TRUE;
}
///

///IconWindow__MUIM_IconWindow_Clicked()
IPTR IconWindow__MUIM_IconWindow_Clicked
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_ICONWINDOW_INST_DATA;

    D(bug("[Wanderer:IconWindow]: %s()\n", __PRETTY_FUNCTION__));

    if (data->iwd_ActionHook)
    {
        struct IconWindow_ActionMsg msg;
        msg.type     = ICONWINDOW_ACTION_CLICK;
        msg.iconlist = data->iwd_IconListObj;
        msg.isroot   = (data->iwd_Flags & IWDFLAG_ISROOT);
        GET(data->iwd_IconListObj, MUIA_IconList_Clicked, &msg.click);
        CallHookPkt(data->iwd_ActionHook, self, &msg);
    }

    return TRUE;
}
///

///IconWindow__MUIM_IconWindow_IconsDropped()
IPTR IconWindow__MUIM_IconWindow_IconsDropped
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_ICONWINDOW_INST_DATA;

    D(bug("[Wanderer:IconWindow]: %s()\n", __PRETTY_FUNCTION__));

    if (data->iwd_ActionHook)
    {
        struct IconWindow_ActionMsg msg;
        msg.type     = ICONWINDOW_ACTION_ICONDROP;
        msg.iconlist = data->iwd_IconListObj;
        msg.isroot   = (data->iwd_Flags & IWDFLAG_ISROOT);
        GET(data->iwd_IconListObj, MUIA_IconList_IconsDropped, &msg.drop);
        if (msg.drop)
        {
            NNSET(data->iwd_IconListObj, MUIA_IconList_IconsDropped, NULL);
            CallHookPkt(data->iwd_ActionHook, self, &msg);
        }
    }

    return TRUE;
}
///

///IconWindow__MUIM_IconWindow_AppWindowDrop()
IPTR IconWindow__MUIM_IconWindow_AppWindowDrop
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_ICONWINDOW_INST_DATA;

D(bug("[Wanderer:IconWindow]: %s()\n", __PRETTY_FUNCTION__));

    if (data->iwd_ActionHook)
    {
        struct IconWindow_ActionMsg msg;
        msg.type     = ICONWINDOW_ACTION_APPWINDOWDROP;
        msg.iconlist = data->iwd_IconListObj;
        msg.isroot   = (data->iwd_Flags & IWDFLAG_ISROOT);
        GET(data->iwd_IconListObj, MUIA_IconList_IconsDropped, &msg.drop);
        CallHookPkt(data->iwd_ActionHook, self, &msg);
    }

    return TRUE;
}
///

///IconWindow__MUIM_IconWindow_Open()
IPTR IconWindow__MUIM_IconWindow_Open
(
    Class *CLASS, Object *self, Msg message
)
{
//    SETUP_ICONWINDOW_INST_DATA;

D(bug("[Wanderer:IconWindow]: %s()\n", __PRETTY_FUNCTION__));

    if (!XGET(self, MUIA_Window_Open))
    {
        SET(self, MUIA_Window_Open, TRUE);
    }

D(bug("[Wanderer:IconWindow] %s: Setting window as active ..\n", __PRETTY_FUNCTION__));
    SET(self, MUIA_Window_Activate, TRUE);

D(bug("[Wanderer:IconWindow] %s: All done\n", __PRETTY_FUNCTION__));

    return TRUE;
}
///

///IconWindow__MUIM_IconWindow_DirectoryUp()
IPTR IconWindow__MUIM_IconWindow_DirectoryUp
(
  Class *CLASS, Object *self, Msg message
)
{
  SETUP_ICONWINDOW_INST_DATA;
  
  D(bug("[Wanderer:IconWindow]: %s()\n", __PRETTY_FUNCTION__));
  
  if (data->iwd_ActionHook)
  {
    struct IconWindow_ActionMsg msg;
    msg.type     = ICONWINDOW_ACTION_DIRUP;
    msg.iconlist = data->iwd_IconListObj;
    msg.isroot   = (data->iwd_Flags & IWDFLAG_ISROOT);
    msg.click    = NULL;
    CallHookPkt(data->iwd_ActionHook, self, &msg);
    
  }
  
  return TRUE;
}
///

///IconWindow__MUIM_IconWindow_UnselectAll()
IPTR IconWindow__MUIM_IconWindow_UnselectAll
(
  Class *CLASS, Object *self, Msg message
)
{
  SETUP_ICONWINDOW_INST_DATA;
  
  D(bug("[Wanderer:IconWindow]: %s()\n", __PRETTY_FUNCTION__));
  
  DoMethod(data->iwd_IconListObj, MUIM_IconList_UnselectAll);
  
  return TRUE;
}
///

///IconWindow__MUIM_IconWindow_Remove()
IPTR IconWindow__MUIM_IconWindow_Remove
(
  Class *CLASS, Object *self, Msg message
)
{
//  SETUP_ICONWINDOW_INST_DATA;
  
  D(bug("[Wanderer:IconWindow]: %s()\n", __PRETTY_FUNCTION__));
  
  // Remove window
  SET( self, MUIA_Window_Open, FALSE );
  DoMethod ( _app(self), OM_REMMEMBER, self );
  DisposeObject(self);
  
  return TRUE;
}
///

/*** Stubs for Backfill Hooks ******************************************************************/
///IconWindow__MUIM_IconWindow_BackFill_Register()
IPTR IconWindow__MUIM_IconWindow_BackFill_Register
(
  Class *CLASS, Object *self, struct MUIP_IconWindow_BackFill_Register *message
)
{
//    SETUP_ICONWINDOW_INST_DATA;
  
  D(bug("[Wanderer:IconWindow]: %s('%s')\n", __PRETTY_FUNCTION__, message->register_Node->bfd_BackFillID));

  AddTail(&iconwindow_BackFillNodes, (struct Node *)message->register_Node);
  if (iconwindow_BackFill_Active == NULL) iconwindow_BackFill_Active = message->register_Node;

  return TRUE;
}
///

///IconWindow__MUIM_IconWindow_BackFill_Setup()
IPTR IconWindow__MUIM_IconWindow_BackFill_Setup
(
  Class *CLASS, Object *self, struct MUIP_IconWindow_BackFill_Setup *message
)
{
//  SETUP_ICONWINDOW_INST_DATA;
  
  D(bug("[Wanderer:IconWindow]: %s()\n", __PRETTY_FUNCTION__));

  if (iconwindow_BackFill_Active == NULL) return FALSE;

  return (iconwindow_BackFill_Active->bfd_MUIM_IconWindow_BackFill_Setup)(CLASS, self, message);
}
///

///IconWindow__MUIM_IconWindow_BackFill_Cleanup()
IPTR IconWindow__MUIM_IconWindow_BackFill_Cleanup
(
  Class *CLASS, Object *self, struct MUIP_IconWindow_BackFill_Cleanup *message
)
{
  //SETUP_ICONWINDOW_INST_DATA;
  
  D(bug("[Wanderer:IconWindow]: %s()\n", __PRETTY_FUNCTION__));

  if (iconwindow_BackFill_Active == NULL) return FALSE;

  return (iconwindow_BackFill_Active->bfd_MUIM_IconWindow_BackFill_Cleanup)(CLASS, self, message);
}
///

///IconWindow__MUIM_IconWindow_BackFill_ProcessBackground()
IPTR IconWindow__MUIM_IconWindow_BackFill_ProcessBackground
(
  Class *CLASS, Object *self, struct MUIP_IconWindow_BackFill_ProcessBackground *message
)
{
  SETUP_ICONWINDOW_INST_DATA;
  
  IPTR retVal = (IPTR)FALSE;

  D(bug("[Wanderer:IconWindow]: %s()\n", __PRETTY_FUNCTION__));

  if (iconwindow_BackFill_Active != NULL)
  {
    D(bug("[Wanderer:IconWindow] %s: Asking module @ 0x%p to process ..\n", __PRETTY_FUNCTION__, iconwindow_BackFill_Active));
    retVal = (iconwindow_BackFill_Active->bfd_MUIM_IconWindow_BackFill_ProcessBackground)(CLASS, self, message);
  }
  
  if (!retVal && (data->iwd_RootViewObj != NULL))
  {
    Object                *IconWindowPB_PrefsObj = NULL;

    D(bug("[Wanderer:IconWindow] %s: No BackFill module/ module cant render mode\n", __PRETTY_FUNCTION__));
    D(bug("[Wanderer:IconWindow] %s: Using default MUI functions ..\n", __PRETTY_FUNCTION__));
    
    GET(_app(self), MUIA_Wanderer_Prefs, &IconWindowPB_PrefsObj);
    if (IconWindowPB_PrefsObj)
    {
      IPTR IconWindowPB_Background = 0;
      IPTR IconWindowPB_BGMode     = 0;
      IPTR IconWindowPB_BGTileMode = 0;

      if ((IconWindowPB_Background = DoMethod(IconWindowPB_PrefsObj, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwd_ViewSettings_Attrib, MUIA_Background)) != -1)
      {
        char *bgmode_string;
        BYTE this_mode;
  
        if ((IconWindowPB_BGMode = DoMethod(IconWindowPB_PrefsObj, MUIM_WandererPrefs_ViewSettings_GetAttribute,
                        data->iwd_ViewSettings_Attrib, MUIA_IconWindowExt_ImageBackFill_BGRenderMode)) == -1)
          IconWindowPB_BGMode = IconWindowExt_ImageBackFill_RenderMode_Tiled;

        if ((IconWindowPB_BGTileMode = DoMethod(IconWindowPB_PrefsObj, MUIM_WandererPrefs_ViewSettings_GetAttribute,
                                  data->iwd_ViewSettings_Attrib, MUIA_IconWindowExt_ImageBackFill_BGTileMode)) == -1)
          IconWindowPB_BGTileMode = IconWindowExt_ImageBackFill_TileMode_Float;
        
        SET(data->iwd_RootViewObj, MUIA_Background, IconWindowPB_Background);

        bgmode_string =(STRPTR) IconWindowPB_Background;
        this_mode = bgmode_string[0] - 48;

        D(bug("[Wanderer:IconWindow] %s: MUI BG Mode = %d\n", __PRETTY_FUNCTION__, this_mode));

        switch (this_mode)
        {
          case 5:
            //Image =D
            if (IconWindowPB_BGMode == IconWindowExt_ImageBackFill_RenderMode_Scale)
            {
              SET(data->iwd_RootViewObj, MUIA_IconListview_ScaledBackground, TRUE);
              break;
            }
            else
              SET(data->iwd_RootViewObj, MUIA_IconListview_ScaledBackground, FALSE);
  
          case 0:
            //MUI Pattern
            if (IconWindowPB_BGTileMode == IconWindowExt_ImageBackFill_TileMode_Fixed)
              SET(data->iwd_RootViewObj, MUIA_IconListview_FixedBackground, TRUE);
            else
              SET(data->iwd_RootViewObj, MUIA_IconListview_FixedBackground, FALSE);
            break;
        }
      }
    }
    retVal = FALSE;
  }

  return retVal;
}
///

///IconWindow__MUIM_IconWindow_BackFill_DrawBackground()
IPTR IconWindow__MUIM_IconWindow_BackFill_DrawBackground
(
  Class *CLASS, Object *self, struct MUIP_IconWindow_BackFill_DrawBackground *message
)
{
//  SETUP_ICONWINDOW_INST_DATA;

  D(bug("[Wanderer:IconWindow]: %s()\n", __PRETTY_FUNCTION__));

  if (iconwindow_BackFill_Active == NULL) return FALSE;

  return (iconwindow_BackFill_Active->bfd_MUIM_IconWindow_BackFill_DrawBackground)(CLASS, self, message);
}
///

///IconWindow__MUIM_IconWindow_RateLimitRefresh()
IPTR IconWindow__MUIM_IconWindow_RateLimitRefresh
(
  Class *CLASS, Object *self, Msg message
)
{
    Object * iconList = NULL;

    GET(self, MUIA_IconWindow_IconList, &iconList);

    if (iconList != NULL)
        return DoMethod(iconList, MUIM_IconWindowIconList_RateLimitRefresh);

    return (IPTR)FALSE;
}
///

///IconWindow__MUIM_IconWindow_Snapshot()
IPTR IconWindow__MUIM_IconWindow_Snapshot
(
  Class *CLASS, Object *self, struct MUIP_IconWindow_Snapshot * message
)
{
    Object  *iconList = (Object *) XGET(self, MUIA_IconWindow_IconList);

    D(bug("[Wanderer:IconWindow]: %s('%s')\n", __PRETTY_FUNCTION__, dir_name));

    if (message->snapshotall == TRUE)
    {
        struct IconList_Entry *icon_entry    = (IPTR)MUIV_IconList_NextIcon_Start;
        struct IconEntry      *node = NULL;
        struct TagItem            icon_tags[] =
        {
            { ICONPUTA_OnlyUpdatePosition, TRUE },
            { TAG_DONE, 0                       }
        };
        D(bug("[Wanderer:IconWindow] %s: snapshot ALL\n", __PRETTY_FUNCTION__));

        do
        {
            DoMethod(iconList, MUIM_IconList_NextIcon, MUIV_IconList_NextIcon_Visible, (IPTR)&icon_entry);

            if ((IPTR)icon_entry != MUIV_IconList_NextIcon_End)
            {
                node = (struct IconEntry *)((IPTR)icon_entry - ((IPTR)&node->ie_IconListEntry - (IPTR)node));
                if (node->ie_Flags & ICONENTRY_FLAG_HASICON)
                {
                    /* Snapshot-all should only save positions of objects that already have icons.
                     * This is by design and how 3.1 Workbench works. In order to snapshot all objects in window
                     * regardless if they have icons or not: 1) make all objects visible, 2) select all objects,
                     * 3) select icon->snapshot */
                    D(bug("[Wanderer:IconWindow] %s: SNAPSHOT entry = '%s' @ %p, (%p)\n", __PRETTY_FUNCTION__,
                            icon_entry->ile_IconEntry->ie_IconNode.ln_Name, icon_entry, node));
                    if (node->ie_DiskObj)
                    {
                        node->ie_DiskObj->do_CurrentX = node->ie_IconX;
                        node->ie_DiskObj->do_CurrentY = node->ie_IconY;
                        PutIconTagList(icon_entry->ile_IconEntry->ie_IconNode.ln_Name, node->ie_DiskObj, icon_tags);
                    }
                    else
                    {
                        D(bug("[[Wanderer:IconWindow] %s: icon has no diskobj!\n", __PRETTY_FUNCTION__));
                    }
                }
            }
            else
            {
                break;
            }
        } while (TRUE);
    }
    else
    {
        D(bug("[Wanderer:IconWindow] %s: snapshot WINDOW\n", __PRETTY_FUNCTION__));
    }

    IconWindow_StoreSettings(self);

    return (IPTR)TRUE;
}
///

///
IPTR IconWindow__SetupClass()
{
    D(bug("[Wanderer:IconWindow]: %s()\n", __PRETTY_FUNCTION__));

    NewList(&iconwindow_BackFillNodes);
    D(bug("[Wanderer:IconWindow] %s: iconwindow_BackFillNodes @ %p\n", __PRETTY_FUNCTION__, &iconwindow_BackFillNodes));
    NewList(&iconwindow_Extensions);
    D(bug("[Wanderer:IconWindow] %s: iconwindow_Extensions @ %p\n", __PRETTY_FUNCTION__, &iconwindow_Extensions));
    iconwindow_BackFill_Active = NULL;

    return TRUE;
}
///
/*** Setup ******************************************************************/
ICONWINDOW_CUSTOMCLASS
(
  IconWindow, NULL, MUIC_Window, NULL,
  OM_NEW,                                     struct opSet *,
  OM_DISPOSE,                         Msg,
  OM_SET,                                     struct opSet *,
  OM_GET,                                     struct opGet *,
  MUIM_Window_Setup,                          Msg,
  MUIM_Window_Cleanup,                        Msg,
  MUIM_IconWindow_Open,                       Msg,
  MUIM_IconWindow_UnselectAll,                Msg,
  MUIM_IconWindow_DoubleClicked,              Msg,
  MUIM_IconWindow_IconsDropped,               Msg,
  MUIM_IconWindow_Clicked,                    Msg,
  MUIM_IconWindow_DirectoryUp,                Msg,
  MUIM_IconWindow_AppWindowDrop,              Msg,
  MUIM_IconWindow_Remove,                     Msg,
  MUIM_IconWindow_RateLimitRefresh,           Msg,
  MUIM_IconWindow_Snapshot,                   struct MUIP_IconWindow_Snapshot *,
  MUIM_IconWindow_BackFill_Register,          struct MUIP_IconWindow_BackFill_Register *,
  MUIM_IconWindow_BackFill_Setup,             struct MUIP_IconWindow_BackFill_Setup *,
  MUIM_IconWindow_BackFill_Cleanup,           struct MUIP_IconWindow_BackFill_Cleanup *,
  MUIM_IconWindow_BackFill_ProcessBackground, struct MUIP_IconWindow_BackFill_ProcessBackground *,
  MUIM_IconWindow_BackFill_DrawBackground,    struct MUIP_IconWindow_BackFill_DrawBackground *
);

ADD2INIT(IconWindow__SetupClass, 0);

#ifndef __AROS__
int initIconWindowClass(void)
{
  IPTR ret1 = IconWindow_Initialize();

  IPTR ret2 = IconWindow__SetupClass();

  IPTR ret3 = ImageBackFill__SetupClass();

  if (ret1 && ret2 && ret3)
    return TRUE;
  else
    return FALSE;

}
#endif
