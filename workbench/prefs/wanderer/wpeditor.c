/*
    Copyright  2004-2018, The AROS Development Team. All rights reserved.
    This file is part of the Wanderer Preferences program, which is distributed
    under the terms of version 2 of the GNU General Public License.
    
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#define DEBUG 0

#define       WP_MAX_BG_TAG_COUNT                20
#define       WP_IFF_CHUNK_BUFFER_SIZE           1024
#define       WP_MAX_RENDER_MODES                4
#define       WP_MAX_TILE_MODES                  4
#define       WP_DRAWMODE_COUNT                  2

#define       DEBUG_TOOLBARINTERNAL
#define       DEBUG_SHOWUSERFILES
//#define       DEBUG_FORCEWINSIZE
//#define       DEBUG_NEWVIEWSETTINGS
//#define       DEBUG_NETWORKBROWSER
//#define       DEBUG_PERVIEWFONTS
#define       DEBUG_MULTLINE
#define       DEBUG_CHANGESCREENTITLE

#include <exec/types.h>
#include <utility/tagitem.h>
#include <libraries/mui.h>
#include <zune/customclasses.h>
#include <zune/prefseditor.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <proto/dos.h>
#include <proto/iffparse.h>

#include <prefs/prefhdr.h>
#include <prefs/wanderer.h>

#include <clib/alib_protos.h>
#include <stdio.h>
#include <string.h>

#include "locale.h"
#include "wpeditor.h"
#include "entryelements.h"

/* "TODO: Include the wandererprefs definitions in a better way .." */
#include "../../system/Wanderer/wandererprefs.h"
#include "../../system/Wanderer/Classes/iconlist_attributes.h"
#include "../../system/Wanderer/iconwindow_attributes.h"

#define WPD_ADVANCED_ICONS      0
#define WPD_ADVANCED_LABELS     1
#define WPD_ADVANCED_BACKGROUND 2

#define WPD_ICONSORTMODE_BYNAME 0
#define WPD_ICONSORTMODE_BYDATE 1
#define WPD_ICONSORTMODE_BYSIZE 2

#define WPD_ICONVIEWMODE_ALL    0
#define WPD_ICONVIEWMODE_ICONS  1

struct TagItem32 {
    ULONG ti_Tag;
    ULONG ti_Data;
};

/*** Private Methods ********************************************************/

/*** Instance Data **********************************************************/
struct WPEditor_ViewSettingsObject
{
    struct Node            wpedbo_Node;
    char                *wpedbo_ViewName;
    Object                *wpedbo_ImageSpecObject;
    Object                *wpedbo_AdvancedOptionsObject;
    IPTR                wpedbo_Type;

/* "TODO: Replace _wpeditor_intern_IconTextRenderModeNames and co. with per view list" */
    IPTR                *wpedbo_LabelRenderModeNames;
    IPTR                *wpedbo_LabelRenderModeEntries;
    IPTR                wpedbo_LabelRenderModeCount;

/* "TODO: Replace _wpeditor_intern_IconListModeNames and co. with per view list" */
    IPTR                *wpedbo_ListModeNames;
    IPTR                *wpedbo_ListModeEntries;
    IPTR                wpedbo_ListModeCount;

    IPTR                *wpedbo_BFRenderModeNames;
    IPTR                *wpedbo_BFRenderModeEntries;
    IPTR                *wpedbo_BFRenderModePages;
    IPTR                wpedbo_BFRenderModeCount;

    IPTR                *wpedbo_BFTileModeNames;
    IPTR                *wpedbo_BFTileModeEntries;
    IPTR                wpedbo_BFTileModeCount;

    struct TagItem32    *wpedbo_Options;
    struct Hook            wpedbo_Hook_CheckImage;
    struct Hook            wpedbo_Hook_OpenAdvancedOptions;
    IPTR                wpedbo_state_AdvancedDisabled;
};

struct WPEditor_AdvancedBackgroundWindow_DATA
{
    Object                *wpedabwd_Window_WindowObj,
                        *wpedabwd_Window_UseObj,
                        *wpedabwd_Window_CancelObj,
                        *wpedabwd_Window_BackgroundGrpObj,
                        *wpedabwd_Window_RenderModeGrpObj,
                        *wpedabwd_Window_RenderModeObj,
                        *wpedabwd_Window_RenderModePageObj,
                        *wpedabwd_Window_TileModeObjContainer,
                        *wpedabwd_Window_TileModeObj,
                        *wpedabwd_Window_XOffsetObj,
                        *wpedabwd_Window_YOffsetObj,
                        *wpedabwd_Window_IconLabel_LabelFont,
                        *wpedabwd_Window_IconLabel_InfoFont,
                        *wpedabwd_Window_Icon_ListMode,
                        *wpedabwd_Window_Icon_TextMode,
                        *wpedabwd_Window_IconLabel_MaxLineLen,
                        *wpedabwd_Window_Icon_SortMode,
                        *wpedabwd_Window_Icon_SortModeLabel,
                        *wpedabwd_Window_Icon_AutoSort,
                        *wpedabwd_Window_Icon_DragTransparent,
                        *wpedabwd_Window_Icon_ViewMode,
                        *wpedabwd_Window_Icon_ViewModeLabel,
#if defined(DEBUG_MULTLINE)
                        *wpedabwd_Window_IconLabel_TextMultiLine, 
                        *wpedabwd_Window_IconLabel_MultiLineonFocus, 
                        *wpedabwd_Window_IconLabel_MultiLineNo,     
#endif
                        *wpedabwd_Window_Icon_HorSpacing,
                        *wpedabwd_Window_Icon_VertSpacing,
                        *wpedabwd_Window_Icon_ImageSpacing,
                        *wpedabwd_Window_IconLabel_HorPadd,
                        *wpedabwd_Window_IconLabel_VertPadd,
                        *wpedabwd_Window_IconLabel_BorderWidth,
                        *wpedabwd_Window_IconLabel_BorderHeight;

    struct List         wpedabwd_Window_AdvancedViewRenderModes;
    struct List         wpedabwd_Window_AdvancedViewTileModes;
    struct List         wpedabwd_Window_AdvancedViewIconListModes;
    struct List         wpedabwd_Window_AdvancedViewIconTextRenderModes;

    struct Hook         wpedabwd_Hook_DrawModeChage;
};

struct WPEditor_DATA
{
    struct WPEditor_AdvancedBackgroundWindow_DATA       *wped_AdvancedViewSettings_WindowData;

    struct WPEditor_ViewSettingsObject                  *wped_ViewSettings_Current;

    Object                                              *wped_FirstBGImSpecObj,
                                                        *wped_FirstBGAdvancedObj;    

    ULONG                                               wped_DimensionsSet;

    Object                                              *wped_ViewSettings_GroupObj,
                                                        *wped_ViewSettings_SpacerObj,
                                                        *wped_c_NavigationMethod,
                                                        *wped_cm_ToolbarEnabled, 
#if defined(DEBUG_CHANGESCREENTITLE)
                                                        *wped_s_screentitle, 
#endif
                                                        *wped_toolbarpreview,
#if defined(DEBUG_NETWORKBROWSER)
                                                        *wped_cm_EnableNetworkBrowser, 
#endif
#if defined(DEBUG_SHOWUSERFILES)
                                                        *wped_cm_EnableUserFiles, 
#endif
                                                        *wped_toolbarGroup;
    struct Hook                                         wped_Hook_CloseAdvancedOptions;
};

//static struct Hook navichangehook;
static STRPTR        _wpeditor_intern_NavigationModes[3];

static STRPTR        _wpeditor_intern_IconListModeNames[iconlist_ListViewModesCount + 1];
static ULONG         _wpeditor_intern_IconListModeIDs[iconlist_ListViewModesCount + 1];

static STRPTR        _wpeditor_intern_IconTextRenderModeNames[iconlist_LabelRenderModesCount + 1];
static ULONG         _wpeditor_intern_IconTextRenderModeIDs[iconlist_LabelRenderModesCount + 1];

static STRPTR        _wpeditor_intern_IconSortModeNames[4];
static STRPTR        _wpeditor_intern_IconViewModeNames[3];

static STRPTR        _wpeditor_intern_MainPageNames[4];
static STRPTR        _wpeditor_intern_AdvancedPageNames[4];

static Class         *_wpeditor_intern_CLASS = NULL;
static struct List   _wpeditor_intern_ViewSettings;

static CONST_STRPTR  toolbar_PrefsFile = "SYS/Wanderer/toolbar.prefs";
#define TOOLBAR_PREFSSIZE 1024

/*** Macros *****************************************************************/
#define SETUP_WPEDITOR_INST_DATA struct WPEditor_DATA *data = INST_DATA(CLASS, self)

/*** Misc Helper functions **************************************************/
/* 32bit replacements for utility.library tag funcs */
struct TagItem32 *  NextTag32Item(const struct TagItem32 ** tagListPtr)
{
    if(!(*tagListPtr)) return NULL;

    while (TRUE)
    {
    switch ((*tagListPtr)->ti_Tag)
        {
        case TAG_IGNORE:
            break;

        case TAG_END:
            (*tagListPtr) = NULL;
            return NULL;

        case TAG_SKIP:
            (*tagListPtr) += (*tagListPtr)->ti_Data + 1;
            continue;

        default:
            /* Use post-increment (return will return the current value and
            then tagListPtr will be incremented) */
            return (struct TagItem32 *)(*tagListPtr)++;
    }

    (*tagListPtr) ++;
    }
}

struct TagItem32 * FindTag32Item(ULONG tagValue, const struct TagItem32 *tagList)
{
    struct TagItem32       *tag;
    const struct TagItem32 *tagptr = tagList;

    while((tag = NextTag32Item(&tagptr)))
    {
        if(tag->ti_Tag == tagValue) return tag;
    }

    return NULL;

}

IPTR GetTag32Data(ULONG tagValue, ULONG defaultVal, const struct TagItem32 *tagList)
{
    struct TagItem32 *ti = NULL;

    if ((tagList != NULL) && (ti = FindTag32Item(tagValue, tagList)))
    return ti->ti_Data;

    return defaultVal;
}

IPTR GetRenderModePage(char *rendermode_name, STRPTR __RenderModeNames[], IPTR __DrawModePages[])
{
    int i = 0;

    for (i = 0; i < WP_MAX_RENDER_MODES; i++)
    {
        if (strcmp(__RenderModeNames[i], rendermode_name) == 0)
        {
            return __DrawModePages[i];
        }
    }
    return -1;
}

IPTR SetViewSettingTag32(struct TagItem32 *this_Taglist, ULONG Tag_ID, ULONG newTag_Value)
{
    int i = 0;
    do
    {
        if (this_Taglist[i].ti_Tag == TAG_DONE)
            break;
        else if (this_Taglist[i].ti_Tag == Tag_ID)
        {
            if (this_Taglist[i].ti_Data == newTag_Value) return TRUE;
            this_Taglist[i].ti_Data = newTag_Value;
            return TRUE;
        }
        i ++;
    }while(i < WP_MAX_BG_TAG_COUNT);

    return FALSE;
}

/*** Hook functions *********************************************************/

AROS_UFH3(
    void, WandererPrefs_Hook_OpenAdvancedOptionsFunc,
    AROS_UFHA(struct Hook *,    hook,   A0),
    AROS_UFHA(APTR *,           obj,    A2),
    AROS_UFHA(APTR,             param,  A1)
)
{
    AROS_USERFUNC_INIT

    Object                             *self= (Object *)obj;
    struct WPEditor_ViewSettingsObject *_viewSettings_Current = *(struct WPEditor_ViewSettingsObject **)param;
    Class                              *CLASS = _wpeditor_intern_CLASS;
    UBYTE                               *ImageSelector_Spec = NULL;
    char                                *Image_Spec = NULL;

    struct WPEditor_DATA *data = INST_DATA(CLASS, self);

    data->wped_ViewSettings_Current = _viewSettings_Current;

    GET(_viewSettings_Current->wpedbo_ImageSpecObject, MUIA_Imagedisplay_Spec, &ImageSelector_Spec);

    Image_Spec = AllocVec(strlen(ImageSelector_Spec) + 1, MEMF_ANY|MEMF_CLEAR);
    if (Image_Spec)
    {
        strcpy(Image_Spec, ImageSelector_Spec);
        /* This "reloads" the dynamic controls by calling WandererPrefs_Hook_CheckImageFunc */
        SET(_viewSettings_Current->wpedbo_ImageSpecObject, MUIA_Imagedisplay_Spec, Image_Spec);

        struct WPEditor_ViewSettingsObject *_viewSettings_Node = NULL;

        ForeachNode(&_wpeditor_intern_ViewSettings, _viewSettings_Node)
        {
            GET(_viewSettings_Node->wpedbo_AdvancedOptionsObject, 
                MUIA_Disabled, 
                &_viewSettings_Node->wpedbo_state_AdvancedDisabled);

            SET(_viewSettings_Node->wpedbo_ImageSpecObject, MUIA_Disabled, TRUE);//1_Disable
            SET(_viewSettings_Node->wpedbo_AdvancedOptionsObject, MUIA_Disabled, TRUE);//2_Disable
            if (_viewSettings_Node == _viewSettings_Current)
            {
                ULONG current_SortFlags = GetTag32Data(MUIA_IconList_SortFlags,
                        (MUIV_IconList_Sort_AutoSort | MUIV_IconList_Sort_ByName), _viewSettings_Node->wpedbo_Options);
                ULONG current_ViewFlags = GetTag32Data(MUIA_IconList_DisplayFlags,
                        ICONLIST_DISP_SHOWINFO, _viewSettings_Node->wpedbo_Options);


                D(bug("[WPEditor] WandererPrefs_Hook_OpenAdvancedOptionsFunc: Found ViewSettings chunk for node we are editing\n"));
                if (XGET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_BackgroundGrpObj, MUIA_ShowMe) == TRUE)
                {
                    ULONG current_RenderMode = GetTag32Data(MUIA_IconWindowExt_ImageBackFill_BGRenderMode, IconWindowExt_ImageBackFill_RenderMode_Tiled, _viewSettings_Node->wpedbo_Options);
                    int i;

                    for (i = 0; i < data->wped_ViewSettings_Current->wpedbo_BFRenderModeCount; i++)
                    {
                        if ((ULONG)GetEntryElementID(data->wped_ViewSettings_Current->wpedbo_BFRenderModeEntries[i]) == current_RenderMode)
                        {
                            SET
                            (
                                (Object *)data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_RenderModeObj,
                                MUIA_Cycle_Active, i
                            );
                            SET
                            (
                                (Object *)data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_RenderModePageObj,
                                MUIA_Group_ActivePage, data->wped_ViewSettings_Current->wpedbo_BFRenderModePages[i]
                            );
                        }
                    }

//D(bug("[WPEditor] WandererPrefs_Hook_CloseAdvancedOptionsFunc: Render Mode '%s'\n", data->wped_ViewSettings_Current->wpedbo_BFRenderModeNames[current_rendermodepage]));

                    if (current_RenderMode == IconWindowExt_ImageBackFill_RenderMode_Tiled)
                    {

                        ULONG current_TileMode = GetTag32Data(MUIA_IconWindowExt_ImageBackFill_BGTileMode, IconWindowExt_ImageBackFill_TileMode_Float, _viewSettings_Node->wpedbo_Options);

                        for (i = 0; i < data->wped_ViewSettings_Current->wpedbo_BFTileModeCount; i++)
                        {
                            if ((ULONG)GetEntryElementID(data->wped_ViewSettings_Current->wpedbo_BFTileModeEntries[i]) == current_TileMode)
                            {
                                SET
                                (
                                    data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_TileModeObj,
                                    MUIA_Cycle_Active, i
                                );
                            }
                        }

                    }

                    SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_XOffsetObj, MUIA_String_Integer, GetTag32Data(MUIA_IconWindowExt_ImageBackFill_BGXOffset, 0, _viewSettings_Current->wpedbo_Options));
                    SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_YOffsetObj, MUIA_String_Integer, GetTag32Data(MUIA_IconWindowExt_ImageBackFill_BGYOffset, 0, _viewSettings_Current->wpedbo_Options));
                }
#if defined(DEBUG_PERVIEWFONTS)
//                SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_IconLabel_LabelFont,
//                SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_IconLabel_InfoFont,
#endif
                SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_Icon_ListMode, MUIA_Cycle_Active, GetTag32Data(MUIA_IconList_IconListMode, ICON_LISTMODE_GRID, _viewSettings_Current->wpedbo_Options));
                SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_Icon_TextMode, MUIA_Cycle_Active, GetTag32Data(MUIA_IconList_LabelText_Mode, ICON_TEXTMODE_OUTLINE, _viewSettings_Current->wpedbo_Options));
                SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_IconLabel_MaxLineLen, MUIA_String_Integer, GetTag32Data(MUIA_IconList_LabelText_MaxLineLen, ILC_ICONLABEL_MAXLINELEN_DEFAULT, _viewSettings_Current->wpedbo_Options));
#if defined(DEBUG_MULTLINE)
                SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_IconLabel_MultiLineonFocus, MUIA_Selected, GetTag32Data(MUIA_IconList_LabelText_MultiLineOnFocus, FALSE, _viewSettings_Current->wpedbo_Options));
                LONG MultiLineNo = GetTag32Data(MUIA_IconList_LabelText_MultiLine, 1, _viewSettings_Current->wpedbo_Options);
                if (MultiLineNo < 1) MultiLineNo = 1;
                if (MultiLineNo > 1)
                {
                    SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_IconLabel_TextMultiLine, MUIA_Selected, TRUE);
                    SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_IconLabel_MultiLineNo, MUIA_Disabled, FALSE);
                }
                else
                {
                    SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_IconLabel_TextMultiLine, MUIA_Selected, FALSE);
                    SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_IconLabel_MultiLineNo, MUIA_Disabled, TRUE);
                }
                SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_IconLabel_MultiLineNo, MUIA_String_Integer, MultiLineNo);
#endif
                SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_Icon_HorSpacing, MUIA_String_Integer, GetTag32Data(MUIA_IconList_Icon_HorizontalSpacing, ILC_ICON_HORIZONTALMARGIN_DEFAULT, _viewSettings_Current->wpedbo_Options));
                SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_Icon_VertSpacing, MUIA_String_Integer, GetTag32Data(MUIA_IconList_Icon_VerticalSpacing, ILC_ICON_VERTICALMARGIN_DEFAULT, _viewSettings_Current->wpedbo_Options));
                SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_Icon_ImageSpacing, MUIA_String_Integer, GetTag32Data(MUIA_IconList_Icon_ImageSpacing, ILC_ICONLABEL_IMAGEMARGIN_DEFAULT, _viewSettings_Current->wpedbo_Options));
                SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_IconLabel_HorPadd, MUIA_String_Integer, GetTag32Data(MUIA_IconList_LabelText_HorizontalPadding, ILC_ICONLABEL_HORIZONTALTEXTMARGIN_DEFAULT, _viewSettings_Current->wpedbo_Options));
                SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_IconLabel_VertPadd, MUIA_String_Integer, GetTag32Data(MUIA_IconList_LabelText_VerticalPadding, ILC_ICONLABEL_VERTICALTEXTMARGIN_DEFAULT, _viewSettings_Current->wpedbo_Options));
                SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_IconLabel_BorderWidth, MUIA_String_Integer, GetTag32Data(MUIA_IconList_LabelText_BorderWidth, ILC_ICONLABEL_BORDERWIDTH_DEFAULT, _viewSettings_Current->wpedbo_Options));
                SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_IconLabel_BorderHeight, MUIA_String_Integer, GetTag32Data(MUIA_IconList_LabelText_BorderHeight, ILC_ICONLABEL_BORDERHEIGHT_DEFAULT, _viewSettings_Current->wpedbo_Options));

                SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_Icon_AutoSort, MUIA_Selected, !!(current_SortFlags & MUIV_IconList_Sort_AutoSort));
                if (current_SortFlags & MUIV_IconList_Sort_ByName)
                    SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_Icon_SortMode, MUIA_Cycle_Active, 0);
                if (current_SortFlags & MUIV_IconList_Sort_ByDate)
                    SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_Icon_SortMode, MUIA_Cycle_Active, 1);
                if (current_SortFlags & MUIV_IconList_Sort_BySize)
                    SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_Icon_SortMode, MUIA_Cycle_Active, 2);

                SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_Icon_DragTransparent, MUIA_Selected, GetTag32Data(MUIA_IconList_DragImageTransparent, FALSE, _viewSettings_Current->wpedbo_Options));

                if (current_ViewFlags & ICONLIST_DISP_SHOWINFO)
                    SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_Icon_ViewMode, MUIA_Cycle_Active, WPD_ICONVIEWMODE_ICONS);
                else
                    SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_Icon_ViewMode, MUIA_Cycle_Active, WPD_ICONVIEWMODE_ALL);
            }
        }

        /*Enable this and remove *_Disable instructions over 
          when discovered the zune refresh problem...*/
        //SET(data->wped_ViewSettings_GroupObj, MUIA_Disabled, TRUE);

        SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_WindowObj, MUIA_Window_Open, TRUE);
        FreeVec(Image_Spec);
    }

    AROS_USERFUNC_EXIT
}

#define SETCURRENTVIEWSETTINGSTAG(tag, obj, objtag)                                                                 \
    success = SetViewSettingTag32(data->wped_ViewSettings_Current->wpedbo_Options, tag, XGET(obj, objtag));         \
    if (success == FALSE)                                                                                           \
    {                                                                                                               \
        /* TODO: Allocate extra storage for our tags.. */                                                           \
    }                                                                                                               \
    else if (success == TRUE)                                                                                       \
    {                                                                                                               \
        settings_changed = TRUE;                                                                                    \
    }

AROS_UFH3(
    void, WandererPrefs_Hook_CloseAdvancedOptionsFunc,
    AROS_UFHA(struct Hook *,    hook,   A0),
    AROS_UFHA(APTR *,           obj,    A2),
    AROS_UFHA(APTR,             param,  A1)
)
{
    AROS_USERFUNC_INIT

    Object                           *self = ( Object *)obj;
    BOOL                             use_settings = (BOOL) *( IPTR *)param;
    struct WPEditor_ViewSettingsObject *_viewSettings_Node = NULL;
    Class                            *CLASS  = _wpeditor_intern_CLASS;
    BOOL                             settings_changed = FALSE;
    BOOL                             success = FALSE;

    SETUP_WPEDITOR_INST_DATA;

D(bug("[WPEditor] WandererPrefs_Hook_CloseAdvancedOptionsFunc()\n"));
    
    /*Enable this and remove *_Enable instructions under 
      when discovered the zune refresh problem...*/
    //SET(data->wped_ViewSettings_GroupObj, MUIA_Disabled, FALSE);

    ForeachNode(&_wpeditor_intern_ViewSettings, _viewSettings_Node)
    {
        SET(_viewSettings_Node->wpedbo_ImageSpecObject, MUIA_Disabled, FALSE);
        SET(_viewSettings_Node->wpedbo_AdvancedOptionsObject, MUIA_Disabled, _viewSettings_Node->wpedbo_state_AdvancedDisabled);
    }

    if ((use_settings) && (data->wped_ViewSettings_Current))
    {
D(bug("[WPEditor] WandererPrefs_Hook_CloseAdvancedOptionsFunc: Updating tags for Background ..\n"));
        IPTR current_rendermodepage = 0;
        ULONG current_rendermode;

        GET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_RenderModeObj, MUIA_Cycle_Active, &current_rendermodepage);
        IPTR current_rendermode_entry = data->wped_ViewSettings_Current->wpedbo_BFRenderModeEntries[current_rendermodepage];

D(bug("[WPEditor] WandererPrefs_Hook_CloseAdvancedOptionsFunc: Render Mode '%s'\n", data->wped_ViewSettings_Current->wpedbo_BFRenderModeNames[current_rendermodepage]));
        current_rendermode = (ULONG)GetEntryElementID(current_rendermode_entry);
        if (current_rendermode == -1)
        {
D(bug("[WPEditor] WandererPrefs_Hook_CloseAdvancedOptionsFunc: ERROR: Unknown Render mode string!?!?!\n"));
        }
        else
        {
            success = SetViewSettingTag32(data->wped_ViewSettings_Current->wpedbo_Options, MUIA_IconWindowExt_ImageBackFill_BGRenderMode, current_rendermode);
            if (success == FALSE)
            {
D(bug("[WPEditor] WandererPrefs_Hook_CloseAdvancedOptionsFunc: No MUIA_IconWindowExt_ImageBackFill_BGRenderMode TAG - Adding ..\n"));
/* "TODO: Allocate extra storage for our tags.." */
            }
            else if (success == TRUE)
            {
                settings_changed = TRUE;
            }

            switch (current_rendermode)
            {
                case IconWindowExt_ImageBackFill_RenderMode_Scale:
                    break;

                case IconWindowExt_ImageBackFill_RenderMode_Tiled:
                    SETCURRENTVIEWSETTINGSTAG(MUIA_IconWindowExt_ImageBackFill_BGXOffset, data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_XOffsetObj, MUIA_String_Integer);
                    SETCURRENTVIEWSETTINGSTAG(MUIA_IconWindowExt_ImageBackFill_BGYOffset, data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_YOffsetObj, MUIA_String_Integer);


                    IPTR current_tilemodeno = 0;
                    ULONG current_tilemode = 0;

                    GET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_TileModeObj, MUIA_Cycle_Active, &current_tilemodeno);
                    IPTR current_tilemode_entry = data->wped_ViewSettings_Current->wpedbo_BFTileModeEntries[current_tilemodeno];

D(bug("[WPEditor] WandererPrefs_Hook_CloseAdvancedOptionsFunc: Tile Mode '%s'\n", data->wped_ViewSettings_Current->wpedbo_BFTileModeNames[current_tilemodeno]));
                    current_tilemode = (ULONG)GetEntryElementID(current_tilemode_entry);

                    success = SetViewSettingTag32(data->wped_ViewSettings_Current->wpedbo_Options, MUIA_IconWindowExt_ImageBackFill_BGTileMode, current_tilemode);
                    if (success == FALSE)
                    {
D(bug("[WPEditor] WandererPrefs_Hook_CloseAdvancedOptionsFunc: No MUIA_IconWindowExt_ImageBackFill_BGTileMode TAG - Adding ..\n"));
/* "TODO: Allocate extra storage for our tags.." */
                    }
                    else if (success == TRUE)
                    {
                        settings_changed = TRUE;
                    }
                    break;
            }
        }

        SETCURRENTVIEWSETTINGSTAG(MUIA_IconList_IconListMode, data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_Icon_ListMode, MUIA_Cycle_Active);
        SETCURRENTVIEWSETTINGSTAG(MUIA_IconList_LabelText_Mode, data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_Icon_TextMode, MUIA_Cycle_Active);
        SETCURRENTVIEWSETTINGSTAG(MUIA_IconList_LabelText_MaxLineLen, data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_IconLabel_MaxLineLen, MUIA_String_Integer);


#if defined(DEBUG_MULTLINE)
        SETCURRENTVIEWSETTINGSTAG(MUIA_IconList_LabelText_MultiLineOnFocus, data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_IconLabel_MultiLineonFocus, MUIA_Selected);
        SETCURRENTVIEWSETTINGSTAG(MUIA_IconList_LabelText_MultiLine, data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_IconLabel_MultiLineNo, MUIA_String_Integer);
#endif

        SETCURRENTVIEWSETTINGSTAG(MUIA_IconList_Icon_HorizontalSpacing, data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_Icon_HorSpacing, MUIA_String_Integer);
        SETCURRENTVIEWSETTINGSTAG(MUIA_IconList_Icon_VerticalSpacing, data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_Icon_VertSpacing, MUIA_String_Integer);
        SETCURRENTVIEWSETTINGSTAG(MUIA_IconList_Icon_ImageSpacing, data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_Icon_ImageSpacing, MUIA_String_Integer);
        SETCURRENTVIEWSETTINGSTAG(MUIA_IconList_LabelText_HorizontalPadding, data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_IconLabel_HorPadd, MUIA_String_Integer);
        SETCURRENTVIEWSETTINGSTAG(MUIA_IconList_LabelText_VerticalPadding, data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_IconLabel_VertPadd, MUIA_String_Integer);
        SETCURRENTVIEWSETTINGSTAG(MUIA_IconList_LabelText_BorderWidth, data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_IconLabel_BorderWidth, MUIA_String_Integer);
        SETCURRENTVIEWSETTINGSTAG(MUIA_IconList_LabelText_BorderHeight, data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_IconLabel_BorderHeight, MUIA_String_Integer);

        {
            ULONG new_SortFlags = 0;

            if (XGET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_Icon_AutoSort, MUIA_Selected) == TRUE)
                new_SortFlags |= MUIV_IconList_Sort_AutoSort;

            switch(XGET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_Icon_SortMode, MUIA_Cycle_Active))
            {
            case 0: new_SortFlags |= MUIV_IconList_Sort_ByName; break;
            case 1: new_SortFlags |= MUIV_IconList_Sort_ByDate; break;
            case 2: new_SortFlags |= MUIV_IconList_Sort_BySize; break;
            }

            success = SetViewSettingTag32(data->wped_ViewSettings_Current->wpedbo_Options, MUIA_IconList_SortFlags, new_SortFlags);
            if (success == FALSE)
            {
                D(bug("[WPEditor] WandererPrefs_Hook_CloseAdvancedOptionsFunc: No MUIA_IconList_SortFlags TAG - Adding ..\n"));
                /* TODO: Allocate extra storage for our tags.. */
            }
            else if (success == TRUE)
            {
                settings_changed = TRUE;
            }
        }

        SETCURRENTVIEWSETTINGSTAG(MUIA_IconList_DragImageTransparent, data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_Icon_DragTransparent, MUIA_Selected);

        if (XGET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_Icon_ViewMode, MUIA_Cycle_Active) == WPD_ICONVIEWMODE_ICONS)
            success = SetViewSettingTag32(data->wped_ViewSettings_Current->wpedbo_Options, MUIA_IconList_DisplayFlags, ICONLIST_DISP_SHOWINFO);
        else
            success = SetViewSettingTag32(data->wped_ViewSettings_Current->wpedbo_Options, MUIA_IconList_DisplayFlags, 0);
        if (success == FALSE)
        {
            D(bug("[WPEditor] WandererPrefs_Hook_CloseAdvancedOptionsFunc: No MUIA_IconList_DisplayFlags TAG - Adding ..\n"));
            /* TODO: Allocate extra storage for our tags.. */
        }
        else if (success == TRUE)
        {
            settings_changed = TRUE;
        }

        if (settings_changed) SET(self, MUIA_PrefsEditor_Changed, TRUE);
    }
    else
    {
D(bug("[WPEditor] WandererPrefs_Hook_CloseAdvancedOptionsFunc: Cancel selected\n"));
    }

    SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_WindowObj, MUIA_Window_Open, FALSE);

    AROS_USERFUNC_EXIT
}


AROS_UFH3(
    void, WandererPrefs_Hook_DrawModeChangeFunc,
    AROS_UFHA(struct Hook *,    hook,   A0),
    AROS_UFHA(APTR *,           obj,    A2),
    AROS_UFHA(APTR,             param,  A1)
)
{
    AROS_USERFUNC_INIT

    Object                           *self            = ( Object *)obj;
    Class                            *CLASS           = *( Class **)param;

    SETUP_WPEDITOR_INST_DATA;

D(bug("[WPEditor] WandererPrefs_Hook_DrawModeChangeFunc()\n"));

    if (data->wped_ViewSettings_Current)
    {
        IPTR active_cycleentry = 0;
        GET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_RenderModeObj, MUIA_Cycle_Active, &active_cycleentry);

D(bug("[WPEditor] WandererPrefs_Hook_DrawModeChangeFunc: RenderMode Cycle Active = %d, Page = %d\n", active_cycleentry, data->wped_ViewSettings_Current->wpedbo_BFRenderModePages[active_cycleentry]));

        IPTR active_page = 0;
        GET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_RenderModePageObj, MUIA_Group_ActivePage, &active_page);

        if (active_page != data->wped_ViewSettings_Current->wpedbo_BFRenderModePages[active_cycleentry])
        {
            SET
            (
                (Object *)data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_RenderModePageObj,
                MUIA_Group_ActivePage, data->wped_ViewSettings_Current->wpedbo_BFRenderModePages[active_cycleentry]
            );
D(bug("[WPEditor] WandererPrefs_Hook_DrawModeChangeFunc: Changing Active Page to %d\n", data->wped_ViewSettings_Current->wpedbo_BFRenderModePages[active_cycleentry]));

        }
    }

    AROS_USERFUNC_EXIT
}

#define SETNEVSOPTION(tag, defvalue)                                                                                    \
    newVS_Options[newVS_OptionCount].ti_Tag = tag;                                                                      \
    if (_viewSettings_Current->wpedbo_Options)                                                                          \
        newVS_Options[newVS_OptionCount++].ti_Data = GetTag32Data(tag, defvalue, _viewSettings_Current->wpedbo_Options);\
    else                                                                                                                \
        newVS_Options[newVS_OptionCount++].ti_Data = defvalue;

AROS_UFH3(
    void, WandererPrefs_Hook_CheckImageFunc,
    AROS_UFHA(struct Hook *,    hook,   A0),
    AROS_UFHA(APTR *,           obj,    A2),
    AROS_UFHA(APTR,             param,  A1)
)
{
    AROS_USERFUNC_INIT

    Object *self = (Object *) obj;
    struct WPEditor_ViewSettingsObject *_viewSettings_Current = *( struct WPEditor_ViewSettingsObject **)param;
    Class *CLASS = _wpeditor_intern_CLASS;

    SETUP_WPEDITOR_INST_DATA;
    
    UBYTE *ImageSelector_Spec = NULL;

    GET(_viewSettings_Current->wpedbo_ImageSpecObject, MUIA_Imagedisplay_Spec, &ImageSelector_Spec);

D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: Object @ 0x%p reports image spec '%s'\n", _viewSettings_Current->wpedbo_ImageSpecObject, (char *)ImageSelector_Spec));

    IPTR  this_Background_type = (IPTR)(ImageSelector_Spec[0] - 48);

    _viewSettings_Current->wpedbo_Type = this_Background_type;
    data->wped_ViewSettings_Current = _viewSettings_Current;

    if (_viewSettings_Current->wpedbo_AdvancedOptionsObject)
    {
        struct TagItem32   newVS_Options[WP_MAX_BG_TAG_COUNT];
        IPTR               newVS_OptionCount = 0, BFTileModeCount = 0;
        IPTR               newBG_RenderModeCount = 0;
        IPTR               newBG_RenderModePages[WP_DRAWMODE_COUNT];
        STRPTR             newBG_RenderModes[WP_DRAWMODE_COUNT];

        if ((_viewSettings_Current->wpedbo_Type == 5)||(_viewSettings_Current->wpedbo_Type == 0))
        {
D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: Image-type spec (%d) - Enabling Advanced Image options ..\n", _viewSettings_Current->wpedbo_Type));
            SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_BackgroundGrpObj, MUIA_ShowMe, TRUE);

            for (newVS_OptionCount = 0; newVS_OptionCount < WP_MAX_BG_TAG_COUNT; newVS_OptionCount++)
            {
                newVS_Options[newVS_OptionCount].ti_Tag = TAG_DONE;
            }
            newVS_OptionCount = 0;

            if (_viewSettings_Current->wpedbo_Options)
            {
D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: Existing options @ 0x%p\n", _viewSettings_Current->wpedbo_Options));
            }

            IPTR DRAWMODEENTRY = 0;
            switch ((int)_viewSettings_Current->wpedbo_Type)
            {
                case 5:
                {
                    if ((strcmp(_viewSettings_Current->wpedbo_ViewName, "Workbench")) == 0)
                    {
                        DRAWMODEENTRY = EntryElementFindNode(&data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_AdvancedViewRenderModes, IconWindowExt_ImageBackFill_RenderMode_Scale);
                        newBG_RenderModes[newBG_RenderModeCount] = (APTR)GetEntryElementName(DRAWMODEENTRY);
                        newBG_RenderModePages[newBG_RenderModeCount++] = 0;

D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: DrawMode %d = '%s'\n", newBG_RenderModeCount -1, newBG_RenderModes[newBG_RenderModeCount-1]));
                    }
                }
                case 0:
                {
                    DRAWMODEENTRY = EntryElementFindNode(&data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_AdvancedViewRenderModes, IconWindowExt_ImageBackFill_RenderMode_Tiled);
                    newBG_RenderModes[newBG_RenderModeCount] = (APTR)GetEntryElementName((IPTR)DRAWMODEENTRY);
                    newBG_RenderModePages[newBG_RenderModeCount++] = 1;

                    IPTR old_bg_tilemodes = (IPTR)data->wped_ViewSettings_Current->wpedbo_BFTileModeNames;
                    IPTR old_bg_tilemodeentries = (IPTR)data->wped_ViewSettings_Current->wpedbo_BFTileModeEntries;

D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: Old TileModes @ 0x%p, Entries @ 0x%p\n", old_bg_tilemodes, old_bg_tilemodeentries));

                    data->wped_ViewSettings_Current->wpedbo_BFTileModeNames = NULL;
                    data->wped_ViewSettings_Current->wpedbo_BFTileModeEntries = NULL;

                    data->wped_ViewSettings_Current->wpedbo_BFTileModeCount = EntryElementCount(&data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_AdvancedViewTileModes);

                    data->wped_ViewSettings_Current->wpedbo_BFTileModeNames = AllocVec((sizeof(STRPTR) * (data->wped_ViewSettings_Current->wpedbo_BFTileModeCount + 1)), MEMF_ANY|MEMF_CLEAR);
                    data->wped_ViewSettings_Current->wpedbo_BFTileModeEntries = AllocVec((sizeof(IPTR) * data->wped_ViewSettings_Current->wpedbo_BFTileModeCount), MEMF_ANY|MEMF_CLEAR);

D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: Allocated %d TileModes  - Array @ 0x%p, Entries @ 0x%p\n", data->wped_ViewSettings_Current->wpedbo_BFTileModeCount, data->wped_ViewSettings_Current->wpedbo_BFTileModeNames, data->wped_ViewSettings_Current->wpedbo_BFTileModeEntries));
                    struct Node *tilelist_entry = NULL;

                    ForeachNode(&data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_AdvancedViewTileModes, tilelist_entry)
                    {
                        data->wped_ViewSettings_Current->wpedbo_BFTileModeNames[BFTileModeCount] = GetEntryElementName((IPTR)tilelist_entry);
                        data->wped_ViewSettings_Current->wpedbo_BFTileModeEntries[BFTileModeCount] = (IPTR)tilelist_entry;
                        BFTileModeCount++;
                    }

                    Object *new_TileModeObj = NULL;

D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: Creating TileModes Cycle gadget\n"));
                    new_TileModeObj = MUI_MakeObject(MUIO_Cycle, 
                                       NULL, 
                                       data->wped_ViewSettings_Current->wpedbo_BFTileModeNames
                                      );

                    if (new_TileModeObj)
                    {
D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: Object @ 0x%p\n", new_TileModeObj));
                        if (DoMethod(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_TileModeObjContainer, MUIM_Group_InitChange))
                        {
                            DoMethod(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_TileModeObjContainer, OM_REMMEMBER, data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_TileModeObj);
                            DoMethod(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_TileModeObjContainer, OM_ADDMEMBER, new_TileModeObj);
                            DoMethod(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_TileModeObjContainer, MUIM_Group_ExitChange);
                            
                            data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_TileModeObj = new_TileModeObj;
                        }
                    }

                    if (old_bg_tilemodes)
                        FreeVec((APTR)old_bg_tilemodes);

                    if (old_bg_tilemodeentries)
                        FreeVec((APTR)old_bg_tilemodeentries);
                    
                    SETNEVSOPTION(MUIA_IconWindowExt_ImageBackFill_BGRenderMode, IconWindowExt_ImageBackFill_RenderMode_Tiled);
                    SETNEVSOPTION(MUIA_IconWindowExt_ImageBackFill_BGTileMode, IconWindowExt_ImageBackFill_TileMode_Float);
                    SETNEVSOPTION(MUIA_IconWindowExt_ImageBackFill_BGXOffset, 0);
                    SETNEVSOPTION(MUIA_IconWindowExt_ImageBackFill_BGYOffset, 0);

D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: DrawMode %d = '%s'\n", newBG_RenderModeCount -1, newBG_RenderModes[newBG_RenderModeCount-1]));

                    break;
                }
            }
        }
        else
        {
            SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_BackgroundGrpObj, MUIA_ShowMe, FALSE);
        }

        SETNEVSOPTION(MUIA_IconList_IconListMode, ICON_LISTMODE_GRID);
        SETNEVSOPTION(MUIA_IconList_LabelText_Mode, ICON_TEXTMODE_OUTLINE);
        SETNEVSOPTION(MUIA_IconList_SortFlags, (MUIV_IconList_Sort_AutoSort | MUIV_IconList_Sort_ByName));
        SETNEVSOPTION(MUIA_IconList_DragImageTransparent, FALSE);
        SETNEVSOPTION(MUIA_IconList_DisplayFlags, ICONLIST_DISP_SHOWINFO);
        SETNEVSOPTION(MUIA_IconList_LabelText_MaxLineLen, ILC_ICONLABEL_MAXLINELEN_DEFAULT);

#if defined(DEBUG_MULTLINE)
        SETNEVSOPTION(MUIA_IconList_LabelText_MultiLine, 1);
        SETNEVSOPTION(MUIA_IconList_LabelText_MultiLineOnFocus, FALSE);
#endif

        SETNEVSOPTION(MUIA_IconList_Icon_HorizontalSpacing, ILC_ICON_HORIZONTALMARGIN_DEFAULT);
        SETNEVSOPTION(MUIA_IconList_Icon_VerticalSpacing, ILC_ICON_VERTICALMARGIN_DEFAULT);
        SETNEVSOPTION(MUIA_IconList_Icon_ImageSpacing, ILC_ICONLABEL_IMAGEMARGIN_DEFAULT);
        SETNEVSOPTION(MUIA_IconList_LabelText_HorizontalPadding, ILC_ICONLABEL_HORIZONTALTEXTMARGIN_DEFAULT);
        SETNEVSOPTION(MUIA_IconList_LabelText_VerticalPadding, ILC_ICONLABEL_VERTICALTEXTMARGIN_DEFAULT);
        SETNEVSOPTION(MUIA_IconList_LabelText_BorderWidth, ILC_ICONLABEL_BORDERWIDTH_DEFAULT);
        SETNEVSOPTION(MUIA_IconList_LabelText_BorderHeight, ILC_ICONLABEL_BORDERHEIGHT_DEFAULT);

        if (newVS_OptionCount > 0)
        {
            IPTR old_bg_options = (IPTR) _viewSettings_Current->wpedbo_Options,
                 VSOptionCountCount = 0;

            _viewSettings_Current->wpedbo_Options = NULL;

            _viewSettings_Current->wpedbo_Options = AllocVec((sizeof(struct TagItem32) * (newVS_OptionCount + 1)), MEMF_ANY|MEMF_CLEAR);
            for (VSOptionCountCount = 0; VSOptionCountCount < newVS_OptionCount; VSOptionCountCount++)
            {
                _viewSettings_Current->wpedbo_Options[VSOptionCountCount].ti_Tag = newVS_Options[VSOptionCountCount].ti_Tag;
                _viewSettings_Current->wpedbo_Options[VSOptionCountCount].ti_Data = newVS_Options[VSOptionCountCount].ti_Data;
            }

            if (old_bg_options)
                FreeVec((struct TagItem32 *)old_bg_options);
        }

        if (newBG_RenderModeCount > 0)
        {
            IPTR old_bg_drawmodes = (IPTR)data->wped_ViewSettings_Current->wpedbo_BFRenderModeNames;
            IPTR old_bg_drawpages = (IPTR)data->wped_ViewSettings_Current->wpedbo_BFRenderModePages;
            IPTR BFRenderModeCount = 0;

D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: Old RenderModes @ 0x%p, pages @ 0x%p\n", old_bg_drawmodes, old_bg_drawpages));

            data->wped_ViewSettings_Current->wpedbo_BFRenderModeNames = NULL;
            data->wped_ViewSettings_Current->wpedbo_BFRenderModePages = NULL;
            data->wped_ViewSettings_Current->wpedbo_BFRenderModeEntries = NULL;

            data->wped_ViewSettings_Current->wpedbo_BFRenderModeNames = AllocVec((sizeof(STRPTR) * (newBG_RenderModeCount + 1)), MEMF_ANY|MEMF_CLEAR);
            data->wped_ViewSettings_Current->wpedbo_BFRenderModePages = AllocVec((sizeof(IPTR) * newBG_RenderModeCount), MEMF_ANY|MEMF_CLEAR);
            data->wped_ViewSettings_Current->wpedbo_BFRenderModeEntries = AllocVec((sizeof(IPTR) * newBG_RenderModeCount), MEMF_ANY|MEMF_CLEAR);
            data->wped_ViewSettings_Current->wpedbo_BFRenderModeCount = newBG_RenderModeCount;
            
D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: Allocated %d RenderModes  - Array @ 0x%p, page mappings @ 0x%p\n", data->wped_ViewSettings_Current->wpedbo_BFRenderModeCount, data->wped_ViewSettings_Current->wpedbo_BFRenderModeNames, data->wped_ViewSettings_Current->wpedbo_BFRenderModePages));

            for (BFRenderModeCount = 0; BFRenderModeCount < data->wped_ViewSettings_Current->wpedbo_BFRenderModeCount; BFRenderModeCount++)
            {
                data->wped_ViewSettings_Current->wpedbo_BFRenderModeNames[BFRenderModeCount] = (IPTR)newBG_RenderModes[BFRenderModeCount];
                data->wped_ViewSettings_Current->wpedbo_BFRenderModePages[BFRenderModeCount] = newBG_RenderModePages[BFRenderModeCount];
                data->wped_ViewSettings_Current->wpedbo_BFRenderModeEntries[BFRenderModeCount] = EntryElementFindNamedNode(&data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_AdvancedViewRenderModes, newBG_RenderModes[BFRenderModeCount]);
            }

            Object *new_RenderModeObj = NULL;

D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: Creating RenderModes Cycle gadget\n"));
            new_RenderModeObj = MUI_MakeObject(MUIO_Cycle, 
                               NULL, 
                               data->wped_ViewSettings_Current->wpedbo_BFRenderModeNames
                              );
            if (new_RenderModeObj)
            {
D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: Object @ 0x%p\n", new_RenderModeObj));
                if (DoMethod(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_RenderModeGrpObj, MUIM_Group_InitChange))
                {
                    DoMethod(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_RenderModeGrpObj, OM_REMMEMBER, data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_RenderModeObj);
                    DoMethod(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_RenderModeGrpObj, OM_ADDMEMBER, new_RenderModeObj);
                    DoMethod(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_RenderModeGrpObj, MUIM_Group_ExitChange);
                    
                    data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_RenderModeObj = new_RenderModeObj;
                    
                    DoMethod (
                        data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_RenderModeObj,
                        MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
                        (IPTR)self, 3, MUIM_CallHook,
                        &data->wped_AdvancedViewSettings_WindowData->wpedabwd_Hook_DrawModeChage, CLASS
                    );
                }
            }

            if (old_bg_drawmodes)
                FreeVec((APTR)old_bg_drawmodes);

            if (old_bg_drawpages)
                FreeVec((APTR)old_bg_drawpages);
        }

        if ((strcmp(_viewSettings_Current->wpedbo_ViewName, "Workbench")) == 0)
        {
            SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_Icon_SortModeLabel, MUIA_ShowMe, FALSE);
            SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_Icon_SortMode, MUIA_ShowMe, FALSE);
            SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_Icon_ViewModeLabel, MUIA_ShowMe, FALSE);
            SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_Icon_ViewMode, MUIA_ShowMe, FALSE);
        }
        else
        {
            SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_Icon_SortModeLabel, MUIA_ShowMe, TRUE);
            SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_Icon_SortMode, MUIA_ShowMe, TRUE);
            SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_Icon_ViewModeLabel, MUIA_ShowMe, TRUE);
            SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_Icon_ViewMode, MUIA_ShowMe, TRUE);
        }
    }
    else
    {
D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: xxxxxxx - Disabling Advanced options ..\n"));
        SET(_viewSettings_Current->wpedbo_AdvancedOptionsObject, MUIA_Disabled, TRUE);
    }
    SET(self, MUIA_PrefsEditor_Changed, TRUE);

    AROS_USERFUNC_EXIT
}

struct WPEditor_ViewSettingsObject *WPEditor__FindViewSettingObjects(char * this_Name)
{
    struct WPEditor_ViewSettingsObject *_viewSettings_Current = NULL;

    ForeachNode(&_wpeditor_intern_ViewSettings, _viewSettings_Current)
    {
        if ((strcmp(_viewSettings_Current->wpedbo_ViewName, this_Name)) == 0) return _viewSettings_Current;
    }
    return NULL;
}

/*WPEditor__NewViewSettingObjects(): Add new ViewSetting object to
 *scrollgroup in left part of Appearance page;
 *It wants the name of new ViewSetting object, and if this 
 *ViewSetting object supports a backfill;
 *The function creates a new 50x50 MUIC_Popimage (a private class
 *which is composed by an  imagebutton connected to a setting
 *window) which has "MSG_SELECT_WORKBENCH_BACKGROUND" as window 
 *title;
 *Image contained in MUIC_Popimage is going to be inserted by a call
 *WPEditor__MUIM_PrefsEditor_ImportFH() during the initialization
 *of WPEditor Zune object and when these objects must be repainted;
 *If backfillsupport is TRUE a button labelled "Advanced" is 
 *created and hooked to WandererPrefs_Hook_CheckImageFunc();
 *The new MUIC_Popimage and eventually an "Advanced" button
 *are added as attributes to a ViewSetting object, and this last one
 *is added as node to the global struct _wpeditor_intern_ViewSettings;
 *The function returns the new ViewSetting object or NULL if
 *it failed;
 */
struct WPEditor_ViewSettingsObject *WPEditor__NewViewSettingObjects(char * this_Name, BOOL backfillsupport)
{
    struct WPEditor_ViewSettingsObject *_viewSettings_Current = NULL;

D(bug("[WPEditor] WPEditor__NewViewSettingObjects('%s')\n", this_Name));
    _viewSettings_Current = AllocMem(sizeof(struct WPEditor_ViewSettingsObject), MEMF_ANY|MEMF_CLEAR);
    if (_viewSettings_Current)
    {    
        _viewSettings_Current->wpedbo_ViewName = AllocVec(strlen(this_Name) + 1, MEMF_ANY|MEMF_CLEAR);
        if (_viewSettings_Current->wpedbo_ViewName)
        {
            strcpy(_viewSettings_Current->wpedbo_ViewName, this_Name);

            _viewSettings_Current->wpedbo_ImageSpecObject = (Object *)PopimageObject,
                                        MUIA_FixWidth, 50,
                                        MUIA_FixHeight, 50,
                                        MUIA_Window_Title, __(MSG_SELECT_WORKBENCH_BACKGROUND),
                                        MUIA_Imageadjust_Type, MUIV_Imageadjust_Type_Background,
                                        MUIA_CycleChain,       1,
                                    End;

            if (_viewSettings_Current->wpedbo_ImageSpecObject)
            {
                if (backfillsupport)
                    _viewSettings_Current->wpedbo_AdvancedOptionsObject = SimpleButton(_(MSG_ADVANCED));

                _viewSettings_Current->wpedbo_Hook_CheckImage.h_Entry = ( HOOKFUNC )WandererPrefs_Hook_CheckImageFunc;
                AddTail(&_wpeditor_intern_ViewSettings, &_viewSettings_Current->wpedbo_Node);
D(bug("[WPEditor] WPEditor__NewViewSettingObjects: Successfully created\n"));
                return _viewSettings_Current;
            }
            FreeVec(_viewSettings_Current->wpedbo_ViewName);
        }

        FreeMem(_viewSettings_Current, sizeof(struct WPEditor_ViewSettingsObject));
        _viewSettings_Current = NULL;
    }
D(bug("[WPEditor] WPEditor__NewViewSettingObjects: Failed to create objects\n"));
    
    return NULL;
}

/*** Methods ****************************************************************/
Object *WPEditor__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    struct WPEditor_DATA *data = NULL;
    struct WPEditor_AdvancedBackgroundWindow_DATA  *advancedView_data = NULL;

    Object  *_WP_Navigation_HGrp1 = NULL,
            *_WP_Navigation_InnerHGrp1 = NULL,
            *_WP_Navigation_TypeObj = NULL,
            *_WP_Navigation_InnerHGrp2 = NULL,
#if defined(DEBUG_CHANGESCREENTITLE)
            *_WP_Navigation_HGrp2 =NULL,
            *_WP_Navigation_InnerHGrp3 = NULL,
            *_WP_Navigator_ScreenTitleObj = NULL,
#endif
            *_WP_NavigationObj = NULL,    
            *_WP_Appearance_GroupObj = NULL,
            *_WP_ViewSettings_GroupObj = NULL,
            *_WP_ViewSettings_SpacerObj = NULL,
            *_WP_ViewSettings_VirtGrpObj = NULL,
            *_WP_ViewSettings_ScrollGrpObj = NULL,
#if defined(DEBUG_NETWORKBROWSER)
            *_WP_NetworkBrowser_EnabledObj = NULL, 
#endif
#if defined(DEBUG_SHOWUSERFILES)
            *_WP_UserFiles_ShowFileFolderObj = NULL, 
#endif
            *_WP_Toolbar_EnabledObj = NULL,
            *_WP_Toolbar_GroupObj = NULL,
            *_WP_Toolbar_InnerGroupObj1 = NULL,
            *_WP_Toolbar_InnerGroupObj2 = NULL,
            *_WP_Toolbar_InnerGroupObj3 = NULL,
            *_WP_Toolbar_InnerGroupObj4 = NULL,
            *_WP_Toolbar_PreviewObj = NULL,
            *_WP_Toolbar_PreviewDirUpObj = NULL,
            *_WP_Toolbar_PreviewSearchObj = NULL,
            *_WP_Prefs_PageGroupObj = NULL;

    Object    *_WP_AdvancedViewWindow = NULL,
            *_WP_AdvancedViewPageGroupObj = NULL,
            *_WP_AdvancedViewWindowVGrp = NULL,
            *_WP_AdvancedViewBackgroundGrpObj = NULL,
            *_WP_AdvancedViewRenderModeGrpObj = NULL,
            *_WP_AdvancedViewRenderModeObj = NULL,
            *_WP_AdvancedView_RenderModePageObj = NULL,
            *_WP_AdvancedView_ScaleModeGrpObj = NULL,
            *_WP_AdvancedView_TileModeGrpObj = NULL,
            *_WP_AdvancedView_TileModeObjContainer = NULL,
            *_WP_AdvancedView_TileModeObj = NULL, 
            *_WP_AdvancedView_X_OffsetObj = NULL,
            *_WP_AdvancedView_Y_OffsetObj = NULL,
            *_WP_AdvancedView_IconRenderGrpObj = NULL,
            *_WP_AdvancedView_LabelRenderGrpObj = NULL,
            *_WP_AdvancedView_Icon_ListModeObj = NULL,
            *_WP_AdvancedView_Icon_TextModeObj = NULL,
            *_WP_AdvancedView_Icon_HorSpacingObj = NULL,
            *_WP_AdvancedView_Icon_VertSpacingObj = NULL,
            *_WP_AdvancedView_Icon_ImageSpacingObj = NULL,
            *_WP_AdvancedView_Icon_SortModeObj = NULL,
            *_WP_AdvancedView_Icon_SortModeLabelObj = NULL,
            *_WP_AdvancedView_Icon_DragTransparentObj = NULL,
            *_WP_AdvancedView_Icon_AutoSortObj = NULL,
            *_WP_AdvancedView_Icon_ViewModeObj = NULL,
            *_WP_AdvancedView_Icon_ViewModeLabelObj = NULL,
            *_WP_AdvancedView_IconLabel_LabelFontObj = NULL,
            *_WP_AdvancedView_IconLabel_InfoFontObj = NULL,
            *_WP_AdvancedView_IconLabel_MaxLineLenObj = NULL,
#if defined(DEBUG_MULTLINE)
            *_WP_AdvancedView_IconLabel_TextMultiLineObj = NULL,
            *_WP_AdvancedView_IconLabel_MultiLineonFocusObj = NULL,
            *_WP_AdvancedView_IconLabel_MultiLineNoObj = NULL,
#endif
            *_WP_AdvancedView_IconLabel_HorPaddObj = NULL,
            *_WP_AdvancedView_IconLabel_VertPaddObj = NULL,
            *_WP_AdvancedView_IconLabel_BorderWidthObj = NULL,
            *_WP_AdvancedView_IconLabel_BorderHeightObj = NULL,
            *_WP_AdvancedView_ButtonGrpObj = NULL,
            *_WP_AdvancedView_ButtonObj_Use = NULL,
            *_WP_AdvancedView_ButtonObj_Cancel = NULL;

/**/
D(bug("[WPEditor] WPEditor__OM_NEW()\n"));

        //Object *cm_searchenabled;

/*main window----------------------------------------------------------------*/
/*self : Window?-------------------------------------------------------------*/

    self = (Object *) DoSuperNewTags
                        (
                            CLASS, self, NULL,
                            MUIA_PrefsEditor_Name, __(MSG_NAME),
                            MUIA_PrefsEditor_Path, (IPTR) "SYS/Wanderer/global.prefs",
                            TAG_DONE
                        );

/*END self-------------------------------------------------------------------*/

/*_WP_Prefs_PageGroupObj = Object for handling multi (3) page groups---------*/
    _wpeditor_intern_MainPageNames[WPD_GENERAL] = (STRPTR)_(MSG_GENERAL);
    _wpeditor_intern_MainPageNames[WPD_APPEARANCE] = (STRPTR)_(MSG_APPEARANCE);
    _wpeditor_intern_MainPageNames[WPD_TOOLBAR] = (STRPTR)_(MSG_TOOLBAR);

    _WP_Prefs_PageGroupObj = (Object *)RegisterObject,
                    MUIA_Register_Titles, (IPTR) _wpeditor_intern_MainPageNames,      
                End;
/*END _WP_Prefs_PageGroupObj-------------------------------------------------*/

/*_WP_NavigationObj: "Navigation" page group---------------------------------*/

    _WP_NavigationObj = (Object *)GroupObject, End;

    _WP_Navigation_HGrp1 = (Object *)HGroup,                    // general 
                    MUIA_FrameTitle, __(MSG_NAVIGATION),
                    MUIA_Group_SameSize, TRUE,
                    MUIA_Frame, MUIV_Frame_Group,
                    MUIA_Group_Columns, 2,
                End;

    _WP_Navigation_InnerHGrp1 = (Object *)HGroup,
                    MUIA_Group_Columns, 2,
                    MUIA_Group_SameSize, FALSE,
                    Child, (IPTR) Label1(_(MSG_METHOD)),
                End;

    /*Navigation cycle button--------------------*/
    _wpeditor_intern_NavigationModes[WPD_NAVIGATION_CLASSIC] = (STRPTR)_(MSG_CLASSIC);
    _wpeditor_intern_NavigationModes[WPD_NAVIGATION_ENHANCED] = (STRPTR)_(MSG_ENHANCED);
    _WP_Navigation_TypeObj = MUI_MakeObject(MUIO_Cycle, NULL, _wpeditor_intern_NavigationModes);

    /*END Navigation cycle button----------------*/
    _WP_Navigation_InnerHGrp2 = (Object *)HGroup,
                    MUIA_Group_Columns, 2,
                    MUIA_Group_SameSize, FALSE,
                End;

#if defined(DEBUG_SHOWUSERFILES)
    _WP_UserFiles_ShowFileFolderObj = MUI_MakeObject(MUIO_Checkmark,NULL);
#endif

#if defined(DEBUG_CHANGESCREENTITLE)
    _WP_Navigation_HGrp2 = (Object *)HGroup,                   
                    MUIA_FrameTitle, (IPTR)_(MSG_WANDERERSCREENTITLE),
                    MUIA_Group_SameSize, TRUE,
                    MUIA_Frame, MUIV_Frame_Group,
                End;

    _WP_Navigation_InnerHGrp3 = (Object *)HGroup, End;

    _WP_Navigator_ScreenTitleObj = (Object *)StringObject,
                    StringFrame,
                    MUIA_String_MaxLen, 256,
                    //MUIA_String_Contents, (IPTR)" ",
                End;
#endif
/*END _WP_NavigationObj------------------------------------------------------*/

/*_WP_Appearance_GroupObj: "Appearance" page group---------------------------*/

    _WP_Appearance_GroupObj = (Object *)GroupObject,                     // appearance 
                    MUIA_Group_SameSize, FALSE,
                    MUIA_Group_Horiz, TRUE,
                End;

    /*Left part of Appearance*/
    _WP_ViewSettings_ScrollGrpObj = (Object *)ScrollgroupObject,
                    MUIA_Group_SameSize, FALSE,
                    MUIA_Scrollgroup_FreeHoriz, FALSE,
                    MUIA_Scrollgroup_FreeVert, TRUE,
                    MUIA_Scrollgroup_Contents, 
                    (IPTR) (_WP_ViewSettings_VirtGrpObj = (Object *)VirtgroupObject,
                        MUIA_FrameTitle, (IPTR)_(MSG_VIEWSETTINGS),
                        MUIA_Frame, MUIV_Frame_ReadList,
                        MUIA_Virtgroup_Input, FALSE,
                    End),
                End;    

    /*_WP_ViewSettings_GroupObj is going to contain nodes 
      of list called _wpeditor_intern_ViewSettings, 
      it's created after in this function...;
    */
    _WP_ViewSettings_GroupObj = (Object *)GroupObject,
                    MUIA_Background, MUII_SHINE,
                    Child, (IPTR) (_WP_ViewSettings_SpacerObj = HVSpace),
                End;
    /**/

/*END _WP_Appearance_GroupObj------------------------------------------------*/

/*_WP_Toolbar_GroupObj: "Toolbar" page group---------------------------------*/
    
    _WP_Toolbar_GroupObj = (Object *)GroupObject,                     // toolbar 
                   MUIA_Group_SameSize, FALSE,
                End;
    
    _WP_Toolbar_InnerGroupObj1 = (Object *)HGroup,
                    MUIA_FrameTitle,  __(MSG_OBJECTS),
                    MUIA_Group_SameSize, TRUE,
                    MUIA_Frame, MUIV_Frame_Group,
                    MUIA_Group_Columns, 2,
                End;
    
    _WP_Toolbar_InnerGroupObj2 = (Object *)HGroup,
                    MUIA_Group_Columns, 2,
                    MUIA_Group_SameSize, FALSE,
                    Child, (IPTR)Label1(_(MSG_TOOLBAR_ENABLED)),
                End;
            
    _WP_Toolbar_EnabledObj = MUI_MakeObject(MUIO_Checkmark, NULL);

    _WP_Toolbar_InnerGroupObj3 = (Object *)HGroup,
                    MUIA_Group_Columns, 2,
                    MUIA_Group_SameSize, FALSE,
                    Child, (IPTR) HVSpace,
                    Child, (IPTR) HVSpace,
                    Child, (IPTR) HVSpace,
                    Child, (IPTR) HVSpace,
                End;

    _WP_Toolbar_PreviewObj = (Object *)HGroup,
                   MUIA_FrameTitle, __(MSG_PREVIEW),
                   MUIA_Frame, MUIV_Frame_Group,
                   MUIA_Group_SameSize, FALSE,
                End;    

    _WP_Toolbar_InnerGroupObj4 = (Object *)HGroup,
                    MUIA_HorizWeight, 0,
                    MUIA_Group_SameSize, TRUE,
                End;

    _WP_Toolbar_PreviewDirUpObj = ImageButton("", "THEME:Images/Gadgets/Revert");
    _WP_Toolbar_PreviewSearchObj = ImageButton("", "THEME:Images/Gadgets/Search");
/*END _WP_Toolbar_GroupObj---------------------------------------------------*/

/*Add main objects to main window (self?)------------------------------------*/

/*Add navigation Objects to Navigation page*/

    DoMethod(_WP_Navigation_InnerHGrp1, OM_ADDMEMBER,_WP_Navigation_TypeObj);
    DoMethod(_WP_Navigation_InnerHGrp1, OM_ADDMEMBER,HVSpace);
    DoMethod(_WP_Navigation_InnerHGrp1, OM_ADDMEMBER,HVSpace);
#if defined(DEBUG_NETWORKBROWSER)
    _WP_NetworkBrowser_EnabledObj = MUI_MakeObject(MUIO_Checkmark, NULL);
    DoMethod(_WP_Navigation_InnerHGrp2, OM_ADDMEMBER,Label1(_(MSG_NETWORKONWB)));
    DoMethod(_WP_Navigation_InnerHGrp2, OM_ADDMEMBER,_WP_NetworkBrowser_EnabledObj);
#else
    DoMethod(_WP_Navigation_InnerHGrp2, OM_ADDMEMBER,HVSpace);
    DoMethod(_WP_Navigation_InnerHGrp2, OM_ADDMEMBER,HVSpace);
#endif
#if defined(DEBUG_SHOWUSERFILES)
    DoMethod(_WP_Navigation_InnerHGrp2, OM_ADDMEMBER,Label1(_(MSG_USERFILEONWB)));
    DoMethod(_WP_Navigation_InnerHGrp2, OM_ADDMEMBER,_WP_UserFiles_ShowFileFolderObj);
#else
    DoMethod(_WP_Navigation_InnerHGrp2, OM_ADDMEMBER,HVSpace);
    DoMethod(_WP_Navigation_InnerHGrp2, OM_ADDMEMBER,HVSpace);
#endif
    DoMethod(_WP_Navigation_InnerHGrp2, OM_ADDMEMBER,HVSpace);
    DoMethod(_WP_Navigation_InnerHGrp2, OM_ADDMEMBER,HVSpace);

#if defined(DEBUG_CHANGESCREENTITLE)
    DoMethod(_WP_Navigator_ScreenTitleObj,MUIM_Set, MUIA_ShortHelp,_(MSG_BUBBLESCREENTITLE));
    DoMethod(_WP_Navigation_InnerHGrp3, OM_ADDMEMBER,_WP_Navigator_ScreenTitleObj);
    DoMethod(_WP_Navigation_HGrp2, OM_ADDMEMBER,_WP_Navigation_InnerHGrp3);
#endif

    DoMethod(_WP_Navigation_HGrp1, OM_ADDMEMBER,_WP_Navigation_InnerHGrp1);
    DoMethod(_WP_Navigation_HGrp1, OM_ADDMEMBER,_WP_Navigation_InnerHGrp2);

    DoMethod(_WP_NavigationObj, OM_ADDMEMBER,_WP_Navigation_HGrp1);
#if defined(DEBUG_CHANGESCREENTITLE)
    DoMethod(_WP_NavigationObj, OM_ADDMEMBER,_WP_Navigation_HGrp2);
#endif
    /**/
    
    /*Add appearance Objects to Appearance page*/
    /*Add objects which are contain into right part*/
    /*Add objects which are contain into left part*/
    DoMethod(_WP_ViewSettings_VirtGrpObj, OM_ADDMEMBER,_WP_ViewSettings_GroupObj);
    /**/

    DoMethod(_WP_Appearance_GroupObj, OM_ADDMEMBER,_WP_ViewSettings_ScrollGrpObj);
    /**/

    /*Add toolbar Objects to Toolbar page*/
    DoMethod(_WP_Toolbar_InnerGroupObj4, OM_ADDMEMBER,_WP_Toolbar_PreviewDirUpObj);
    DoMethod(_WP_Toolbar_InnerGroupObj4, OM_ADDMEMBER,_WP_Toolbar_PreviewSearchObj);

    DoMethod(_WP_Toolbar_PreviewObj, OM_ADDMEMBER,HVSpace);
    DoMethod(_WP_Toolbar_PreviewObj, OM_ADDMEMBER, _WP_Toolbar_InnerGroupObj4);
    
    DoMethod(_WP_Toolbar_InnerGroupObj2 , OM_ADDMEMBER,_WP_Toolbar_EnabledObj);
    DoMethod(_WP_Toolbar_InnerGroupObj2 , OM_ADDMEMBER,HVSpace);
    DoMethod(_WP_Toolbar_InnerGroupObj2 , OM_ADDMEMBER,HVSpace);

    DoMethod(_WP_Toolbar_InnerGroupObj1 , OM_ADDMEMBER,_WP_Toolbar_InnerGroupObj2);
    DoMethod(_WP_Toolbar_InnerGroupObj1 , OM_ADDMEMBER,_WP_Toolbar_InnerGroupObj3);    


    DoMethod(_WP_Toolbar_GroupObj, OM_ADDMEMBER,_WP_Toolbar_InnerGroupObj1);
    DoMethod(_WP_Toolbar_GroupObj, OM_ADDMEMBER,_WP_Toolbar_PreviewObj);
    /**/

    DoMethod(_WP_Prefs_PageGroupObj, OM_ADDMEMBER,_WP_NavigationObj);/*add Navigation page to pagesGroup*/
    DoMethod(_WP_Prefs_PageGroupObj, OM_ADDMEMBER,_WP_Appearance_GroupObj);/*add Appearance page to pagesGroup*/
    DoMethod(_WP_Prefs_PageGroupObj, OM_ADDMEMBER,_WP_Toolbar_GroupObj);/*add Toolbar page to pagesGroup*/

    DoMethod(self, OM_ADDMEMBER,_WP_Prefs_PageGroupObj);/*add pagesGroup to self*/
/*END Add main objects to main window (self?)--------------------------------*/

    //SET( _WP_Prefs_PageGroupObj, MUIA_Group_ActivePage, 1);  //Goto the Appearance page by default..

/*END main window------------------------------------------------------------*/

/*AdvancedViewWindow---------------------------------------------------------*/
    
    /*Window-------------------------------------------------------------*/
    _WP_AdvancedViewWindow = (Object *)WindowObject,
                    MUIA_Window_CloseGadget, FALSE,
                    MUIA_Window_Title, (IPTR)_(MSG_ADVANCEDOPTIONS),
                    WindowContents, (IPTR) (_WP_AdvancedViewWindowVGrp = (Object *)VGroup, End),
                End;

    /*_WP_AdvancedViewPageGroupObj = Object for handling multi (3) page groups---------*/
    _wpeditor_intern_AdvancedPageNames[WPD_ADVANCED_ICONS] = (STRPTR)_(MSG_ADVANCEDICONS);
    _wpeditor_intern_AdvancedPageNames[WPD_ADVANCED_LABELS] = (STRPTR)_(MSG_ADVANCEDLABELS);
    _wpeditor_intern_AdvancedPageNames[WPD_ADVANCED_BACKGROUND] = (STRPTR)_(MSG_ADVANCEDBACKGROUND);

    _WP_AdvancedViewPageGroupObj = (Object *)RegisterObject,
                    MUIA_Register_Titles, (IPTR) _wpeditor_intern_AdvancedPageNames,
                End;
    /*END _WP_AdvancedViewPageGroupObj-------------------------------------------------*/

    /*Draw Mode Group----------------------------------------------------*/        
    _WP_AdvancedViewBackgroundGrpObj = (Object *)VGroup,
                    MUIA_FrameTitle, (IPTR)_(MSG_BACKGROUNDOPTIONS),
                    MUIA_Frame, MUIV_Frame_Group,
                End;

    _WP_AdvancedViewRenderModeGrpObj = (Object *)HGroup,
                    Child, (IPTR) Label1(_(MSG_DRAWMODE)), End;

    /*AdvancedViewRenderMode cycle button------------------------*/
    _WP_AdvancedViewRenderModeObj = HVSpace;

    /*END AdvancedViewRenderMode cycle button------------------------*/
    /*END Draw Mode Group------------------------------------------------*/

    /*Scale Mode/Tile Mode Group-----------------------------------------*/
    _WP_AdvancedView_RenderModePageObj = (Object *)GroupObject,
                    MUIA_Group_PageMode, TRUE,
                End;

    _WP_AdvancedView_ScaleModeGrpObj = (Object *)GroupObject,
                    MUIA_Group_SameSize, FALSE,
                    MUIA_FrameTitle, (IPTR)_(MSG_SCALEMODEOPTIONS),
                    MUIA_Frame, MUIV_Frame_Group,
                    Child, (IPTR)HVSpace,
                End;
        
    /*AdvancedView_TileModeNames cicle button--------------------*/
    _WP_AdvancedView_TileModeObj = HVSpace;
    _WP_AdvancedView_TileModeObjContainer = (Object *)HGroup, Child, (IPTR)_WP_AdvancedView_TileModeObj, End;
    /*END AdvancedView_TileModeNames cicle button----------------*/

    _WP_AdvancedView_TileModeGrpObj = (Object *)GroupObject,    
                    MUIA_Group_SameSize, FALSE,
                    MUIA_FrameTitle, (IPTR)_(MSG_TILEMODEOPTIONS),
                    MUIA_Frame, MUIV_Frame_Group,
                    MUIA_Group_Columns, 2,
                End;

    _WP_AdvancedView_X_OffsetObj = (Object *)StringObject,
                    StringFrame,
                    MUIA_String_MaxLen, 3,
                    MUIA_String_Accept, (IPTR)"0123456789",
                End;

    _WP_AdvancedView_Y_OffsetObj = (Object *)StringObject,
                    StringFrame,
                    MUIA_String_MaxLen, 3,
                    MUIA_String_Accept, (IPTR)"0123456789",
                End;

    /*END Scale Mode/Tile Mode Group-------------------------------------*/
    /*Icon List Mode Cycle button------------------------*/
    _wpeditor_intern_IconListModeNames[WPD_ICONLISTMODE_GRID] = (STRPTR)_(MSG_ICONLISTMODE_GRID);
    _wpeditor_intern_IconListModeIDs[WPD_ICONLISTMODE_GRID] = WPD_ICONLISTMODE_GRID;
    _wpeditor_intern_IconListModeNames[WPD_ICONLISTMODE_PLAIN] = (STRPTR)_(MSG_ICONLISTMODE_PLAIN);
    _wpeditor_intern_IconListModeIDs[WPD_ICONLISTMODE_PLAIN] = WPD_ICONLISTMODE_PLAIN;
    _WP_AdvancedView_Icon_ListModeObj = MUI_MakeObject(MUIO_Cycle, NULL, _wpeditor_intern_IconListModeNames);

    _wpeditor_intern_IconTextRenderModeNames[WPD_ICONTEXTMODE_OUTLINE] = (STRPTR)_(MSG_ICONTEXTMODE_OUTLINE);
    _wpeditor_intern_IconTextRenderModeIDs[WPD_ICONTEXTMODE_OUTLINE] = WPD_ICONTEXTMODE_OUTLINE;
    _wpeditor_intern_IconTextRenderModeNames[WPD_ICONTEXTMODE_PLAIN] = (STRPTR)_(MSG_ICONTEXTMODE_PLAIN);
    _wpeditor_intern_IconTextRenderModeIDs[WPD_ICONTEXTMODE_PLAIN] = WPD_ICONTEXTMODE_PLAIN;
    _wpeditor_intern_IconTextRenderModeNames[ICON_TEXTMODE_DROPSHADOW] = (STRPTR)_(MSG_ICONTEXTMODE_SHADOW);
    _wpeditor_intern_IconTextRenderModeIDs[ICON_TEXTMODE_DROPSHADOW] = ICON_TEXTMODE_DROPSHADOW;
    _WP_AdvancedView_Icon_TextModeObj = MUI_MakeObject(MUIO_Cycle, NULL, _wpeditor_intern_IconTextRenderModeNames);

    _wpeditor_intern_IconSortModeNames[WPD_ICONSORTMODE_BYNAME] = (STRPTR)_(MSG_ICONSORTMODE_BYNAME);
    _wpeditor_intern_IconSortModeNames[WPD_ICONSORTMODE_BYDATE] = (STRPTR)_(MSG_ICONSORTMODE_BYDATE);
    _wpeditor_intern_IconSortModeNames[WPD_ICONSORTMODE_BYSIZE] = (STRPTR)_(MSG_ICONSORTMODE_BYSIZE);
    _WP_AdvancedView_Icon_SortModeObj = MUI_MakeObject(MUIO_Cycle, NULL, _wpeditor_intern_IconSortModeNames);

    _WP_AdvancedView_Icon_SortModeLabelObj = Label1(_(MSG_DEFAULT_ICONSORTMODE));

    _wpeditor_intern_IconViewModeNames[WPD_ICONVIEWMODE_ALL] = (STRPTR)_(MSG_ICONVIEWMODE_ALL);
    _wpeditor_intern_IconViewModeNames[WPD_ICONVIEWMODE_ICONS] = (STRPTR)_(MSG_ICONVIEWMODE_ICONS);
    _WP_AdvancedView_Icon_ViewModeObj = MUI_MakeObject(MUIO_Cycle, NULL, _wpeditor_intern_IconViewModeNames);

    _WP_AdvancedView_Icon_ViewModeLabelObj = Label1(_(MSG_DEFAULT_ICONVIEWMODE));
    /*END Icon List Mode Cycle button--------------------*/

    _WP_AdvancedView_Icon_AutoSortObj = MUI_MakeObject(MUIO_Checkmark, NULL);

    _WP_AdvancedView_Icon_DragTransparentObj = MUI_MakeObject(MUIO_Checkmark, NULL);

    _WP_AdvancedView_IconLabel_MaxLineLenObj = (Object *)StringObject,
                    StringFrame,
                    MUIA_String_MaxLen, 3,
                    MUIA_String_Format, MUIV_String_Format_Right,
                    MUIA_String_Accept, (IPTR)"0123456789",
                End;

    _WP_AdvancedView_Icon_HorSpacingObj = (Object *)StringObject,
                    StringFrame,
                    MUIA_String_MaxLen, 3,
                    MUIA_String_Format, MUIV_String_Format_Right,
                    MUIA_String_Accept, (IPTR)"0123456789",
                End;

    _WP_AdvancedView_Icon_VertSpacingObj = (Object *)StringObject,
                    StringFrame,
                    MUIA_String_MaxLen, 3,
                    MUIA_String_Format, MUIV_String_Format_Right,
                    MUIA_String_Accept, (IPTR)"0123456789",
                End;

    _WP_AdvancedView_Icon_ImageSpacingObj = (Object *)StringObject,
                    StringFrame,
                    MUIA_String_MaxLen, 3,
                    MUIA_String_Format, MUIV_String_Format_Right,
                    MUIA_String_Accept, (IPTR)"0123456789",
                End;

    _WP_AdvancedView_IconLabel_LabelFontObj = (Object *)StringObject,
                    StringFrame,
                    MUIA_String_MaxLen, 3,
                    MUIA_String_Format, MUIV_String_Format_Right,
                End;

    _WP_AdvancedView_IconLabel_InfoFontObj = (Object *)StringObject,
                    StringFrame,
                    MUIA_String_MaxLen, 3,
                    MUIA_String_Format, MUIV_String_Format_Right,
                End;

#if defined(DEBUG_MULTLINE)
    _WP_AdvancedView_IconLabel_MultiLineNoObj = (Object *)StringObject,
                            StringFrame,
                            MUIA_String_MaxLen, 2,
                            MUIA_String_Format, MUIV_String_Format_Right,
                            MUIA_String_Accept, (IPTR)"0123456789",
                        End;

    _WP_AdvancedView_IconLabel_TextMultiLineObj = MUI_MakeObject(MUIO_Checkmark, NULL);
    _WP_AdvancedView_IconLabel_MultiLineonFocusObj = MUI_MakeObject(MUIO_Checkmark, NULL);
#endif
    _WP_AdvancedView_IconLabel_HorPaddObj = (Object *)StringObject,
                    StringFrame,
                    MUIA_String_MaxLen, 3,
                    MUIA_String_Format, MUIV_String_Format_Right,
                    MUIA_String_Accept, (IPTR)"0123456789",
                End;

    _WP_AdvancedView_IconLabel_VertPaddObj = (Object *)StringObject,
                    StringFrame,
                    MUIA_String_MaxLen, 3,
                    MUIA_String_Format, MUIV_String_Format_Right,
                    MUIA_String_Accept, (IPTR)"0123456789",
                End;

    _WP_AdvancedView_IconLabel_BorderWidthObj = (Object *)StringObject,
                    StringFrame,
                    MUIA_String_MaxLen, 3,
                    MUIA_String_Format, MUIV_String_Format_Right,
                    MUIA_String_Accept, (IPTR)"0123456789",
                End;

    _WP_AdvancedView_IconLabel_BorderHeightObj = (Object *)StringObject,
                    StringFrame,
                    MUIA_String_MaxLen, 3,
                    MUIA_String_Format, MUIV_String_Format_Right,
                    MUIA_String_Accept, (IPTR)"0123456789",
                End;

    _WP_AdvancedView_IconRenderGrpObj = (Object *)GroupObject,
                    MUIA_Group_SameSize, FALSE,
                    MUIA_FrameTitle, __(MSG_ICONSPREFS),
                    MUIA_Frame, MUIV_Frame_Group,
                    MUIA_Group_Columns, 2,
                End;

    _WP_AdvancedView_LabelRenderGrpObj = (Object *)GroupObject,
                    MUIA_Group_SameSize, FALSE,
                    MUIA_FrameTitle, __(MSG_LABELSPREFS),
                    MUIA_Frame, MUIV_Frame_Group,
                    MUIA_Group_Columns, 2,
                End;

    /*Button Group-------------------------------------------------------*/
    _WP_AdvancedView_ButtonGrpObj = (Object *)HGroup,
                    Child, (IPTR)(_WP_AdvancedView_ButtonObj_Use = ImageButton(_(MSG_USE), "THEME:Images/Gadgets/Use")),
                    Child, (IPTR)(_WP_AdvancedView_ButtonObj_Cancel = ImageButton(_(MSG_CANCEL), "THEME:Images/Gadgets/Cancel")),
                End;
    /*END Button Group--------------------------------------------------*/

    /*END Window--------------------------------------------------------*/


/*Add advanced view objects to AdvancedViewWindow object--------------------*/
    DoMethod(_WP_AdvancedViewRenderModeGrpObj, OM_ADDMEMBER,_WP_AdvancedViewRenderModeObj);

    DoMethod(_WP_AdvancedView_TileModeGrpObj, OM_ADDMEMBER, Label1(_(MSG_TILEMODE)));
    DoMethod(_WP_AdvancedView_TileModeGrpObj, OM_ADDMEMBER, _WP_AdvancedView_TileModeObjContainer);
    DoMethod(_WP_AdvancedView_TileModeGrpObj, OM_ADDMEMBER, Label1(_(MSG_XOFFSET)));
    DoMethod(_WP_AdvancedView_TileModeGrpObj, OM_ADDMEMBER, _WP_AdvancedView_X_OffsetObj);
    DoMethod(_WP_AdvancedView_TileModeGrpObj, OM_ADDMEMBER, Label1(_(MSG_YOFFSET)));
    DoMethod(_WP_AdvancedView_TileModeGrpObj, OM_ADDMEMBER, _WP_AdvancedView_Y_OffsetObj);

    DoMethod(_WP_AdvancedView_RenderModePageObj, OM_ADDMEMBER, _WP_AdvancedView_ScaleModeGrpObj);
    DoMethod(_WP_AdvancedView_RenderModePageObj, OM_ADDMEMBER, _WP_AdvancedView_TileModeGrpObj);

    DoMethod(_WP_AdvancedView_IconRenderGrpObj, OM_ADDMEMBER, Label1(_(MSG_ICONLISTMODE)));
    DoMethod(_WP_AdvancedView_IconRenderGrpObj, OM_ADDMEMBER, _WP_AdvancedView_Icon_ListModeObj);
    DoMethod(_WP_AdvancedView_IconRenderGrpObj, OM_ADDMEMBER, Label1(_(MSG_HORIZSPACINGICON)));
    DoMethod(_WP_AdvancedView_IconRenderGrpObj, OM_ADDMEMBER, _WP_AdvancedView_Icon_HorSpacingObj);
    DoMethod(_WP_AdvancedView_IconRenderGrpObj, OM_ADDMEMBER, Label1(_(MSG_VERTSPACINGICON)));
    DoMethod(_WP_AdvancedView_IconRenderGrpObj, OM_ADDMEMBER, _WP_AdvancedView_Icon_VertSpacingObj);
    DoMethod(_WP_AdvancedView_IconRenderGrpObj, OM_ADDMEMBER, Label1(_(MSG_PADDINGICONIMAGELABEL)));
    DoMethod(_WP_AdvancedView_IconRenderGrpObj, OM_ADDMEMBER, _WP_AdvancedView_Icon_ImageSpacingObj);
    DoMethod(_WP_AdvancedView_IconRenderGrpObj, OM_ADDMEMBER, _WP_AdvancedView_Icon_SortModeLabelObj);
    DoMethod(_WP_AdvancedView_IconRenderGrpObj, OM_ADDMEMBER, _WP_AdvancedView_Icon_SortModeObj);
    DoMethod(_WP_AdvancedView_IconRenderGrpObj, OM_ADDMEMBER, Label1(_(MSG_ICON_AUTOSORT_ENABLED)));
    DoMethod(_WP_AdvancedView_IconRenderGrpObj, OM_ADDMEMBER, _WP_AdvancedView_Icon_AutoSortObj );
    DoMethod(_WP_AdvancedView_IconRenderGrpObj, OM_ADDMEMBER, Label1(_(MSG_ICON_DRAG_TRANSPARENT)));
    DoMethod(_WP_AdvancedView_IconRenderGrpObj, OM_ADDMEMBER, _WP_AdvancedView_Icon_DragTransparentObj);
    DoMethod(_WP_AdvancedView_IconRenderGrpObj, OM_ADDMEMBER, _WP_AdvancedView_Icon_ViewModeLabelObj);
    DoMethod(_WP_AdvancedView_IconRenderGrpObj, OM_ADDMEMBER, _WP_AdvancedView_Icon_ViewModeObj);
    DoMethod(_WP_AdvancedView_IconRenderGrpObj, OM_ADDMEMBER,HVSpace);
    DoMethod(_WP_AdvancedView_IconRenderGrpObj, OM_ADDMEMBER,HVSpace);

    DoMethod(_WP_AdvancedView_LabelRenderGrpObj, OM_ADDMEMBER, Label1(_(MSG_ICONTEXTMODE)));
    DoMethod(_WP_AdvancedView_LabelRenderGrpObj, OM_ADDMEMBER, _WP_AdvancedView_Icon_TextModeObj);

/* "TODO: Replace with propper font selectors" */
    DoMethod(_WP_AdvancedView_LabelRenderGrpObj, OM_ADDMEMBER, Label1(_(MSG_ICONLABELFONT)));
    DoMethod(_WP_AdvancedView_LabelRenderGrpObj, OM_ADDMEMBER, _WP_AdvancedView_IconLabel_LabelFontObj);
    DoMethod(_WP_AdvancedView_LabelRenderGrpObj, OM_ADDMEMBER, Label1(_(MSG_ICONINFOFONT)));
    DoMethod(_WP_AdvancedView_LabelRenderGrpObj, OM_ADDMEMBER, _WP_AdvancedView_IconLabel_InfoFontObj);

#if !defined(DEBUG_PERVIEWFONTS)
    SET(_WP_AdvancedView_IconLabel_LabelFontObj, MUIA_Disabled, TRUE);
    SET(_WP_AdvancedView_IconLabel_InfoFontObj, MUIA_Disabled, TRUE);
#endif

    DoMethod(_WP_AdvancedView_LabelRenderGrpObj, OM_ADDMEMBER, Label1(_(MSG_LABELLINELENGTH)));
    DoMethod(_WP_AdvancedView_LabelRenderGrpObj, OM_ADDMEMBER, _WP_AdvancedView_IconLabel_MaxLineLenObj);
#if defined(DEBUG_MULTLINE)
    DoMethod(_WP_AdvancedView_LabelRenderGrpObj, OM_ADDMEMBER, Label1(_(MSG_USEMULTILINELABELS)));
    DoMethod(_WP_AdvancedView_LabelRenderGrpObj, OM_ADDMEMBER, _WP_AdvancedView_IconLabel_TextMultiLineObj);
    DoMethod(_WP_AdvancedView_LabelRenderGrpObj, OM_ADDMEMBER, Label1(_(MSG_SHOWFOCUSEDICON)));
    DoMethod(_WP_AdvancedView_LabelRenderGrpObj, OM_ADDMEMBER, _WP_AdvancedView_IconLabel_MultiLineonFocusObj);
    DoMethod(_WP_AdvancedView_LabelRenderGrpObj, OM_ADDMEMBER, Label1(_(MSG_NDISPLAYLINES)));
    DoMethod(_WP_AdvancedView_LabelRenderGrpObj, OM_ADDMEMBER, _WP_AdvancedView_IconLabel_MultiLineNoObj);
#endif

    DoMethod(_WP_AdvancedView_LabelRenderGrpObj, OM_ADDMEMBER, Label1(_(MSG_HORIZPADDINGLABELTEXT)));
    DoMethod(_WP_AdvancedView_LabelRenderGrpObj, OM_ADDMEMBER, _WP_AdvancedView_IconLabel_HorPaddObj);

    DoMethod(_WP_AdvancedView_LabelRenderGrpObj, OM_ADDMEMBER, Label1(_(MSG_VERTPADDINGLABELTEXT)));
    DoMethod(_WP_AdvancedView_LabelRenderGrpObj, OM_ADDMEMBER, _WP_AdvancedView_IconLabel_VertPaddObj);

    DoMethod(_WP_AdvancedView_LabelRenderGrpObj, OM_ADDMEMBER, Label1(_(MSG_FRAMEWIDTH)));
    DoMethod(_WP_AdvancedView_LabelRenderGrpObj, OM_ADDMEMBER, _WP_AdvancedView_IconLabel_BorderWidthObj);

    DoMethod(_WP_AdvancedView_LabelRenderGrpObj, OM_ADDMEMBER, Label1(_(MSG_FRAMEHEIGHT)));
    DoMethod(_WP_AdvancedView_LabelRenderGrpObj, OM_ADDMEMBER, _WP_AdvancedView_IconLabel_BorderHeightObj);

    DoMethod(_WP_AdvancedView_LabelRenderGrpObj, OM_ADDMEMBER,HVSpace);
    DoMethod(_WP_AdvancedView_LabelRenderGrpObj, OM_ADDMEMBER,HVSpace);


    DoMethod(_WP_AdvancedViewBackgroundGrpObj, OM_ADDMEMBER,_WP_AdvancedViewRenderModeGrpObj);
    DoMethod(_WP_AdvancedViewBackgroundGrpObj, OM_ADDMEMBER,_WP_AdvancedView_RenderModePageObj);
    DoMethod(_WP_AdvancedViewBackgroundGrpObj, OM_ADDMEMBER,HVSpace);

    DoMethod(_WP_AdvancedViewPageGroupObj, OM_ADDMEMBER,_WP_AdvancedView_IconRenderGrpObj);
    DoMethod(_WP_AdvancedViewPageGroupObj, OM_ADDMEMBER,_WP_AdvancedView_LabelRenderGrpObj);
    DoMethod(_WP_AdvancedViewPageGroupObj, OM_ADDMEMBER,_WP_AdvancedViewBackgroundGrpObj);
    DoMethod(_WP_AdvancedViewWindowVGrp, OM_ADDMEMBER,_WP_AdvancedViewPageGroupObj);/*add pagesGroup to view*/
    DoMethod(_WP_AdvancedViewWindowVGrp, OM_ADDMEMBER,_WP_AdvancedView_ButtonGrpObj);

/*END Add advanced view objects to AdvancedViewWindow object-----------------*/
/*END AdvancedViewWindow-----------------------------------------------------*/

/*-------------------*/
    if ((self != NULL) && (_WP_AdvancedViewWindow != NULL ))
    {
        _wpeditor_intern_CLASS = CLASS;
        data = INST_DATA(CLASS, self);

D(bug("[WPEditor] WPEditor__OM_NEW: Prefs Object (self) @ 0x%p\n", self));

        if ((advancedView_data = AllocMem(sizeof(struct WPEditor_AdvancedBackgroundWindow_DATA), MEMF_CLEAR|MEMF_ANY)) != NULL)
        {
            advancedView_data->wpedabwd_Hook_DrawModeChage.h_Entry            = (HOOKFUNC) WandererPrefs_Hook_DrawModeChangeFunc;

            advancedView_data->wpedabwd_Window_WindowObj                      = _WP_AdvancedViewWindow;
            advancedView_data->wpedabwd_Window_BackgroundGrpObj               = _WP_AdvancedViewBackgroundGrpObj;
            advancedView_data->wpedabwd_Window_RenderModeGrpObj               = _WP_AdvancedViewRenderModeGrpObj ;
            advancedView_data->wpedabwd_Window_RenderModeObj                  = _WP_AdvancedViewRenderModeObj;
            advancedView_data->wpedabwd_Window_RenderModePageObj              = _WP_AdvancedView_RenderModePageObj;
            advancedView_data->wpedabwd_Window_TileModeObjContainer           = _WP_AdvancedView_TileModeObjContainer;
            advancedView_data->wpedabwd_Window_TileModeObj                    = _WP_AdvancedView_TileModeObj;
            advancedView_data->wpedabwd_Window_XOffsetObj                     = _WP_AdvancedView_X_OffsetObj;
            advancedView_data->wpedabwd_Window_YOffsetObj                     = _WP_AdvancedView_Y_OffsetObj;

            advancedView_data->wpedabwd_Window_Icon_ListMode                  = _WP_AdvancedView_Icon_ListModeObj;
            advancedView_data->wpedabwd_Window_Icon_TextMode                  = _WP_AdvancedView_Icon_TextModeObj;
            advancedView_data->wpedabwd_Window_Icon_SortMode                  = _WP_AdvancedView_Icon_SortModeObj;
            advancedView_data->wpedabwd_Window_Icon_SortModeLabel             = _WP_AdvancedView_Icon_SortModeLabelObj;
            advancedView_data->wpedabwd_Window_Icon_AutoSort                  = _WP_AdvancedView_Icon_AutoSortObj;
            advancedView_data->wpedabwd_Window_Icon_DragTransparent           = _WP_AdvancedView_Icon_DragTransparentObj;
            advancedView_data->wpedabwd_Window_Icon_ViewMode                  = _WP_AdvancedView_Icon_ViewModeObj;
            advancedView_data->wpedabwd_Window_Icon_ViewModeLabel             = _WP_AdvancedView_Icon_ViewModeLabelObj;

            advancedView_data->wpedabwd_Window_Icon_HorSpacing                = _WP_AdvancedView_Icon_HorSpacingObj;
            advancedView_data->wpedabwd_Window_Icon_VertSpacing               = _WP_AdvancedView_Icon_VertSpacingObj;
            advancedView_data->wpedabwd_Window_Icon_ImageSpacing              = _WP_AdvancedView_Icon_ImageSpacingObj;

            advancedView_data->wpedabwd_Window_IconLabel_LabelFont            = _WP_AdvancedView_IconLabel_LabelFontObj;
            advancedView_data->wpedabwd_Window_IconLabel_InfoFont             = _WP_AdvancedView_IconLabel_InfoFontObj;

            advancedView_data->wpedabwd_Window_IconLabel_MaxLineLen           = _WP_AdvancedView_IconLabel_MaxLineLenObj;

#if defined(DEBUG_MULTLINE)
            advancedView_data->wpedabwd_Window_IconLabel_TextMultiLine        = _WP_AdvancedView_IconLabel_TextMultiLineObj;
            advancedView_data->wpedabwd_Window_IconLabel_MultiLineonFocus     = _WP_AdvancedView_IconLabel_MultiLineonFocusObj;
            advancedView_data->wpedabwd_Window_IconLabel_MultiLineNo          = _WP_AdvancedView_IconLabel_MultiLineNoObj;        
#endif
            advancedView_data->wpedabwd_Window_IconLabel_HorPadd              = _WP_AdvancedView_IconLabel_HorPaddObj;
            advancedView_data->wpedabwd_Window_IconLabel_VertPadd             = _WP_AdvancedView_IconLabel_VertPaddObj;
            advancedView_data->wpedabwd_Window_IconLabel_BorderWidth          = _WP_AdvancedView_IconLabel_BorderWidthObj;
            advancedView_data->wpedabwd_Window_IconLabel_BorderHeight         = _WP_AdvancedView_IconLabel_BorderHeightObj;

            advancedView_data->wpedabwd_Window_UseObj                           = _WP_AdvancedView_ButtonObj_Use;
            advancedView_data->wpedabwd_Window_CancelObj                      = _WP_AdvancedView_ButtonObj_Cancel;

            NewList(&advancedView_data->wpedabwd_Window_AdvancedViewRenderModes);
            EntryElementRegister(&advancedView_data->wpedabwd_Window_AdvancedViewRenderModes, IconWindowExt_ImageBackFill_RenderMode_Tiled, _(MSG_TILED));
            EntryElementRegister(&advancedView_data->wpedabwd_Window_AdvancedViewRenderModes, IconWindowExt_ImageBackFill_RenderMode_Scale, _(MSG_SCALED));

            NewList(&advancedView_data->wpedabwd_Window_AdvancedViewTileModes);
            EntryElementRegister(&advancedView_data->wpedabwd_Window_AdvancedViewTileModes, IconWindowExt_ImageBackFill_TileMode_Float, _(MSG_FLOATING));
            EntryElementRegister(&advancedView_data->wpedabwd_Window_AdvancedViewTileModes, IconWindowExt_ImageBackFill_TileMode_Fixed, _(MSG_FIXED));

D(bug("[WPEditor] WPEditor__OM_NEW: 'Advanced' Window Object @ 0x%p\n", advancedView_data->wpedabwd_Window_WindowObj));
        }
        data->wped_AdvancedViewSettings_WindowData    = advancedView_data;

        data->wped_ViewSettings_GroupObj              = _WP_ViewSettings_GroupObj;
        data->wped_ViewSettings_SpacerObj             = _WP_ViewSettings_SpacerObj;

        data->wped_c_NavigationMethod                 = _WP_Navigation_TypeObj;
        data->wped_cm_ToolbarEnabled                  = _WP_Toolbar_EnabledObj;
#if defined(DEBUG_CHANGESCREENTITLE)
        data->wped_s_screentitle                      =_WP_Navigator_ScreenTitleObj;
#endif
#if defined(DEBUG_NETWORKBROWSER)
        data->wped_cm_EnableNetworkBrowser            = _WP_NetworkBrowser_EnabledObj;
#endif
#if defined(DEBUG_SHOWUSERFILES)
        data->wped_cm_EnableUserFiles                 = _WP_UserFiles_ShowFileFolderObj;
#endif

        data->wped_toolbarpreview                     = _WP_Toolbar_PreviewObj;

        data->wped_toolbarGroup                       = _WP_Toolbar_GroupObj;
        data->wped_Hook_CloseAdvancedOptions.h_Entry  = ( HOOKFUNC )WandererPrefs_Hook_CloseAdvancedOptionsFunc;

        //-- Setup notifications -------------------------------------------
/* "TODO: The toolbar class will become an external module to wanderer with its own prefs" */
#if defined(DEBUG_TOOLBARINTERNAL)
        /*    Enhanced navigation depends on the toolbar class for
            control - so we disable it if the toolbar isnt available! */
        DoMethod
        (
            data->wped_cm_ToolbarEnabled, MUIM_Notify, MUIA_Selected, FALSE,
            (IPTR)data->wped_c_NavigationMethod, 3, MUIM_Set, MUIA_Cycle_Active, 0
        ); 

        DoMethod
        (
            data->wped_cm_ToolbarEnabled, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR)data->wped_c_NavigationMethod, 3, MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue
        );

        /* Only enable the preview if the toolbar is enabled */
        DoMethod
        (
            data->wped_cm_ToolbarEnabled, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR)data->wped_toolbarpreview, 3, MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue
        );

        DoMethod
        (
            data->wped_cm_ToolbarEnabled, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );

        DoMethod
        (
            data->wped_c_NavigationMethod, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );

#endif
#if defined(DEBUG_NETWORKBROWSER)
        DoMethod
        (
            data->wped_cm_EnableNetworkBrowser, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
#endif
#if defined(DEBUG_SHOWUSERFILES)
        DoMethod
        (
            data->wped_cm_EnableUserFiles, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
#endif

        if (data->wped_AdvancedViewSettings_WindowData)
        {
            DoMethod
            (
                data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_IconLabel_MultiLineNo, MUIM_Notify, MUIA_Disabled, TRUE,
                (IPTR)data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_IconLabel_MultiLineNo, 3, MUIM_Set, MUIA_String_Integer, 1
            );

            DoMethod
            (
                data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_IconLabel_MultiLineNo, MUIM_Notify, MUIA_Disabled, FALSE,
                (IPTR)data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_IconLabel_MultiLineNo, 3, MUIM_Set, MUIA_String_Integer, 2
            );

            DoMethod
            (
                data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_IconLabel_TextMultiLine, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,  
                (IPTR)data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_IconLabel_MultiLineNo, 3, MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue
            );

            DoMethod
            (
                data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_UseObj, MUIM_Notify, MUIA_Pressed, FALSE,
                (IPTR)self, 3, MUIM_CallHook, &data->wped_Hook_CloseAdvancedOptions, TRUE
            );

            DoMethod
            (
                data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_CancelObj, MUIM_Notify, MUIA_Pressed, FALSE,
                (IPTR)self, 3, MUIM_CallHook, &data->wped_Hook_CloseAdvancedOptions, FALSE
            );

            DoMethod
            (
                data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_WindowObj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
                (IPTR)self, 3, MUIM_CallHook, &data->wped_Hook_CloseAdvancedOptions, FALSE
            );

        }

        /*--------------------*/        

        /*Initialization and setup of _wpeditor_intern_ViewSettings----------*/
            /*_wpeditor_intern_ViewSettings is a 
          list of objects attached to
          _WP_ViewSettings_GroupObj;
        */
        NewList(&_wpeditor_intern_ViewSettings);

        WPEditor__NewViewSettingObjects("Workbench", TRUE);//add a setting node
        WPEditor__NewViewSettingObjects("Drawer", TRUE);//add a setting node
#if defined(DEBUG_NEWVIEWSETTINGS)
        WPEditor__NewViewSettingObjects("Screen", TRUE);//add a setting node
        WPEditor__NewViewSettingObjects("Toolbar", FALSE);//add a setting node
#endif
        
        struct WPEditor_ViewSettingsObject *_viewSettings_Node = NULL;
        
        ForeachNode(&_wpeditor_intern_ViewSettings, _viewSettings_Node)
        {
            Object     *thisViewImspecGrp = NULL;
            Object     *thisViewAdvancedGrp = NULL;

D(bug("[WPEditor] WPEditor__OM_NEW: Adding ViewSetting Objects for '%s' to Prefs GUI ..\n", _viewSettings_Node->wpedbo_ViewName));

            thisViewImspecGrp = (Object *)GroupObject,
                            MUIA_Group_SameSize, FALSE,
                            MUIA_Frame, MUIV_Frame_None,
                            MUIA_Group_Columns, 2,
                                Child, (IPTR) HVSpace,
                            Child, (IPTR) HVSpace,    
                            Child, (IPTR) Label1(_viewSettings_Node->wpedbo_ViewName),
                            Child, (IPTR) _viewSettings_Node->wpedbo_ImageSpecObject,
                            Child, (IPTR) HVSpace,
                            Child, (IPTR) HVSpace,
                        End;

            if ((thisViewImspecGrp) && (data->wped_FirstBGImSpecObj == NULL)) 
                data->wped_FirstBGImSpecObj = thisViewImspecGrp;

            if (_viewSettings_Node->wpedbo_AdvancedOptionsObject)
            {
                thisViewAdvancedGrp = (Object *)GroupObject,
                                MUIA_Group_SameSize, FALSE,
                                MUIA_Frame, MUIV_Frame_None,
                                MUIA_Group_Columns, 2,
                                Child, (IPTR) HVSpace,
                                Child, (IPTR) _viewSettings_Node->wpedbo_AdvancedOptionsObject,
                            End;

                if ((thisViewAdvancedGrp) && (data->wped_FirstBGAdvancedObj == NULL)) 
                    data->wped_FirstBGAdvancedObj = thisViewAdvancedGrp;
            }

            if ((thisViewImspecGrp) &&
                ((!(_viewSettings_Node->wpedbo_AdvancedOptionsObject)) ||
                 ((_viewSettings_Node->wpedbo_AdvancedOptionsObject) && (thisViewAdvancedGrp))))
            {
D(bug("[WPEditor] WPEditor__OM_NEW: GUI Objects Created ..\n"));

                if (DoMethod(_WP_ViewSettings_GroupObj, MUIM_Group_InitChange))
                {
                    DoMethod(_WP_ViewSettings_GroupObj, OM_ADDMEMBER, thisViewImspecGrp);
        
                    if (_viewSettings_Node->wpedbo_AdvancedOptionsObject)
                        DoMethod(_WP_ViewSettings_GroupObj, OM_ADDMEMBER, thisViewAdvancedGrp);
        
                    DoMethod(_WP_ViewSettings_GroupObj, MUIM_Group_ExitChange);
                }

D(bug("[WPEditor] WPEditor__OM_NEW: GUI Objects inserted in Prefs GUI ..\n"));
                    /* Set our ViewSetting(s) notifications */
                DoMethod
                    (
                        _viewSettings_Node->wpedbo_ImageSpecObject,
                        MUIM_Notify, MUIA_Imagedisplay_Spec, MUIV_EveryTime,
                        (IPTR)self, 3, MUIM_CallHook, 
                        &_viewSettings_Node->wpedbo_Hook_CheckImage, _viewSettings_Node
                    );

                if (_viewSettings_Node->wpedbo_AdvancedOptionsObject)
                {
                    _viewSettings_Node->wpedbo_Hook_OpenAdvancedOptions.h_Entry = ( HOOKFUNC )WandererPrefs_Hook_OpenAdvancedOptionsFunc;
                    
                    DoMethod
                        (
                            _viewSettings_Node->wpedbo_AdvancedOptionsObject, MUIM_Notify, MUIA_Pressed, FALSE,
                            (IPTR)self, 3, MUIM_CallHook, &_viewSettings_Node->wpedbo_Hook_OpenAdvancedOptions, _viewSettings_Node
                        );
                }
D(bug("[WPEditor] WPEditor__OM_NEW: GUI Objects Notifications set ..\n"));
            }
            else
            {
D(bug("[WPEditor] WPEditor__OM_NEW: Failed to create objects ..\n"));
                if (thisViewAdvancedGrp) DoMethod(thisViewAdvancedGrp, OM_DISPOSE);
                        
                if (thisViewImspecGrp) DoMethod(thisViewImspecGrp, OM_DISPOSE);
            }
        }
    }
    else
    {
D(bug("[WPEditor] WPEditor__OM_NEW: Failed to create GUI ..\n"));
        if (_WP_AdvancedViewWindow) DoMethod(_WP_AdvancedViewWindow, OM_DISPOSE);
        if (self) DoMethod(self, OM_DISPOSE);

        self = NULL;
    }
/*--------------*/

    return self;
}


/*BOOL WPEditor_ProccessGlobalChunk(): read a TagItem global_chunk (from global setting) 
 *and memorize its value into correspondent attribute of an object of the gui...;
 */
BOOL WPEditor_ProccessGlobalChunk(Class *CLASS, Object *self, struct TagItem32 *global_chunk, IPTR chunk_size)
{
    SETUP_WPEDITOR_INST_DATA;

    int i = 0, tag_count = (chunk_size / sizeof(struct TagItem32));
    BOOL cont = TRUE;
    //BOOL state = FALSE;

D(bug("[WPEditor] WPEditor_ProccessGlobalChunk(%d tags)\n", tag_count));

    for (i = 0; i < tag_count; i++)
    {
        if (cont)
        {
                        /* prefs file is stored in little endian */
            switch ((int)AROS_LE2LONG(global_chunk[i].ti_Tag))
            {
                case MUIA_IconWindow_WindowNavigationMethod:
                {
D(bug("[WPEditor] WPEditor_ProccessGlobalChunk: Tag %d = MUIA_IconWindow_WindowNavigationMethod, val = %d\n", i, AROS_LE2LONG(global_chunk[i].ti_Data)));
                    SET(data->wped_c_NavigationMethod, MUIA_Cycle_Active, (IPTR)AROS_LE2LONG(global_chunk[i].ti_Data));
                    break;
                }
#if defined(DEBUG_SHOWUSERFILES)
                case MUIA_IconWindowExt_UserFiles_ShowFilesFolder:
                {
D(bug("[WPEditor] WPEditor_ProccessGlobalChunk: Tag %d = MUIA_IconWindowExt_UserFiles_ShowFilesFolder, val = %d\n", i, AROS_LE2LONG(global_chunk[i].ti_Data)));
                    SET(data->wped_cm_EnableUserFiles, MUIA_Selected, (BOOL)AROS_LE2LONG(global_chunk[i].ti_Data));
                    break;
                }
#endif
                case TAG_DONE:
                {
D(bug("[WPEditor] WPEditor_ProccessGlobalChunk: Tag %d = TAG_DONE!!\n", i));
                    cont = FALSE;
                    break;
                }
            }
        }
    }

    return TRUE;
}

#if defined(DEBUG_NETWORKBROWSER)
BOOL WPEditor_ProccessNetworkChunk(Class *CLASS, Object *self, UBYTE *_viewSettings_Chunk)
{
    SETUP_WPEDITOR_INST_DATA;

    struct TagItem32 *network_tags = _viewSettings_Chunk;
    SET(data->wped_cm_EnableNetworkBrowser, MUIA_Selected, AROS_LE2LONG(network_tags[0].ti_Data));

    return TRUE;
}
#endif

#if defined(DEBUG_CHANGESCREENTITLE)
BOOL WPEditor_ProccessScreenTitleChunk(Class *CLASS, Object *self, UBYTE *_ScreenTitle_Chunk)
{
    SETUP_WPEDITOR_INST_DATA;

    
D(bug("[WPEditor] WPEditor_ProccessScreenTitleChunk: string readed = %s\n", _ScreenTitle_Chunk));
    SET(data->wped_s_screentitle, MUIA_String_Contents, _ScreenTitle_Chunk);
D(bug("[WPEditor] WPEditor_ProccessScreenTitleChunk: string setted = %s\n", _ScreenTitle_Chunk));

    return TRUE;
}
#endif

/*Renabled WPEditor_ProccessViewSettingsChunk() as Nic Andrews (nicja@yahoo.com) has asked...;
 *Please report here what do it do this function ;)
 */

BOOL WPEditor_ProccessViewSettingsChunk(Class *CLASS, Object *self, char *_viewSettings_Name, UBYTE *_viewSettings_Chunk, IPTR chunk_size)
{
    SETUP_WPEDITOR_INST_DATA;

D(bug("[WPEditor] WPEditor_ProccessViewSettingsChunk('%s')\n", _viewSettings_Name));
    //BOOL  _viewSettings_NodeFound = FALSE;//unused
    struct WPEditor_ViewSettingsObject  *_viewSettings_Node = NULL;

    _viewSettings_Node = WPEditor__FindViewSettingObjects(_viewSettings_Name);

    if (_viewSettings_Node)
    {
D(bug("[WPEditor] WPEditor_ProccessViewSettingsChunk: Updating Existing node @ 0x%p\n", _viewSettings_Node));
    }
    else
    {
D(bug("[WPEditor] WPEditor_ProccessViewSettingsChunk: Creating new Objects for '%s'\n", _viewSettings_Name));
        if (((strcmp(_viewSettings_Name, "Workbench")) == 0) ||
            ((strcmp(_viewSettings_Name, "Drawer")) == 0) ||
            ((strcmp(_viewSettings_Name, "Screen")) == 0))
            _viewSettings_Node = WPEditor__NewViewSettingObjects(_viewSettings_Name, TRUE);
        else
            _viewSettings_Node = WPEditor__NewViewSettingObjects(_viewSettings_Name, FALSE);

        Object         *thisViewImspecGrp = NULL;
        Object         *thisViewAdvancedGrp = NULL;

D(bug("[WPEditor] WPEditor_ProccessViewSettingsChunk: Adding ViewSetting Objects for '%s' to Prefs GUI ..\n", _viewSettings_Node->wpedbo_ViewName));

        thisViewImspecGrp = (Object *)GroupObject,
                        MUIA_Group_SameSize, FALSE,
                        MUIA_Frame, MUIV_Frame_None,
                        MUIA_Group_Columns, 2,

                        Child, (IPTR) HVSpace,
                        Child, (IPTR) HVSpace,
                        
                        Child, (IPTR) Label1(_viewSettings_Node->wpedbo_ViewName),
                        Child, (IPTR) _viewSettings_Node->wpedbo_ImageSpecObject,
                        Child, (IPTR) HVSpace,
                        Child, (IPTR) HVSpace,
                    End;
                    
        if (_viewSettings_Node->wpedbo_AdvancedOptionsObject)
            thisViewAdvancedGrp = (Object *)GroupObject,
                        MUIA_Group_SameSize, FALSE,
                        MUIA_Frame, MUIV_Frame_None,
                        MUIA_Group_Columns, 2,

                        Child, (IPTR) HVSpace,
                        Child, (IPTR) _viewSettings_Node->wpedbo_AdvancedOptionsObject,
                    End;
        
        if ((thisViewImspecGrp) &&
            ((!(_viewSettings_Node->wpedbo_AdvancedOptionsObject)) ||
             ((_viewSettings_Node->wpedbo_AdvancedOptionsObject) && (thisViewAdvancedGrp))))
        {
D(bug("[WPEditor] WPEditor_ProccessViewSettingsChunk: GUI Objects Created ..\n"));

            if (DoMethod(data->wped_ViewSettings_GroupObj, MUIM_Group_InitChange))
            {
                DoMethod(data->wped_ViewSettings_GroupObj, OM_ADDMEMBER, thisViewImspecGrp);

                if (_viewSettings_Node->wpedbo_AdvancedOptionsObject)
                    DoMethod(data->wped_ViewSettings_GroupObj, OM_ADDMEMBER, thisViewAdvancedGrp);

                DoMethod(data->wped_ViewSettings_GroupObj, MUIM_Group_ExitChange);
            }

D(bug("[WPEditor] WPEditor_ProccessViewSettingsChunk: GUI Objects inserted in Prefs GUI ..\n"));

            DoMethod (
                _viewSettings_Node->wpedbo_ImageSpecObject,
                MUIM_Notify, MUIA_Imagedisplay_Spec, MUIV_EveryTime,
                (IPTR)self, 3, MUIM_CallHook, 
                &_viewSettings_Node->wpedbo_Hook_CheckImage, _viewSettings_Node
            );

            if (_viewSettings_Node->wpedbo_AdvancedOptionsObject)
            {
                _viewSettings_Node->wpedbo_Hook_OpenAdvancedOptions.h_Entry = ( HOOKFUNC )WandererPrefs_Hook_OpenAdvancedOptionsFunc;

                DoMethod
                    (
                        _viewSettings_Node->wpedbo_AdvancedOptionsObject, MUIM_Notify, MUIA_Pressed, FALSE,
                        (IPTR)self, 3, MUIM_CallHook, &_viewSettings_Node->wpedbo_Hook_OpenAdvancedOptions, _viewSettings_Node
                    );
            }

D(bug("[WPEditor] WPEditor_ProccessViewSettingsChunk: GUI Objects Notifications set ..\n"));
        }
        else
        {
D(bug("[WPEditor] WPEditor_ProccessViewSettingsChunk: Failed to create objects ..\n"));
            if (thisViewAdvancedGrp) DoMethod(thisViewAdvancedGrp, OM_DISPOSE);
                
            if (thisViewImspecGrp) DoMethod(thisViewImspecGrp, OM_DISPOSE);
        }
    }

    if ((_viewSettings_Node->wpedbo_AdvancedOptionsObject) && (chunk_size > (strlen(_viewSettings_Chunk) + 1)))
    {
D(bug("[WPEditor] WPEditor_ProccessViewSettingsChunk: Chunk has options Tag data ..\n"));
        UBYTE _viewSettings_TagOffset = ((strlen(_viewSettings_Chunk)  + 1)/4);

        if ((_viewSettings_TagOffset * 4) != (strlen(_viewSettings_Chunk)  + 1))
        {
            _viewSettings_TagOffset = (_viewSettings_TagOffset + 1) * 4;
D(bug("[WPEditor] WPEditor_ProccessViewSettingsChunk: String length unalined - rounding up (length %d, rounded %d) \n", strlen(_viewSettings_Chunk) + 1, _viewSettings_TagOffset ));
        }
        else
        {
            _viewSettings_TagOffset = _viewSettings_TagOffset * 4;
D(bug("[WPEditor] WPEditor_ProccessViewSettingsChunk: String length doesnt need aligned (length %d) \n", strlen(_viewSettings_Chunk) + 1));
        }
        
        if (_viewSettings_Node->wpedbo_Options)
        {
D(bug("[WPEditor] WPEditor_ProccessViewSettingsChunk: Freeing old ViewSetting Tag data @ 0x%p\n", _viewSettings_Node->wpedbo_Options));
            FreeVec(_viewSettings_Node->wpedbo_Options);
        }

        int tag_count = (chunk_size - _viewSettings_TagOffset)/sizeof(struct TagItem32);

        _viewSettings_Node->wpedbo_Options = AllocVec((tag_count + 1) * sizeof(struct TagItem32), 
                                  MEMF_ANY|MEMF_CLEAR
                                 );
        if (_viewSettings_Node->wpedbo_Options)
        {
D(bug("[WPEditor] WPEditor_ProccessViewSettingsChunk: Allocated new Tag storage @ 0x%p [%d bytes] \n", _viewSettings_Node->wpedbo_Options, chunk_size - _viewSettings_TagOffset));
            CopyMem(_viewSettings_Chunk + _viewSettings_TagOffset, _viewSettings_Node->wpedbo_Options, tag_count * sizeof(struct TagItem32));
            _viewSettings_Node->wpedbo_Options[tag_count].ti_Tag = TAG_DONE;
        }
    }

    SET(_viewSettings_Node->wpedbo_ImageSpecObject, MUIA_Imagedisplay_Spec, _viewSettings_Chunk);

    return TRUE;
}


/*IPTR WPEditor__MUIM_PrefsEditor_ImportFH(): definition of an abstract function from
 *MUIC_PrefsEditor; This function basically read the iff prefs file and show in
 *Wanderer prefs window the data readed...;
*/
IPTR WPEditor__MUIM_PrefsEditor_ImportFH
(
    Class *CLASS, Object *self, 
    struct MUIP_PrefsEditor_ImportFH *message
)
{
    //SETUP_WPEDITOR_INST_DATA;//unused
    
    D(struct ContextNode     *context);
    struct IFFHandle       *handle;
    BOOL                   success = TRUE;
    LONG                   error;
    IPTR                   iff_parse_mode = IFFPARSE_SCAN;
    UBYTE                  chunk_buffer[WP_IFF_CHUNK_BUFFER_SIZE];
    STRPTR                  buffer = AllocVec(TOOLBAR_PREFSSIZE, MEMF_ANY | MEMF_CLEAR);

D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH()\n"));

    if (!(handle = AllocIFF()))
        return FALSE;

D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH: Iff current handle 0x%p, msg handle 0x%p\n", handle->iff_Stream, message->fh));

    handle->iff_Stream = (IPTR) message->fh;
    InitIFFasDOS(handle);

    if ((error = OpenIFF(handle, IFFF_READ)) == 0)
    {
    if ((error = StopChunk(handle, ID_PREF, ID_WANDR)) == 0)
    {        
        do
        {                
            if ((error = ParseIFF(handle, iff_parse_mode)) == 0)
            {
                D(context = CurrentChunk(handle));
                iff_parse_mode = IFFPARSE_STEP;

D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH: Context 0x%p\n", context));
                    
                if ((error = ReadChunkBytes(handle, chunk_buffer, WP_IFF_CHUNK_BUFFER_SIZE)))
                {
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH: ReadChunkBytes() Chunk matches Prefs Header size ..\n"));
                    struct WandererPrefsIFFChunkHeader *this_header = (struct WandererPrefsIFFChunkHeader *) chunk_buffer;
                    char                               *this_chunk_name = NULL;
                                        /* prefs file is stored in little endian */
                    IPTR                               this_chunk_size = AROS_LE2LONG(this_header->wpIFFch_ChunkSize);
                        
                    
                    if ((this_chunk_name = AllocVec(strlen(this_header->wpIFFch_ChunkType) +1, MEMF_ANY|MEMF_CLEAR)))
                    {
                        strcpy(this_chunk_name, this_header->wpIFFch_ChunkType);
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH: Prefs Header for '%s' data size %d bytes\n", this_chunk_name, this_chunk_size));

                        if ((error = ParseIFF(handle, IFFPARSE_STEP)) == IFFERR_EOC)
                        {
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH: End of header chunk ..\n"));

                            if ((error = ParseIFF(handle, IFFPARSE_STEP)) == 0)
                            {
                                D(context = CurrentChunk(handle));

D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH: Context 0x%p\n", context));

                                error = ReadChunkBytes(
                                                    handle, 
                                            chunk_buffer, 
                                            this_chunk_size
                                               );
                                    
                                if (error == this_chunk_size)
                                {
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH: ReadChunkBytes() Chunk matches Prefs Data size .. (%d)\n", error));
                                    if ((strcmp(this_chunk_name, "wanderer:global")) == 0)
                                    {
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH: Process data for wanderer global config chunk ..\n"));
                                        WPEditor_ProccessGlobalChunk(CLASS, self,(struct TagItem32 *) chunk_buffer, this_chunk_size);
                                    }
#if defined(DEBUG_NETWORKBROWSER)
                                    else if ((strcmp(this_chunk_name, "wanderer:network")) == 0)
                                    {
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH: Process data for wanderer network config chunk ..\n"));
                                        WPEditor_ProccessNetworkChunk(CLASS, self, chunk_buffer);
                                    }
#endif

#if defined(DEBUG_CHANGESCREENTITLE)
                                    else if ((strcmp(this_chunk_name, "wanderer:screentitle")) == 0)
                                    {
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH: Process data for wanderer screentitle config chunk ..\n"));
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH: Chunk screentitle Data size .. (%d)\n", error));
                                        WPEditor_ProccessScreenTitleChunk(CLASS, self, chunk_buffer);
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH: Data for wanderer screentitle config chunk PROCESSED..\n"));
                                    }
#endif
                                    else if ((strncmp(this_chunk_name, "wanderer:viewsettings", strlen("wanderer:viewsettings"))) == 0)
                                    {
                                        char *view_name = this_chunk_name + strlen("wanderer:viewsettings") + 1;
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH: Process data for wanderer background config chunk '%s'..\n", view_name));
                                        WPEditor_ProccessViewSettingsChunk(CLASS, self, view_name, chunk_buffer, this_chunk_size);
                                        
                                    }
                                }//END if (error == this_chunk_size)    

                                if ((error = ParseIFF(handle, IFFPARSE_STEP)) == IFFERR_EOC)
                                {
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH: End of Data chunk ..\n"));
                                }
                            }//END if ((error = ParseIFF(handle, IFFPARSE_STEP)) == 0)
                        }//END if ((error = ParseIFF(handle, IFFPARSE_STEP)) == IFFERR_EOC)                
                    }//END if (this_chunk_name)
                }//END if ((error = ReadChunkBytes(handle, chunk_buffer, WP_IFF_CHUNK_BUFFER_SIZE)))
            }//END if ((error = ParseIFF(handle, iff_parse_mode)) == 0)
            else
            {
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH: ParseIFF() failed, returncode %ld!\n", error));
                //success = FALSE;// this brokes cancel button
                //break;
            }

        } while (error != IFFERR_EOF);
            
            
    }
    else
    {
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH: StopChunk() failed, returncode %ld!\n", error));
        success = FALSE;// this brokes cancel button
    }//END if ((error = StopChunk(handle, ID_PREF, ID_WANDR)) == 0)

        CloseIFF(handle);
    }
    else
    {
D(bug("[WPEditor] Failed to open stream!, returncode %ld!\n", error));
        //ShowError(_(MSG_CANT_OPEN_STREAM));
    success = FALSE;
    }//END if ((error = OpenIFF(handle, IFFF_READ)) == 0)

    //Close((APTR)handle->iff_Stream);
    FreeIFF(handle);

    /* Importing toolbar preferences */
    if (GetVar(toolbar_PrefsFile, buffer, TOOLBAR_PREFSSIZE, GVF_GLOBAL_ONLY) != -1)
    {
        SETUP_WPEDITOR_INST_DATA;

        if ((strcasecmp(buffer, "True")) == 0)
        {
            SET(data->wped_cm_ToolbarEnabled, MUIA_Selected, TRUE);
        }
        else
        {
            SET(data->wped_cm_ToolbarEnabled, MUIA_Selected, FALSE);
        }
    }
    FreeVec(buffer);

    return success;
}

#define SAVEVIEWSETTINGSTAG(tag, defvalue)                                                                                                  \
    _viewSettings_TagList[_viewSettings_TagCount].ti_Tag = AROS_LONG2LE(tag);                                                               \
    _viewSettings_TagList[_viewSettings_TagCount].ti_Data = AROS_LONG2LE(GetTag32Data(tag, defvalue, _viewSettings_Node->wpedbo_Options));  \
    _viewSettings_TagCount += 1;


/*IPTR WPEditor__MUIM_PrefsEditor_ExportFH(): definition of an abstract function from
 *MUIC_PrefsEditor; This function basically memorized in the correspondent iff prefs file 
 *of Wanderer prefs the changes made with Wanderer prefs window...;
 */
IPTR WPEditor__MUIM_PrefsEditor_ExportFH
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ExportFH *message
)
{
    SETUP_WPEDITOR_INST_DATA;
    
    struct IFFHandle                       *handle;
    struct PrefHeader                      header = { 0 };
    struct WandererPrefsIFFChunkHeader     wanderer_chunkdata = { };
    BOOL                                   success = TRUE;
    LONG                                   error   = 0;

D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH()\n"));

    if ((handle = AllocIFF()))
    {
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: Current IFF handle 0x%p, msg handle 0x%p\n", handle->iff_Stream, message->fh));

        handle->iff_Stream = (IPTR) message->fh;
        
        InitIFFasDOS(handle);
        
        if (!(error = OpenIFF(handle, IFFF_WRITE))) /* NULL = successful! */
        {
            //BYTE i = 0;
            
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: Write IFF FORM Header Chunk ... \n")); /* FIXME: IFFSIZE_UNKNOWN? */
            if ((error = PushChunk(handle, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN)) != 0)
            {
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: IFF FORM Header Chunk : Error! %d \n", error));
                goto exportFH_CloseIFF;
            }
            
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: Write Preference File Header Chunk ... \n")); /* FIXME: IFFSIZE_UNKNOWN? */
            if ((error = PushChunk(handle, ID_PREF, ID_PRHD, IFFSIZE_UNKNOWN)) == 0)
            {
                header.ph_Version = PHV_CURRENT;
                header.ph_Type    = 0;
                
                WriteChunkBytes(handle, &header, sizeof(struct PrefHeader));
                
                if ((error = PopChunk(handle)) != 0)
                {
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: Preference File Header PopChunk() = %ld\n", error));
                    goto exportFH_CloseFORM;
                }     
            }
            else
            {
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: Preference File Header Chunk : Error! %d \n", error));
                goto exportFH_CloseFORM;
            }

D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: Prepare 'global' Wanderer Prefs Data Chunk Data ... \n"));
            /* save toolbar state*/
            struct TagItem32    *_wp_GlobalTags = AllocVec((256 * sizeof(struct TagItem32)), MEMF_ANY|MEMF_CLEAR);
            ULONG              _wp_GlobalTagCounter = 0;
                        /* helper to convert to little endian */
            STACKED IPTR           ti_Data = 0;

                        // save navigation bahaviour
                        _wp_GlobalTags[_wp_GlobalTagCounter].ti_Tag = AROS_LONG2LE(MUIA_IconWindow_WindowNavigationMethod);
                        GET(data->wped_c_NavigationMethod, MUIA_Cycle_Active, &ti_Data);
                        _wp_GlobalTags[_wp_GlobalTagCounter].ti_Data = AROS_LONG2LE(ti_Data);
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'global' MUIA_IconWindow_WindowNavigationMethod @ Tag %d, data = %d\n", _wp_GlobalTagCounter, ti_Data));
                        _wp_GlobalTagCounter += 1;

#if defined(DEBUG_SHOWUSERFILES)
            _wp_GlobalTags[_wp_GlobalTagCounter].ti_Tag = AROS_LONG2LE(MUIA_IconWindowExt_UserFiles_ShowFilesFolder);
            GET(data->wped_cm_EnableUserFiles, MUIA_Selected, &ti_Data);
            _wp_GlobalTags[_wp_GlobalTagCounter].ti_Data = AROS_LONG2LE(ti_Data);
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'global' MUIA_IconWindowExt_UserFiles_ShowFilesFolder @ Tag %d, data = %d\n", _wp_GlobalTagCounter, ti_Data));
            _wp_GlobalTagCounter += 1;
#endif

            _wp_GlobalTags[_wp_GlobalTagCounter].ti_Tag = AROS_LONG2LE(TAG_DONE);
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'global' Marked Tag %d as TAG_DONE\n", _wp_GlobalTagCounter));

            ULONG globaldatasize = (_wp_GlobalTagCounter + 1) * sizeof(struct TagItem32);

D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: Write 'global' Wanderer Prefs Header Chunk ... \n"));
            if ((error = PushChunk(handle, ID_PREF, ID_WANDR, sizeof(struct WandererPrefsIFFChunkHeader))) == 0)
            {
                sprintf(wanderer_chunkdata.wpIFFch_ChunkType, "%s" , "wanderer:global");
                wanderer_chunkdata.wpIFFch_ChunkSize = AROS_LONG2LE(globaldatasize);
                
                WriteChunkBytes(handle, &wanderer_chunkdata, sizeof(struct WandererPrefsIFFChunkHeader));
                
                if ((error = PopChunk(handle)) != 0)
                {
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'global' Header PopChunk() = %ld\n", error));
                    goto exportFH_CloseFORM;
                }
            }
            else
            {
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'global' Wanderer Prefs Header Chunk : Error! %d \n", error));
                goto exportFH_CloseFORM;
            }    

D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: Write 'global' Wanderer Prefs Data Chunk ... \n"));
            if ((error = PushChunk(handle, ID_PREF, ID_WANDR, globaldatasize)) == 0) 
            {
                error = WriteChunkBytes(handle, _wp_GlobalTags, globaldatasize);
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'global' Data Chunk | Wrote %d bytes (data size = %d bytes)\n", error, globaldatasize));
                if ((error = PopChunk(handle)) != 0)
                {
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'global' PopChunk() = %ld\n", error));
                    goto exportFH_CloseFORM;
                }
                FreeVec(_wp_GlobalTags);
            }
            else
            {
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'global' PushChunk() = %ld failed\n", error));
                goto exportFH_CloseFORM;
            }

#if defined(DEBUG_NETWORKBROWSER)
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: Write 'network' Wanderer Prefs Header Chunk ... \n"));
            if ((error = PushChunk(handle, ID_PREF, ID_WANDR, sizeof(struct WandererPrefsIFFChunkHeader))) == 0)
            {
                sprintf(wanderer_chunkdata.wpIFFch_ChunkType, "%s" , "wanderer:network");
                wanderer_chunkdata.wpIFFch_ChunkSize = AROS_LONG2LE(sizeof(struct TagItem));
                
                WriteChunkBytes(handle, &wanderer_chunkdata, sizeof(struct WandererPrefsIFFChunkHeader));
                
                if ((error = PopChunk(handle)) != 0)
                {
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'network' Header PopChunk() = %ld\n", error));
                    goto exportFH_CloseFORM;
                }
            }
            else
            {
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'network' Wanderer Prefs Header Chunk : Error! %d \n", error));
                goto exportFH_CloseFORM;
            }    

D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: Write 'network' Wanderer Prefs Data Chunk ... \n"));
            if ((error = PushChunk(handle, ID_PREF, ID_WANDR, sizeof(struct TagItem))) == 0) 
            {
                struct TagItem32 __wp_networkconfig[2];

                /* save network options*/
                __wp_networkconfig[0].ti_Tag = MUIA_IconWindowExt_NetworkBrowser_Show;
                GET(data->wped_cm_EnableNetworkBrowser, MUIA_Selected, &ti_Data);
                                __wp_networkconfig[0].ti_Data = AROS_LONG2LE(ti_Data);

                error = WriteChunkBytes(handle, __wp_networkconfig, sizeof(struct TagItem32));
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'network' Data Chunk | Wrote %d bytes (data size = %d bytes)\n", error, sizeof(struct TagItem)));
                if ((error = PopChunk(handle)) != 0)
                {
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'network' PopChunk() = %ld\n", error));
                    goto exportFH_CloseFORM;
                }
            }
            else
            {
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'network' PushChunk() = %ld failed\n", error));
                goto exportFH_CloseFORM;
            }
#endif

#if defined(DEBUG_CHANGESCREENTITLE)
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: Write 'screentitle' Wanderer Prefs Header Chunk ... \n"));
            if ((error = PushChunk(handle, ID_PREF, ID_WANDR, sizeof(struct WandererPrefsIFFChunkHeader))) == 0)
            {
                sprintf(wanderer_chunkdata.wpIFFch_ChunkType, "%s" , "wanderer:screentitle");
                UBYTE *screentitlestr = NULL;

                GET(data->wped_s_screentitle, MUIA_String_Contents, &screentitlestr);

                wanderer_chunkdata.wpIFFch_ChunkSize = AROS_LONG2LE(strlen(screentitlestr) + 1);

                WriteChunkBytes(handle, &wanderer_chunkdata, sizeof(struct WandererPrefsIFFChunkHeader));
                
                if ((error = PopChunk(handle)) != 0)
                {
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'screentitle' Header PopChunk() = %ld\n", error));
                    goto exportFH_CloseFORM;
                }
            }
            else
            {
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'screentitle' Wanderer Prefs Header Chunk : Error! %d \n", error));
                goto exportFH_CloseFORM;
            }    

D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: Write 'screentitle' Wanderer Prefs Data Chunk ... \n"));
            if ((error = PushChunk(handle, ID_PREF, ID_WANDR, IFFSIZE_UNKNOWN)) == 0) 
            {
                // save screentitle options
                UBYTE *screentitlestr = NULL;

                GET(data->wped_s_screentitle, MUIA_String_Contents, &screentitlestr);
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'screentitle' string to write %s\n", screentitlestr));            
                error = WriteChunkBytes(handle, screentitlestr, strlen(screentitlestr) + 1);
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'screentitle' string written %s\n", screentitlestr));
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'screentitle' Data Chunk | Wrote %d bytes\n", error));
                if ((error = PopChunk(handle)) != 0)
                {
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'screentitle' PopChunk() = %ld\n", error));
                    goto exportFH_CloseFORM;
                }
            }
            else
            {
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'screentitle' PushChunk() = %ld failed\n", error));
                goto exportFH_CloseFORM;
            }
#endif

            struct WPEditor_ViewSettingsObject *_viewSettings_Node = NULL;
            ForeachNode(&_wpeditor_intern_ViewSettings, _viewSettings_Node)
            {
                IPTR                   _viewSettings_ChunkSize = 0, _viewSettings_TagCount = 0;
                struct TagItem32    _viewSettings_TagList[WP_MAX_BG_TAG_COUNT];

                sprintf(wanderer_chunkdata.wpIFFch_ChunkType, "%s.%s" , "wanderer:viewsettings", _viewSettings_Node->wpedbo_ViewName);

D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: Write 'ViewSettings' Wanderer Prefs Header Chunk  for '%s' ... \n", _viewSettings_Node->wpedbo_ViewName));

                char *background_value = NULL;
                GET(_viewSettings_Node->wpedbo_ImageSpecObject, MUIA_Imagedisplay_Spec, &background_value);
                
                if (background_value)
                {
                    PushChunk(handle, ID_PREF, ID_WANDR, sizeof(struct WandererPrefsIFFChunkHeader));

                    UBYTE _viewSettings_TagOffset = ((strlen(background_value)  + 1)/4);

                    if ((_viewSettings_TagOffset * 4) != (strlen(background_value)  + 1))
                    {
                        _viewSettings_TagOffset = (_viewSettings_TagOffset + 1) * 4;
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: Write 'ViewSettings' String length unalined - rounding up (length %d, rounded %d) \n", strlen(background_value) + 1, _viewSettings_TagOffset ));
                    }
                    else
                    {
                        _viewSettings_TagOffset = _viewSettings_TagOffset * 4;
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: Write 'ViewSettings' String length doesnt need aligned (length %d) \n", strlen(background_value) + 1));
                    }

                    _viewSettings_ChunkSize += _viewSettings_TagOffset;

                    if (((_viewSettings_Node->wpedbo_AdvancedOptionsObject)) && (_viewSettings_Node->wpedbo_Options))
                    {
                        //UBYTE *background_typepointer = background_value;
                        UBYTE background_type = background_value[0] - 48;

                        const struct TagItem32     *tstate = _viewSettings_Node->wpedbo_Options;
                        struct TagItem32        *tag = NULL;

                        switch (background_type)
                        {
                            case 5:
                            {
                                //Picture type -> store appropriate tags ..
                            }
                            case 0:
                            {
                                //Pattern type -> store appropriate tags ..
                                _viewSettings_TagList[_viewSettings_TagCount].ti_Tag   = MUIA_IconWindowExt_ImageBackFill_BGRenderMode;
                                _viewSettings_TagList[_viewSettings_TagCount].ti_Data = GetTag32Data(MUIA_IconWindowExt_ImageBackFill_BGRenderMode, IconWindowExt_ImageBackFill_RenderMode_Tiled, tstate);
                                _viewSettings_TagCount += 1;

                                while ((tag = NextTag32Item(&tstate)) != NULL)
                                {
                                    switch (tag->ti_Tag)
                                    {
                                        case MUIA_IconWindowExt_ImageBackFill_BGTileMode:
                                        case MUIA_IconWindowExt_ImageBackFill_BGXOffset:
                                        case MUIA_IconWindowExt_ImageBackFill_BGYOffset:
                                            _viewSettings_TagList[_viewSettings_TagCount].ti_Tag   = tag->ti_Tag;
                                            _viewSettings_TagList[_viewSettings_TagCount].ti_Data = tag->ti_Data;
                                            _viewSettings_TagCount += 1;
                                            break;
                                        default:
                                            break;
                                    }
                                }
                                break;
                            }
                            default:
                                break;
                        }
                        
                        SAVEVIEWSETTINGSTAG(MUIA_IconList_IconListMode, ICON_LISTMODE_GRID);
                        SAVEVIEWSETTINGSTAG(MUIA_IconList_LabelText_Mode, ICON_TEXTMODE_OUTLINE);
                        SAVEVIEWSETTINGSTAG(MUIA_IconList_LabelText_MaxLineLen, ILC_ICONLABEL_MAXLINELEN_DEFAULT);

#if defined(DEBUG_MULTLINE)
                        SAVEVIEWSETTINGSTAG(MUIA_IconList_LabelText_MultiLine, 1);
                        SAVEVIEWSETTINGSTAG(MUIA_IconList_LabelText_MultiLineOnFocus, FALSE);
#endif

                        SAVEVIEWSETTINGSTAG(MUIA_IconList_Icon_HorizontalSpacing, ILC_ICON_HORIZONTALMARGIN_DEFAULT);
                        SAVEVIEWSETTINGSTAG(MUIA_IconList_Icon_VerticalSpacing, ILC_ICON_VERTICALMARGIN_DEFAULT);
                        SAVEVIEWSETTINGSTAG(MUIA_IconList_Icon_ImageSpacing, ILC_ICONLABEL_IMAGEMARGIN_DEFAULT);
                        SAVEVIEWSETTINGSTAG(MUIA_IconList_LabelText_HorizontalPadding, ILC_ICONLABEL_HORIZONTALTEXTMARGIN_DEFAULT);
                        SAVEVIEWSETTINGSTAG(MUIA_IconList_LabelText_VerticalPadding, ILC_ICONLABEL_VERTICALTEXTMARGIN_DEFAULT);
                        SAVEVIEWSETTINGSTAG(MUIA_IconList_LabelText_BorderWidth, ILC_ICONLABEL_BORDERWIDTH_DEFAULT);
                        SAVEVIEWSETTINGSTAG(MUIA_IconList_LabelText_BorderHeight, ILC_ICONLABEL_BORDERHEIGHT_DEFAULT);
                        SAVEVIEWSETTINGSTAG(MUIA_IconList_SortFlags, MUIV_IconList_Sort_ByName);
                        SAVEVIEWSETTINGSTAG(MUIA_IconList_DragImageTransparent, FALSE);
                        SAVEVIEWSETTINGSTAG(MUIA_IconList_DisplayFlags, ICONLIST_DISP_SHOWINFO);
                    }
                    _viewSettings_ChunkSize += (_viewSettings_TagCount * sizeof(struct TagItem32));

                    wanderer_chunkdata.wpIFFch_ChunkSize = AROS_LONG2LE(_viewSettings_ChunkSize);

                    WriteChunkBytes(handle, &wanderer_chunkdata, sizeof(struct WandererPrefsIFFChunkHeader));

                    PopChunk(handle);

D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: Write 'ViewSettings' Wanderer Prefs Data Chunk  for '%s' ... \n", _viewSettings_Node->wpedbo_ViewName));

                    if ((error = PushChunk(handle, ID_PREF, ID_WANDR, _viewSettings_ChunkSize)) == 0)
                    {
                        UBYTE *_viewSettings_ChunkData = AllocMem(_viewSettings_ChunkSize, MEMF_ANY|MEMF_CLEAR);
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'ViewSettings' Chunk Data storage @ 0x%p, %d bytes\n", _viewSettings_ChunkData, _viewSettings_ChunkSize));

                        sprintf(_viewSettings_ChunkData, "%s", background_value);
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'ViewSettings' MUIA_Background = '%s'\n", _viewSettings_ChunkData));
                        if ((_viewSettings_Node->wpedbo_AdvancedOptionsObject) && ((_viewSettings_Node->wpedbo_Options)&&(_viewSettings_TagCount > 0)))
                        {
                            struct TagItem32 *dest_tag = (struct TagItem32 *)(_viewSettings_ChunkData + _viewSettings_TagOffset);
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'ViewSettings' Writing data for %d Tags @ 0x%p\n", _viewSettings_TagCount, dest_tag));
                            do
                            {
                                dest_tag[_viewSettings_TagCount - 1].ti_Tag  = AROS_LONG2LE(_viewSettings_TagList[_viewSettings_TagCount - 1].ti_Tag);
                                dest_tag[_viewSettings_TagCount - 1].ti_Data = AROS_LONG2LE(_viewSettings_TagList[_viewSettings_TagCount - 1].ti_Data);
                                _viewSettings_TagCount -= 1;
                            }while(_viewSettings_TagCount > 0);
                        }

                        error = WriteChunkBytes(handle, _viewSettings_ChunkData, _viewSettings_ChunkSize);
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'ViewSettings' Data Chunk | Wrote %d bytes (data size = %d bytes)\n", error, _viewSettings_ChunkSize));
                        if ((error = PopChunk(handle)) != 0) // TODO: We need some error checking here!
                        {
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'ViewSettings' Data PopChunk() = %ld\n", error));
                        }
                    }
                    else
                    {
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'ViewSettings' Data PushChunk() = %ld failed\n", error));
                    }
                }
                else
                {
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'ViewSettings' Skipping (no value set) ... \n"));
                }
            }

exportFH_CloseFORM:

            /* Terminate the FORM */
            PopChunk(handle);
        }
        else
        {
            //ShowError(_(MSG_CANT_OPEN_STREAM));
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: Can't open stream!\n"));
            success = FALSE;
        }

exportFH_CloseIFF:

D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: Closing Handles ..\n"));

        CloseIFF(handle);
        FreeIFF(handle);
    }
    else // AllocIFF()
    {
        // Do something more here - if IFF allocation has failed, something isn't right
        //ShowError(_(MSG_CANT_ALLOCATE_IFFPTR));
        success = FALSE;
    }

D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: Export Finished\n"));

    return success;
}


IPTR WPEditor__OM_GET
(
    Class *CLASS, Object *self, struct opGet *msg
)
{
    SETUP_WPEDITOR_INST_DATA;

    switch(msg->opg_AttrID)
    {
    case MUIA_WPEditor_AdvancedViewWindow:
        *msg->opg_Storage = (IPTR)data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_WindowObj;
        return TRUE;
    }

    return DoSuperMethodA(CLASS, self, (Msg) msg);
}

IPTR WPEditor__MUIM_Show
(
    Class *CLASS, Object *self, Msg message
)
{
    //SETUP_WPEDITOR_INST_DATA;
    
D(bug("[WPEditor] WPEditor__MUIM_Show()\n"));

    if (!DoSuperMethodA(CLASS, self, message)) return FALSE;

#if defined(DEBUG_FORCEWINSIZE)
    Object *this_Win = _win(self);
    if ((this_Win) && (data->wped_DimensionsSet == NULL))
    {
        LONG    thisWin_Width  = 0,
                thisWin_Height = 0,
                thisWin_X      = 0,
                thisWin_Y      = 0;
        
        LONG    viewsettingsbox_Height = 0;
        
        struct Window *thisWin_Window = NULL;
        
        GET(this_Win, MUIA_Window_Width, &thisWin_Width);
        GET(this_Win, MUIA_Window_Height, &thisWin_Height);
        GET(this_Win, MUIA_Window_TopEdge, &thisWin_X);
        GET(this_Win, MUIA_Window_LeftEdge, &thisWin_Y);

        GET(this_Win, MUIA_Window_Window, &thisWin_Window);

        GET(data->wped_ViewSettings_GroupObj, MUIA_Height, &viewsettingsbox_Height);
        
D(bug("[WPEditor] WPEditor__MUIM_Show: WindowObj @ 0x%p, Real Window @ 0x%p, %d, %d [%d x %d]\n", 
            this_Win,
            thisWin_Window,
            thisWin_X, thisWin_Y,
            thisWin_Width, thisWin_Height));

D(bug("[WPEditor] WPEditor__MUIM_Show: ViewSettings Group height = %d\n", viewsettingsbox_Height));

        if ((thisWin_Window) && (viewsettingsbox_Height > 0))
        {
            LONG     NEWHEIGHT = 0,
                    ImgSpec_Height = 0,
                    Advanced_Height = 0;
            
            if (data->wped_FirstBGImSpecObj) GET(data->wped_FirstBGImSpecObj, MUIA_Height, &ImgSpec_Height);
            if (data->wped_FirstBGAdvancedObj) GET(data->wped_FirstBGAdvancedObj, MUIA_Height, &Advanced_Height);

            if ((ImgSpec_Height > 0) && (Advanced_Height > 0))
            {
                NEWHEIGHT = ImgSpec_Height + Advanced_Height;
                if ((2 * NEWHEIGHT) < viewsettingsbox_Height)
                {
                    thisWin_Height = (thisWin_Height - viewsettingsbox_Height) + (2 * NEWHEIGHT);
                    thisWin_Y = thisWin_Y + ((viewsettingsbox_Height - (2 * NEWHEIGHT))/2);

D(bug("[WPEditor] WPEditor__MUIM_Show: Changing windows dimensions to  %d, %d [%d x %d]\n", 
            thisWin_X, thisWin_Y,
            thisWin_Width, thisWin_Height));

                    ChangeWindowBox(thisWin_Window,    thisWin_X, thisWin_Y, thisWin_Width, thisWin_Height);
                    data->wped_DimensionsSet = TRUE;
                }
            }
        }
    }
#endif
    return TRUE;
}

IPTR WPEditor__MUIM_PrefsEditor_Save
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_WPEDITOR_INST_DATA;

    BOOL toolbar_enabled = (BOOL)XGET(data->wped_cm_ToolbarEnabled, MUIA_Selected);

    /* Export toolbar preferences */
    if (toolbar_enabled)
        SetVar(toolbar_PrefsFile, "True", 4, GVF_GLOBAL_ONLY | GVF_SAVE_VAR);
    else
        SetVar(toolbar_PrefsFile, "False", 5, GVF_GLOBAL_ONLY | GVF_SAVE_VAR);

    /* Call parent */
    return DoSuperMethodA(CLASS, self, message);
}

IPTR WPEditor__MUIM_PrefsEditor_Use
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_WPEDITOR_INST_DATA;

    BOOL toolbar_enabled = (BOOL)XGET(data->wped_cm_ToolbarEnabled, MUIA_Selected);

    /* Export toolbar preferences */
    if (toolbar_enabled)
        SetVar(toolbar_PrefsFile, "True", 4, GVF_GLOBAL_ONLY);
    else
        SetVar(toolbar_PrefsFile, "False", 5, GVF_GLOBAL_ONLY);

    /* Call parent */
    return DoSuperMethodA(CLASS, self, message);
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_7
(
    WPEditor, NULL, MUIC_PrefsEditor, NULL,
    OM_NEW,                                struct opSet *,
    OM_GET,                                struct opGet *,
    MUIM_Show,                             Msg,
    MUIM_PrefsEditor_ImportFH,             struct MUIP_PrefsEditor_ImportFH *,
    MUIM_PrefsEditor_ExportFH,             struct MUIP_PrefsEditor_ExportFH *,
    MUIM_PrefsEditor_Use,                  Msg,
    MUIM_PrefsEditor_Save,                 Msg
);
