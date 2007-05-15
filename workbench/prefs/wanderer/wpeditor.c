/*
    Copyright  2004, The AROS Development Team. All rights reserved.
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

#define       DEBUG_ADVANCEDIMAGEOPTIONS
#define       DEBUG_TOOLBARINTERNAL
#define       DEBUG_SHOWUSERFILES
//#define       DEBUG_FORCEWINSIZE
//#define       DEBUG_NEWVIEWSETTINGS
//#define       DEBUG_NETWORKBROWSER
//#define       DEBUG_MULTLINE
//#define       DEBUG_CHANGEMENUBAR

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

#warning "TODO: Include the wandererprefs definitions in a better way .."
#include "../../system/Wanderer/wandererprefs.h"
#include "../../libs/muimaster/classes/iconlist_attributes.h"
#include "../../system/Wanderer/iconwindow_attributes.h"

/*** Private Methods ********************************************************/

/*** Instance Data **********************************************************/
struct WPEditor_ViewSettingsObject
{
	struct Node	wpedbo_Node;
	char		*wpedbo_ViewName;
	Object		*wpedbo_ImageSpecObject;
	Object		*wpedbo_AdvancedOptionsObject;
	IPTR		*wpedbo_Type;
    	struct TagItem	*wpedbo_Options;
    	struct Hook	wpedbo_Hook_CheckImage;
    	struct Hook	wpedbo_Hook_OpenAdvancedOptions;
	IPTR		wpedbo_state_AdvancedDisabled;
};

struct WPEditor_AdvancedBackgroundWindow_DATA
{
	Object	*wpedabwd_Window_WindowObj,
                	*wpedabwd_Window_UseObj,
			*wpedabwd_Window_CancelObj,
			*wpedabwd_Window_RenderModeGrpObj,
			*wpedabwd_Window_RenderModeObj,
			*wpedabwd_Window_PageObj,
			*wpedabwd_Window_TileModeObj,
			*wpedabwd_Window_XOffsetObj,
			*wpedabwd_Window_YOffsetObj;

	STRPTR  *wpedabwd_RenderModeObj_RenderModes;
	IPTR    *wpedabwd_RenderModeObj_RenderPages;
    	struct Hook wpedabwd_Hook_DrawModeChage;
};

struct WPEditor_DATA
{
	struct WPEditor_AdvancedBackgroundWindow_DATA   *wped_AdvancedViewSettings_WindowData;

	struct WPEditor_ViewSettingsObject              *wped_ViewSettings_Current;

	Object                                          *wped_FirstBGImSpecObj,
								*wped_FirstBGAdvancedObj;	

	ULONG                                           wped_DimensionsSet;

	Object                                          *wped_ViewSettings_GroupObj,
						        *wped_ViewSettings_SpacerObj,
                                                    	*wped_c_NavigationMethod,
                                                    	*wped_cm_ToolbarEnabled, 
							#if defined(DEBUG_CHANGEMENUBAR)
								*wped_s_menubar, 
							#endif
                                                    	*wped_toolbarpreview,
	                                                *wped_background_drawmode,
                                                    	*wped_icon_listmode,
                                                    	*wped_icon_textmode,
                                                    	*wped_icon_textmaxlen,
							#if defined(DEBUG_NETWORKBROWSER)
                                                    		*wped_cm_EnableNetworkBrowser, 
							#endif
							#if defined(DEBUG_SHOWUSERFILES)
                                                    		*wped_cm_EnableUserFiles, 
							#endif
							#if defined(DEBUG_MULTLINE)
                                                   		*wped_icon_textmultiline, 
                                                   		*wped_icon_multilineonfocus, 
                                                   		*wped_icon_multilineno, 	
							#endif
                                                    	*wped_toolbarGroup;

    	struct Hook                           		wped_Hook_CloseAdvancedOptions;
};

//static struct Hook navichangehook;
static STRPTR   navigationtypelabels[3];
static STRPTR   iconlistmodes[3];
static STRPTR   icontextmodes[3];
static STRPTR   registerpages[4];

static STRPTR   _WP_AdvancedViewRenderModeNames[WP_MAX_RENDER_MODES];
static IPTR     _WP_AdvancedViewRenderModeValues[WP_MAX_RENDER_MODES];
static STRPTR   _WP_AdvancedView_TileModeNames[WP_MAX_TILE_MODES];
static IPTR     _WP_AdvancedView_TileModeValues[WP_MAX_TILE_MODES];

static Class         *_wpeditor_intern_CLASS = NULL;
static struct List   _wpeditor_intern_ViewSettings;

/*** Macros *****************************************************************/
#define SETUP_WPEDITOR_INST_DATA struct WPEditor_DATA *data = INST_DATA(CLASS, self)

/*** Hook functions *********************************************************/

IPTR GetRenderModeTag(char *rendermode_name)
{
	int i = 0;
	
	for (i = 0; i < WP_MAX_RENDER_MODES; i++)
	{
		if (strcmp(_WP_AdvancedViewRenderModeNames[i], rendermode_name) == 0)
		{
			return _WP_AdvancedViewRenderModeValues[i];
		}
	}
	return -1;
}

IPTR GetRenderModeName(IPTR rendermode_val)
{
	int i = 0;
	
	for (i = 0; i < WP_MAX_RENDER_MODES; i++)
	{
		if (_WP_AdvancedViewRenderModeValues[i] == rendermode_val)
		{
			return (IPTR) _WP_AdvancedViewRenderModeNames[i];
		}
	}
	return -1;
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

IPTR GetTagCount(struct TagItem * this_Taglist)
{
	int i = 0;
	
	for (i = 0; i < WP_MAX_BG_TAG_COUNT; i++)
	{
		if (this_Taglist[i].ti_Tag == TAG_DONE) return i;
	}
	return WP_MAX_BG_TAG_COUNT;
}

IPTR SetViewSettingTag(struct TagItem * this_Taglist, IPTR Tag_ID, IPTR newTag_Value)
{
	int i = 0;
	do
	{
		if (this_Taglist[i].ti_Tag == TAG_DONE)
			break;
		else if (this_Taglist[i].ti_Tag == Tag_ID)
		{
			if (this_Taglist[i].ti_Data == newTag_Value) return -1;
			this_Taglist[i].ti_Data = newTag_Value;
			return TRUE;
		}
		i ++;
	}while(i < WP_MAX_BG_TAG_COUNT);

	return FALSE;
}

/* **** */

AROS_UFH3(
    void, WandererPrefs_Hook_OpenAdvancedOptionsFunc,
    AROS_UFHA(struct Hook *,    hook,   A0),
    AROS_UFHA(APTR *,           obj,    A2),
    AROS_UFHA(APTR,             param,  A1)
)
{
	AROS_USERFUNC_INIT

	Object *self= (Object *) obj;
	struct WPEditor_ViewSettingsObject *_viewSettings_Current = 
							*(struct WPEditor_ViewSettingsObject **)param;
    	Class *CLASS = _wpeditor_intern_CLASS;

	struct WPEditor_DATA *data = INST_DATA(CLASS, self);

	data->wped_ViewSettings_Current = _viewSettings_Current;

	UBYTE	*ImageSelector_Spec = NULL;
	char 	*Image_Spec = NULL;

	GET(_viewSettings_Current->wpedbo_ImageSpecObject, MUIA_Imagedisplay_Spec, &ImageSelector_Spec);
	
	Image_Spec = AllocVec(strlen(ImageSelector_Spec) + 1, MEMF_ANY|MEMF_CLEAR);
	if (Image_Spec)
	{
		strcpy(Image_Spec, ImageSelector_Spec);
		SET(_viewSettings_Current->wpedbo_ImageSpecObject, MUIA_Imagedisplay_Spec, Image_Spec);

		struct WPEditor_ViewSettingsObject *_viewSettings_Node = NULL;

		ForeachNode(&_wpeditor_intern_ViewSettings, _viewSettings_Node)
		{
			GET(_viewSettings_Node->wpedbo_AdvancedOptionsObject, 
			    MUIA_Disabled, 
			    &_viewSettings_Node->wpedbo_state_AdvancedDisabled);
			
			SET(_viewSettings_Node->wpedbo_ImageSpecObject, MUIA_Disabled, TRUE);//1_Disable
			SET(_viewSettings_Node->wpedbo_AdvancedOptionsObject, MUIA_Disabled, TRUE);//2_Disable
		}
		
		/*Enable this and remove *_Disable instructions over 
		  when discovered the zune refresh problem...*/
		//SET(data->wped_ViewSettings_GroupObj, MUIA_Disabled, TRUE);
		
		SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_WindowObj, MUIA_Window_Open, TRUE);
		FreeVec(Image_Spec);
	}

    AROS_USERFUNC_EXIT
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
		SET(_viewSettings_Node->wpedbo_ImageSpecObject, MUIA_Disabled, FALSE);//1_Enabled
		SET(_viewSettings_Node->wpedbo_AdvancedOptionsObject, MUIA_Disabled, FALSE);//2_Enabled
		SET(_viewSettings_Node->wpedbo_AdvancedOptionsObject, MUIA_Disabled, _viewSettings_Node->wpedbo_state_AdvancedDisabled);
	}

	if ((use_settings) && (data->wped_ViewSettings_Current))
	{
D(bug("[WPEditor] WandererPrefs_Hook_CloseAdvancedOptionsFunc: Updating tags for Background ..\n"));
		IPTR current_rendermode = 0;
		GET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_RenderModeObj, MUIA_Cycle_Active, &current_rendermode);
		char *current_rendermode_name = data->wped_AdvancedViewSettings_WindowData->wpedabwd_RenderModeObj_RenderModes[current_rendermode];

D(bug("[WPEditor] WandererPrefs_Hook_CloseAdvancedOptionsFunc: Render Mode '%s'\n", current_rendermode_name));
		current_rendermode = GetRenderModeTag(current_rendermode_name);
		if (current_rendermode == -1)
		{
D(bug("[WPEditor] WandererPrefs_Hook_CloseAdvancedOptionsFunc: ERROR: Unknown Render mode string!?!?!\n"));
		}
		else
		{
			success = SetViewSettingTag(data->wped_ViewSettings_Current->wpedbo_Options, MUIA_IconWindowExt_ImageBackFill_BGRenderMode, current_rendermode);
			if (success == FALSE)
			{
D(bug("[WPEditor] WandererPrefs_Hook_CloseAdvancedOptionsFunc: No MUIA_IconWindowExt_ImageBackFill_BGRenderMode TAG - Adding ..\n"));
#warning "TODO: Allocate extra storage for our tags.."
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
					success = SetViewSettingTag(data->wped_ViewSettings_Current->wpedbo_Options, MUIA_IconWindowExt_ImageBackFill_BGXOffset, XGET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_XOffsetObj, MUIA_String_Integer));
					if (success == FALSE)
					{
D(bug("[WPEditor] WandererPrefs_Hook_CloseAdvancedOptionsFunc: No MUIA_IconWindowExt_ImageBackFill_BGXOffset TAG - Adding ..\n"));
#warning "TODO: Allocate extra storage for our tags.."
					}
					else if (success == TRUE)
					{
						settings_changed = TRUE;
					}
					
					success = SetViewSettingTag(data->wped_ViewSettings_Current->wpedbo_Options, MUIA_IconWindowExt_ImageBackFill_BGYOffset, XGET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_YOffsetObj, MUIA_String_Integer));
					if (success == FALSE)
					{
D(bug("[WPEditor] WandererPrefs_Hook_CloseAdvancedOptionsFunc: No MUIA_IconWindowExt_ImageBackFill_BGYOffset TAG - Adding ..\n"));
#warning "TODO: Allocate extra storage for our tags.."
					}
					else if (success == TRUE)
					{
						settings_changed = TRUE;
					}

					IPTR current_tilemode = _WP_AdvancedView_TileModeValues[XGET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_TileModeObj, MUIA_Cycle_Active)];
					success = SetViewSettingTag(data->wped_ViewSettings_Current->wpedbo_Options, MUIA_IconWindowExt_ImageBackFill_BGTileMode, current_tilemode);
					if (success == FALSE)
					{
D(bug("[WPEditor] WandererPrefs_Hook_CloseAdvancedOptionsFunc: No MUIA_IconWindowExt_ImageBackFill_BGTileMode TAG - Adding ..\n"));
#warning "TODO: Allocate extra storage for our tags.."
					}
					else if (success == TRUE)
					{
						settings_changed = TRUE;
					}
					break;
			}

			if (settings_changed) SET(self, MUIA_PrefsEditor_Changed, TRUE);
		}
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
		IPTR drawmode_no = 0;

		GET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_RenderModeObj, MUIA_Cycle_Active, &drawmode_no);

D(bug("[WPEditor] WandererPrefs_Hook_DrawModeChangeFunc: Active DrawMode = %d, Page = %d\n", drawmode_no, data->wped_AdvancedViewSettings_WindowData->wpedabwd_RenderModeObj_RenderPages[drawmode_no]));

		SET((Object *) (data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_PageObj),
		    MUIA_Group_ActivePage, 
		    data->wped_AdvancedViewSettings_WindowData->wpedabwd_RenderModeObj_RenderPages[drawmode_no]);
	}
	
    AROS_USERFUNC_EXIT
}

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
	
D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: Object @ %x reports image spec '%s'\n", _viewSettings_Current->wpedbo_ImageSpecObject, (char *)ImageSelector_Spec));

	IPTR  this_Background_type = (IPTR)(ImageSelector_Spec[0] - 48);
	
	_viewSettings_Current->wpedbo_Type = (IPTR) this_Background_type;
	data->wped_ViewSettings_Current = _viewSettings_Current;

	if ((_viewSettings_Current->wpedbo_AdvancedOptionsObject) && 
		((_viewSettings_Current->wpedbo_Type == 5)||(_viewSettings_Current->wpedbo_Type == 0)))
	{
D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: Image-type spec (%d) - Enabling Advanced options ..\n", _viewSettings_Current->wpedbo_Type));
#if defined(DEBUG_ADVANCEDIMAGEOPTIONS)
		SET(_viewSettings_Current->wpedbo_AdvancedOptionsObject, MUIA_Disabled, FALSE);
#endif
		STRPTR           newBG_RenderModes[WP_DRAWMODE_COUNT];
		IPTR             newBG_DrawModePages[WP_DRAWMODE_COUNT];
		IPTR             newBG_DrawModeCount = 0;

		struct TagItem   newBG_Options[WP_MAX_BG_TAG_COUNT];
		IPTR             newBG_OptionCount = 0;

		int              i;
		for (i = 0; i < WP_MAX_BG_TAG_COUNT;i++)
		{
			newBG_Options[i].ti_Tag = TAG_DONE;
		}

		if (_viewSettings_Current->wpedbo_Options)
		{
D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: Existing options @ %x\n", _viewSettings_Current->wpedbo_Options));
		}

		switch ((int)_viewSettings_Current->wpedbo_Type)
		{
			case 5:
			{
				if ((strcmp(_viewSettings_Current->wpedbo_ViewName, "Workbench")) == 0)
				{
					newBG_RenderModes[newBG_DrawModeCount] = _WP_AdvancedViewRenderModeNames[IconWindowExt_ImageBackFill_RenderMode_Scale - 1];
					newBG_DrawModePages[newBG_DrawModeCount++] = 0;

D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: DrawMode %d = '%s'\n", newBG_DrawModeCount -1, newBG_RenderModes[newBG_DrawModeCount-1]));
				}
			}
			case 0:
			{
				newBG_RenderModes[newBG_DrawModeCount]	= _WP_AdvancedViewRenderModeNames[IconWindowExt_ImageBackFill_RenderMode_Tiled - 1];
				newBG_DrawModePages[newBG_DrawModeCount++] = 1;

				newBG_Options[newBG_OptionCount].ti_Tag = MUIA_IconWindowExt_ImageBackFill_BGRenderMode;
				if (_viewSettings_Current->wpedbo_Options)
					newBG_Options[newBG_OptionCount++].ti_Data = GetTagData(MUIA_IconWindowExt_ImageBackFill_BGRenderMode, IconWindowExt_ImageBackFill_RenderMode_Tiled, _viewSettings_Current->wpedbo_Options);
				else
				 newBG_Options[newBG_OptionCount++].ti_Data = IconWindowExt_ImageBackFill_RenderMode_Tiled;

				newBG_Options[newBG_OptionCount].ti_Tag = MUIA_IconWindowExt_ImageBackFill_BGTileMode;
				if (_viewSettings_Current->wpedbo_Options)
					newBG_Options[newBG_OptionCount++].ti_Data = GetTagData(MUIA_IconWindowExt_ImageBackFill_BGTileMode, IconWindowExt_ImageBackFill_TileMode_Float, _viewSettings_Current->wpedbo_Options);
				else
					newBG_Options[newBG_OptionCount++].ti_Data = IconWindowExt_ImageBackFill_TileMode_Float;
				
				newBG_Options[newBG_OptionCount].ti_Tag = MUIA_IconWindowExt_ImageBackFill_BGXOffset;
				if (_viewSettings_Current->wpedbo_Options)
					newBG_Options[newBG_OptionCount++].ti_Data = GetTagData(MUIA_IconWindowExt_ImageBackFill_BGXOffset, 0, _viewSettings_Current->wpedbo_Options);
				else
					newBG_Options[newBG_OptionCount++].ti_Data = 0;

				newBG_Options[newBG_OptionCount].ti_Tag = MUIA_IconWindowExt_ImageBackFill_BGYOffset;
				if (_viewSettings_Current->wpedbo_Options)
					newBG_Options[newBG_OptionCount++].ti_Data = GetTagData(MUIA_IconWindowExt_ImageBackFill_BGYOffset, 0, _viewSettings_Current->wpedbo_Options);
				else
					newBG_Options[newBG_OptionCount++].ti_Data = 0;

D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: DrawMode %d = '%s'\n", newBG_DrawModeCount -1, newBG_RenderModes[newBG_DrawModeCount-1]));

				break;
			}
		}

		if (newBG_OptionCount > 0)
		{
			IPTR old_bg_options = (IPTR) _viewSettings_Current->wpedbo_Options;

			_viewSettings_Current->wpedbo_Options = NULL;

			_viewSettings_Current->wpedbo_Options = AllocVec((sizeof(struct TagItem) * newBG_OptionCount + 1), MEMF_ANY|MEMF_CLEAR);
			do
			{
				_viewSettings_Current->wpedbo_Options[newBG_OptionCount - 1].ti_Tag = newBG_Options[newBG_OptionCount - 1].ti_Tag;
				_viewSettings_Current->wpedbo_Options[newBG_OptionCount - 1].ti_Data = newBG_Options[newBG_OptionCount - 1].ti_Data;
				newBG_OptionCount -= 1;
			}while(newBG_OptionCount > 0);

			SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_TileModeObj, MUIA_Cycle_Active, GetTagData(MUIA_IconWindowExt_ImageBackFill_BGTileMode, IconWindowExt_ImageBackFill_TileMode_Float, _viewSettings_Current->wpedbo_Options) - 1);
			SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_XOffsetObj, MUIA_String_Integer, GetTagData(MUIA_IconWindowExt_ImageBackFill_BGXOffset, 0, _viewSettings_Current->wpedbo_Options));
			SET(data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_YOffsetObj, MUIA_String_Integer, GetTagData(MUIA_IconWindowExt_ImageBackFill_BGYOffset, 0, _viewSettings_Current->wpedbo_Options));

			if (old_bg_options)
				FreeVec((struct TagItem *) old_bg_options);
		}

		if (newBG_DrawModeCount > 0)
		{
			IPTR setpage_active = (IPTR) (newBG_DrawModeCount - 1);

			IPTR old_bg_drawmodes = (IPTR) data->wped_AdvancedViewSettings_WindowData->wpedabwd_RenderModeObj_RenderModes;
			IPTR old_bg_drawpages = (IPTR) data->wped_AdvancedViewSettings_WindowData->wpedabwd_RenderModeObj_RenderPages;

D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: Old RenderModes @ %x, pages @ %x\n", old_bg_drawmodes, old_bg_drawpages));

			data->wped_AdvancedViewSettings_WindowData->wpedabwd_RenderModeObj_RenderModes = NULL;
			data->wped_AdvancedViewSettings_WindowData->wpedabwd_RenderModeObj_RenderPages = NULL;

			data->wped_AdvancedViewSettings_WindowData->wpedabwd_RenderModeObj_RenderModes = AllocVec((sizeof(STRPTR) * (newBG_DrawModeCount + 1)), MEMF_ANY|MEMF_CLEAR);
			data->wped_AdvancedViewSettings_WindowData->wpedabwd_RenderModeObj_RenderPages = AllocVec((sizeof(IPTR) * newBG_DrawModeCount), MEMF_ANY|MEMF_CLEAR);

D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: Allocated new RenderModes Array @ %x, page mappings @ %x\n", data->wped_AdvancedViewSettings_WindowData->wpedabwd_RenderModeObj_RenderModes, data->wped_AdvancedViewSettings_WindowData->wpedabwd_RenderModeObj_RenderPages));

			for (i = 0; i < newBG_DrawModeCount; i ++)
			{
				data->wped_AdvancedViewSettings_WindowData->wpedabwd_RenderModeObj_RenderModes[i] = newBG_RenderModes[i];
				data->wped_AdvancedViewSettings_WindowData->wpedabwd_RenderModeObj_RenderPages[i] = newBG_DrawModePages[i];
			}

			if ((_viewSettings_Current->wpedbo_Options) && (newBG_DrawModeCount > 1))
			{
				setpage_active = GetRenderModePage(
				                                   (char *) GetRenderModeName(GetTagData(MUIA_IconWindowExt_ImageBackFill_BGRenderMode, 
													 IconWindowExt_ImageBackFill_RenderMode_Tiled, 
													 _viewSettings_Current->wpedbo_Options)),
                                     				   data->wped_AdvancedViewSettings_WindowData->wpedabwd_RenderModeObj_RenderModes,
								   data->wped_AdvancedViewSettings_WindowData->wpedabwd_RenderModeObj_RenderPages);
			}

D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: Setting advanced page to  %d = '%s'\n", setpage_active, data->wped_AdvancedViewSettings_WindowData->wpedabwd_RenderModeObj_RenderModes[i]));

			Object *new_RenderModeObj = NULL;

D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: Creating RenderModes Cycle gadget\n"));
			new_RenderModeObj = MUI_MakeObject(MUIO_Cycle, 
							   NULL, 
							   data->wped_AdvancedViewSettings_WindowData->wpedabwd_RenderModeObj_RenderModes
							  );
			if (new_RenderModeObj)
			{
D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: Object @ %x\n", new_RenderModeObj));
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

D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: Setting cycle active \n"));

				SET((Object *) data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_RenderModeObj, MUIA_Cycle_Active, setpage_active);
			}
			if (old_bg_drawmodes)
				FreeVec((Object *) old_bg_drawmodes);
		}
    }
	else
	{
D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: Not an Image-type spec - Disabling Advanced options ..\n"));
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
 *Image contained in MUIC_Popimage is going to inserted by a call
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
			
			
			_viewSettings_Current->wpedbo_ImageSpecObject = PopimageObject,
										MUIA_FixWidth, 50,
										MUIA_FixHeight, 50,
										MUIA_Window_Title, __(MSG_SELECT_WORKBENCH_BACKGROUND),
										MUIA_Imageadjust_Type, MUIV_Imageadjust_Type_Background,
										MUIA_CycleChain,       1,
									End;
			if (_viewSettings_Current->wpedbo_ImageSpecObject)
			{
				if (backfillsupport)
					_viewSettings_Current->wpedbo_AdvancedOptionsObject = SimpleButton("Advanced");

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
		#if defined(DEBUG_CHANGEMENUBAR)
		*_WP_Navigation_HGrp2 =NULL,
		*_WP_Navigation_InnerHGrp3 = NULL,
		*_WP_Navigator_MenubarObj = NULL,
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
		#if defined(DEBUG_MULTLINE)
			*_WP_Icon_TextMultilineObj = NULL, 
			*_WP_Icon_TextMultilineOnFocusObj = NULL, 
			*_WP_Icon_DisplayedLinesNoObj = NULL, 	
		#endif
		*_WP_Icon_ListModeObj = NULL,
                *_WP_Icon_TextModeObj = NULL, 
		*_WP_Icon_TextLineMaxLenObj = NULL,
		*_WP_Icon_GroupObj = NULL,
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

	Object	*_WP_AdvancedViewWindow = NULL,
		*_WP_AdvancedViewWindowVGrp = NULL,
		*_WP_AdvancedViewRenderModeGrpObj = NULL,
		*_WP_AdvancedViewRenderModeObj = NULL,
		*_WP_AdvancedView_PageObj = NULL,
		*_WP_AdvancedView_ScaleModeGrpObj = NULL,
		*_WP_AdvancedView_TileModeGrpObj = NULL,
		*_WP_AdvancedView_TileModeObj = NULL, 
		*_WP_AdvancedView_X_OffsetObj = NULL,
		*_WP_AdvancedView_Y_OffsetObj = NULL,
		*_WP_AdvancedView_ButtonGrpObj = NULL,
		*_WP_AdvancedView_ButtonObj_Use = NULL,
		*_WP_AdvancedView_ButtonObj_Cancel = NULL;

/**/
D(bug("[WPEditor] WPEditor__OM_NEW()\n"));

    	//Object *cm_searchenabled;

/*main window----------------------------------------------------------------*/
/*self : Window?-------------------------------------------------------------*/

    self = (Object *) DoSuperNewTags(CLASS, self, NULL,
           	 			MUIA_PrefsEditor_Name, __(MSG_NAME),
            				MUIA_PrefsEditor_Path, (IPTR) "SYS/Wanderer.prefs",
        			    TAG_DONE);
/*END self-------------------------------------------------------------------*/

/*_WP_Prefs_PageGroupObj = Object for handling multi (3) page groups---------*/
	registerpages[WPD_GENERAL] = (STRPTR)_(MSG_GENERAL);
	registerpages[WPD_APPEARANCE] = (STRPTR)_(MSG_APPEARANCE);
	registerpages[WPD_TOOLBAR] = (STRPTR)_(MSG_TOOLBAR);
	
	_WP_Prefs_PageGroupObj = RegisterObject,
                			MUIA_Register_Titles, (IPTR) registerpages,      
            			 End;
/*END _WP_Prefs_PageGroupObj-------------------------------------------------*/

/*_WP_NavigationObj: "Navigation" page group---------------------------------*/
	
	_WP_NavigationObj = GroupObject, End;

		_WP_Navigation_HGrp1 = HGroup,                    // general 
                        		MUIA_FrameTitle, __(MSG_NAVIGATION),
                        		MUIA_Group_SameSize, TRUE,
                        		MUIA_Frame, MUIV_Frame_Group,
					MUIA_Group_Columns, 2,
                     	  	  End;

			_WP_Navigation_InnerHGrp1 = HGroup,
							MUIA_Group_Columns, 2,
							MUIA_Group_SameSize, FALSE,
                            				Child, (IPTR) Label1(_(MSG_METHOD)),
						    End;
				
				/*Navigation cycle button--------------------*/
				navigationtypelabels[WPD_NAVIGATION_CLASSIC] = (STRPTR)_(MSG_CLASSIC);
				navigationtypelabels[WPD_NAVIGATION_ENHANCED] = (STRPTR)_(MSG_ENHANCED);
				_WP_Navigation_TypeObj = MUI_MakeObject(MUIO_Cycle, 
									NULL, 
									navigationtypelabels);
				/*END Navigation cycle button----------------*/
			_WP_Navigation_InnerHGrp2 = HGroup,
							MUIA_Group_Columns, 2,
                                                        MUIA_Group_SameSize, FALSE,
						    End;
				#if defined(DEBUG_SHOWUSERFILES)
					_WP_UserFiles_ShowFileFolderObj = MUI_MakeObject(MUIO_Checkmark,NULL);
				#endif

			#if defined(DEBUG_CHANGEMENUBAR)
			_WP_Navigation_HGrp2 = HGroup,                   
                        		MUIA_FrameTitle, "Wanderer Menubar",
                        		MUIA_Group_SameSize, TRUE,
                        		MUIA_Frame, MUIV_Frame_Group,
					
                     	  	  End;

				_WP_Navigation_InnerHGrp3 = HGroup, End;
							
					_WP_Navigator_MenubarObj = StringObject,
									StringFrame,
									MUIA_String_MaxLen, 256,
									//MUIA_String_Contents, (IPTR)" ",
						     		   End;
			#endif
/*END _WP_NavigationObj------------------------------------------------------*/

/*_WP_Appearance_GroupObj: "Appearance" page group---------------------------*/

	_WP_Appearance_GroupObj= GroupObject,                     // appearance 
                    			MUIA_Group_SameSize, FALSE,
                    			MUIA_Group_Horiz, TRUE,
                			 End;
		
		/*Left part of Appearance*/
		_WP_ViewSettings_ScrollGrpObj = ScrollgroupObject,
				        		MUIA_Group_SameSize, FALSE,
							MUIA_Scrollgroup_FreeHoriz, FALSE,
							MUIA_Scrollgroup_FreeVert, TRUE,
							MUIA_Scrollgroup_Contents, 
                                                        (IPTR) (_WP_ViewSettings_VirtGrpObj = VirtgroupObject,
                                                                MUIA_FrameTitle, (IPTR)"View Settings",
								MUIA_Frame, MUIV_Frame_ReadList,
								MUIA_Virtgroup_Input, FALSE,
								End),
						End;	
			
			/*_WP_ViewSettings_GroupObj is going to contain nodes 
			  of list called _wpeditor_intern_ViewSettings, 
			  it's created after in this function...;
			*/
			_WP_ViewSettings_GroupObj = GroupObject,
							MUIA_Background, MUII_SHINE,
							Child, (IPTR) (_WP_ViewSettings_SpacerObj = HVSpace),
                                                    End;
		/**/

		/*Right part of Appearance*/
		_WP_Icon_GroupObj = GroupObject,
					MUIA_Group_SameSize, FALSE,
                        		MUIA_FrameTitle, __(MSG_ICONSPREFS),
                        		MUIA_Frame, MUIV_Frame_Group,
					MUIA_Group_Columns, 2,
                    		End;	
			
			/*Icon List Mode Cycle button------------------------*/
			iconlistmodes[WPD_ICONLISTMODE_GRID] = (STRPTR)_(MSG_ICONLISTMODE_GRID);
			iconlistmodes[WPD_ICONLISTMODE_PLAIN] = (STRPTR)_(MSG_ICONLISTMODE_PLAIN);
			icontextmodes[WPD_ICONTEXTMODE_OUTLINE] = (STRPTR)_(MSG_ICONTEXTMODE_OUTLINE);
			icontextmodes[WPD_ICONTEXTMODE_PLAIN] = (STRPTR)_(MSG_ICONTEXTMODE_PLAIN);

			_WP_Icon_ListModeObj = MUI_MakeObject(MUIO_Cycle, NULL, iconlistmodes);
			_WP_Icon_TextModeObj = MUI_MakeObject(MUIO_Cycle, NULL, icontextmodes);
			/*END Icon List Mode Cycle button--------------------*/
			#if defined(DEBUG_MULTLINE)
				_WP_Icon_DisplayedLinesNoObj = StringObject,
							   	        StringFrame,
                                                                        MUIA_String_MaxLen, 2,
                                                                        MUIA_String_Format, MUIV_String_Format_Right,
									MUIA_String_Accept, (IPTR)"0123456789",
                                                               End;

				_WP_Icon_TextMultilineObj = MUI_MakeObject(MUIO_Checkmark, NULL);
				_WP_Icon_TextMultilineOnFocusObj = MUI_MakeObject(MUIO_Checkmark, NULL);
			#endif

			_WP_Icon_TextLineMaxLenObj = StringObject,
							StringFrame,
							MUIA_String_MaxLen, 3,
							MUIA_String_Format, MUIV_String_Format_Right,
							MUIA_String_Accept, (IPTR)"0123456789",
						     End;
/*END _WP_Appearance_GroupObj------------------------------------------------*/

/*_WP_Toolbar_GroupObj: "Toolbar" page group---------------------------------*/
	
	_WP_Toolbar_GroupObj = GroupObject,                     // toolbar 
			               MUIA_Group_SameSize, FALSE,
                	       End;
	
		_WP_Toolbar_InnerGroupObj1 = HGroup,
                        			MUIA_FrameTitle,  __(MSG_OBJECTS),
                        			MUIA_Group_SameSize, TRUE,
                        			MUIA_Frame, MUIV_Frame_Group,
						MUIA_Group_Columns, 2,
                    			     End;
	
			_WP_Toolbar_InnerGroupObj2 = HGroup,
							MUIA_Group_Columns, 2,
							MUIA_Group_SameSize, FALSE,
							Child, (IPTR)Label1(_(MSG_TOOLBAR_ENABLED)),
						     End;
			
				_WP_Toolbar_EnabledObj = MUI_MakeObject(MUIO_Checkmark, NULL);


			_WP_Toolbar_InnerGroupObj3 = HGroup,
							MUIA_Group_Columns, 2,
							MUIA_Group_SameSize, FALSE,
							Child, (IPTR) HVSpace,
							Child, (IPTR) HVSpace,
							Child, (IPTR) HVSpace,
							Child, (IPTR) HVSpace,
						     End;

		_WP_Toolbar_PreviewObj = HGroup,
                        		   MUIA_FrameTitle, __(MSG_PREVIEW),
                        		   MUIA_Frame, MUIV_Frame_Group,
                        		   MUIA_Group_SameSize, FALSE,
                    			 End;	


			_WP_Toolbar_InnerGroupObj4 = HGroup,
						    	MUIA_HorizWeight, 0,
							MUIA_Group_SameSize, TRUE,
                        		             End;

				_WP_Toolbar_PreviewDirUpObj = ImageButton("", "THEME:Images/Gadgets/Prefs/Revert");
				_WP_Toolbar_PreviewSearchObj = ImageButton("", "THEME:Images/Gadgets/Prefs/Test");
/*END _WP_Toolbar_GroupObj---------------------------------------------------*/


/*Add main objects to main window (self?)------------------------------------*/

	/*Add navigation Objects to Navigation page*/
		
		DoMethod(_WP_Navigation_InnerHGrp1, OM_ADDMEMBER,_WP_Navigation_TypeObj);
		DoMethod(_WP_Navigation_InnerHGrp1, OM_ADDMEMBER,HVSpace);
		DoMethod(_WP_Navigation_InnerHGrp1, OM_ADDMEMBER,HVSpace);
		#if defined(DEBUG_NETWORKBROWSER)
			_WP_NetworkBrowser_EnabledObj = MUI_MakeObject(MUIO_Checkmark, NULL);
			DoMethod(_WP_Navigation_InnerHGrp2, OM_ADDMEMBER,Label1("Network Browser on Workbench"));
			DoMethod(_WP_Navigation_InnerHGrp2, OM_ADDMEMBER,_WP_NetworkBrowser_EnabledObj);
		#else
			DoMethod(_WP_Navigation_InnerHGrp2, OM_ADDMEMBER,HVSpace);
			DoMethod(_WP_Navigation_InnerHGrp2, OM_ADDMEMBER,HVSpace);
		#endif
		#if defined(DEBUG_SHOWUSERFILES)
			
			DoMethod(_WP_Navigation_InnerHGrp2, OM_ADDMEMBER,Label1("User Files Folder on Workbench"));
			DoMethod(_WP_Navigation_InnerHGrp2, OM_ADDMEMBER,_WP_UserFiles_ShowFileFolderObj);
		#else
			DoMethod(_WP_Navigation_InnerHGrp2, OM_ADDMEMBER,HVSpace);
			DoMethod(_WP_Navigation_InnerHGrp2, OM_ADDMEMBER,HVSpace);
		#endif
		
		DoMethod(_WP_Navigation_InnerHGrp2, OM_ADDMEMBER,HVSpace);
		DoMethod(_WP_Navigation_InnerHGrp2, OM_ADDMEMBER,HVSpace);
	
		#if defined(DEBUG_CHANGEMENUBAR)
		DoMethod(_WP_Navigation_InnerHGrp3, OM_ADDMEMBER,_WP_Navigator_MenubarObj);
		DoMethod(_WP_Navigation_HGrp2, OM_ADDMEMBER,_WP_Navigation_InnerHGrp3);
		#endif
		
		DoMethod(_WP_Navigation_HGrp1, OM_ADDMEMBER,_WP_Navigation_InnerHGrp1);
		DoMethod(_WP_Navigation_HGrp1, OM_ADDMEMBER,_WP_Navigation_InnerHGrp2);
		
		DoMethod(_WP_NavigationObj, OM_ADDMEMBER,_WP_Navigation_HGrp1);
		#if defined(DEBUG_CHANGEMENUBAR)
		DoMethod(_WP_NavigationObj, OM_ADDMEMBER,_WP_Navigation_HGrp2);
		#endif
	/**/
	
	/*Add appearance Objects to Appearance page*/
		/*Add objects which are contain into right part*/
			DoMethod(_WP_Icon_GroupObj, OM_ADDMEMBER,Label1(_(MSG_ICONLISTMODE)));
			DoMethod(_WP_Icon_GroupObj, OM_ADDMEMBER,_WP_Icon_ListModeObj);
			DoMethod(_WP_Icon_GroupObj, OM_ADDMEMBER,HVSpace);
			DoMethod(_WP_Icon_GroupObj, OM_ADDMEMBER,HVSpace);
			DoMethod(_WP_Icon_GroupObj, OM_ADDMEMBER,Label1(_(MSG_ICONTEXTMODE)));
			DoMethod(_WP_Icon_GroupObj, OM_ADDMEMBER,_WP_Icon_TextModeObj);
			DoMethod(_WP_Icon_GroupObj, OM_ADDMEMBER,Label1("Max. Label line length .."));	
			DoMethod(_WP_Icon_GroupObj, OM_ADDMEMBER,_WP_Icon_TextLineMaxLenObj);
			#if defined(DEBUG_MULTLINE)
				DoMethod(_WP_Icon_GroupObj, OM_ADDMEMBER,Label1("Use MultiLine Labels "));	
				DoMethod(_WP_Icon_GroupObj, OM_ADDMEMBER,_WP_Icon_TextMultilineObj);
				DoMethod(_WP_Icon_GroupObj, OM_ADDMEMBER,Label1("Only show for Focus(ed) Icon "));	
				DoMethod(_WP_Icon_GroupObj, OM_ADDMEMBER,_WP_Icon_TextMultilineOnFocusObj);
				DoMethod(_WP_Icon_GroupObj, OM_ADDMEMBER,Label1("No. of lines to display .."));
				DoMethod(_WP_Icon_GroupObj, OM_ADDMEMBER,_WP_Icon_DisplayedLinesNoObj);
			#endif
			DoMethod(_WP_Icon_GroupObj, OM_ADDMEMBER,HVSpace);	
			DoMethod(_WP_Icon_GroupObj, OM_ADDMEMBER,HVSpace);
		/**/			

		/*Add objects which are contain into left part*/
			DoMethod(_WP_ViewSettings_VirtGrpObj, OM_ADDMEMBER,_WP_ViewSettings_GroupObj);
		/**/

		DoMethod(_WP_Appearance_GroupObj, OM_ADDMEMBER,_WP_ViewSettings_ScrollGrpObj);
		DoMethod(_WP_Appearance_GroupObj, OM_ADDMEMBER,_WP_Icon_GroupObj);
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
	_WP_AdvancedViewWindow = WindowObject,
				 	MUIA_Window_CloseGadget, FALSE,
					MUIA_Window_Title, (IPTR)"Advanced Options ..",
					WindowContents, (IPTR) (_WP_AdvancedViewWindowVGrp = VGroup, End),
			 	 End;

	/*Draw Mode Group----------------------------------------------------*/    	
	_WP_AdvancedViewRenderModeGrpObj = HGroup, Child, (IPTR) Label1("Draw Mode : "), End;
		
		/*AdvancedViewRenderMode cicle button------------------------*/
			/*_WP_AdvancedViewRenderModeValues: they are values 
		  	  associated (with GetRenderMode...() hook functions) 
			  to AdvancedViewRenderModeNames' ones;
				*/
		_WP_AdvancedViewRenderModeNames[IconWindowExt_ImageBackFill_RenderMode_Tiled - 1] = "Tiled";
		_WP_AdvancedViewRenderModeValues[IconWindowExt_ImageBackFill_RenderMode_Tiled - 1] = IconWindowExt_ImageBackFill_RenderMode_Tiled;
		_WP_AdvancedViewRenderModeNames[IconWindowExt_ImageBackFill_RenderMode_Scale - 1] = "Scaled";
		_WP_AdvancedViewRenderModeValues[IconWindowExt_ImageBackFill_RenderMode_Scale - 1] = IconWindowExt_ImageBackFill_RenderMode_Scale;
		_WP_AdvancedViewRenderModeObj = MUI_MakeObject(MUIO_Cycle, 
							       NULL, 
							       _WP_AdvancedViewRenderModeNames);
		/*AdvancedViewRenderMode cicle button------------------------*/
	/*END Draw Mode Group------------------------------------------------*/

	/*Scale Mode/Tile Mode Group-----------------------------------------*/
	_WP_AdvancedView_PageObj = GroupObject,
						MUIA_Group_PageMode, TRUE,
					 End;

		_WP_AdvancedView_ScaleModeGrpObj = GroupObject,
							MUIA_Group_SameSize, FALSE,
							MUIA_FrameTitle, "Scale Mode Options ..",
							MUIA_Frame, MUIV_Frame_Group,
							Child, HVSpace,
						   End;
		
		/*AdvancedView_TileModeNames cicle button--------------------*/
			/*_WP_AdvancedView_TileModeValues: they are values 
		  	  associated (with 
			  WandererPrefs_Hook_CloseAdvancedOptionsFunc() hook 
			  function) to _WP_AdvancedView_TileModeNames' ones;
			*/
		_WP_AdvancedView_TileModeNames[IconWindowExt_ImageBackFill_TileMode_Float - 1] = "Floating";
		_WP_AdvancedView_TileModeValues[IconWindowExt_ImageBackFill_TileMode_Float - 1] = IconWindowExt_ImageBackFill_TileMode_Float;
		_WP_AdvancedView_TileModeNames[IconWindowExt_ImageBackFill_TileMode_Fixed - 1] = "Fixed";
		_WP_AdvancedView_TileModeValues[IconWindowExt_ImageBackFill_TileMode_Fixed - 1] = IconWindowExt_ImageBackFill_TileMode_Fixed;
		_WP_AdvancedView_TileModeObj = MUI_MakeObject(MUIO_Cycle, 
						    		    NULL, 
						    		    _WP_AdvancedView_TileModeNames);
		/*END AdvancedView_TileModeNames cicle button----------------*/

 			_WP_AdvancedView_TileModeGrpObj = GroupObject,	
								MUIA_Group_SameSize, FALSE,
								MUIA_FrameTitle, "Tile Mode Options ..",
								MUIA_Frame, MUIV_Frame_Group,
								MUIA_Group_Columns, 2,
					 		  End;

				_WP_AdvancedView_X_OffsetObj = StringObject,
								StringFrame,
								MUIA_String_MaxLen, 3,
								MUIA_String_Accept, "0123456789",
				    		    	       End;

				_WP_AdvancedView_Y_OffsetObj = StringObject,
								StringFrame,
								MUIA_String_MaxLen, 3,
								MUIA_String_Accept, "0123456789",
				    		    	       End;
	/*END Scale Mode/Tile Mode Group-------------------------------------*/

	/*Button Group-------------------------------------------------------*/
	_WP_AdvancedView_ButtonGrpObj = HGroup,
					   Child, _WP_AdvancedView_ButtonObj_Use = 
						ImageButton("Use", "THEME:Images/Gadgets/Prefs/Use"),
					   Child, _WP_AdvancedView_ButtonObj_Cancel = 
						ImageButton("Cancel", "THEME:Images/Gadgets/Prefs/Cancel"),
				        End;
	/*END Button Group--------------------------------------------------*/

	/*END Window--------------------------------------------------------*/


/*Add advanced view objects to AdvancedViewWindow object--------------------*/
	DoMethod(_WP_AdvancedViewRenderModeGrpObj, OM_ADDMEMBER,_WP_AdvancedViewRenderModeObj);

	DoMethod(_WP_AdvancedView_TileModeGrpObj, OM_ADDMEMBER, Label1("Tile Mode : "));
	DoMethod(_WP_AdvancedView_TileModeGrpObj, OM_ADDMEMBER, _WP_AdvancedView_TileModeObj);
	DoMethod(_WP_AdvancedView_TileModeGrpObj, OM_ADDMEMBER, Label1("X Offset : "));
	DoMethod(_WP_AdvancedView_TileModeGrpObj, OM_ADDMEMBER, _WP_AdvancedView_X_OffsetObj);
	DoMethod(_WP_AdvancedView_TileModeGrpObj, OM_ADDMEMBER, Label1("Y Offset : "));
	DoMethod(_WP_AdvancedView_TileModeGrpObj, OM_ADDMEMBER, _WP_AdvancedView_Y_OffsetObj);

	DoMethod(_WP_AdvancedView_PageObj, OM_ADDMEMBER, _WP_AdvancedView_ScaleModeGrpObj);
	DoMethod(_WP_AdvancedView_PageObj, OM_ADDMEMBER, _WP_AdvancedView_TileModeGrpObj);

	DoMethod(_WP_AdvancedViewWindowVGrp, OM_ADDMEMBER,_WP_AdvancedViewRenderModeGrpObj);
	DoMethod(_WP_AdvancedViewWindowVGrp, OM_ADDMEMBER,_WP_AdvancedView_PageObj);
	DoMethod(_WP_AdvancedViewWindowVGrp, OM_ADDMEMBER,_WP_AdvancedView_ButtonGrpObj);
/*END Add advanced view objects to AdvancedViewWindow object-----------------*/
/*END AdvancedViewWindow-----------------------------------------------------*/

/*-------------------*/
    if ((self != NULL) && (_WP_AdvancedViewWindow != NULL ))
     {
        data = INST_DATA(CLASS, self);

D(bug("[WPEditor] WPEditor__OM_NEW: Prefs Object (self) @ %x\n", self));

	advancedView_data = AllocMem(sizeof(struct WPEditor_AdvancedBackgroundWindow_DATA), MEMF_CLEAR);
	advancedView_data->wpedabwd_Hook_DrawModeChage.h_Entry = (HOOKFUNC) WandererPrefs_Hook_DrawModeChangeFunc;
	advancedView_data->wpedabwd_Window_WindowObj           = _WP_AdvancedViewWindow;
	advancedView_data->wpedabwd_Window_RenderModeGrpObj    =_WP_AdvancedViewRenderModeGrpObj ;
	advancedView_data->wpedabwd_Window_RenderModeObj       = _WP_AdvancedViewRenderModeObj;
	advancedView_data->wpedabwd_Window_PageObj             = _WP_AdvancedView_PageObj;
	advancedView_data->wpedabwd_Window_TileModeObj         = _WP_AdvancedView_TileModeObj;
	advancedView_data->wpedabwd_Window_XOffsetObj          = _WP_AdvancedView_X_OffsetObj;
	advancedView_data->wpedabwd_Window_YOffsetObj          = _WP_AdvancedView_Y_OffsetObj;
	advancedView_data->wpedabwd_Window_UseObj 	       = _WP_AdvancedView_ButtonObj_Use;
	advancedView_data->wpedabwd_Window_CancelObj	       = _WP_AdvancedView_ButtonObj_Cancel;

D(bug("[WPEditor] WPEditor__OM_NEW: 'Advanced' Window Object @ %x\n", advancedView_data->wpedabwd_Window_WindowObj));

	_wpeditor_intern_CLASS = CLASS;

	data->wped_AdvancedViewSettings_WindowData    = advancedView_data;
		
        data->wped_ViewSettings_GroupObj              = _WP_ViewSettings_GroupObj;
        data->wped_ViewSettings_SpacerObj             = _WP_ViewSettings_SpacerObj;
		
        data->wped_c_NavigationMethod                 = _WP_Navigation_TypeObj;
        data->wped_cm_ToolbarEnabled                  = _WP_Toolbar_EnabledObj;
#if defined(DEBUG_CHANGEMENUBAR)
	data->wped_s_menubar			      =_WP_Navigator_MenubarObj;
#endif
#if defined(DEBUG_NETWORKBROWSER)
        data->wped_cm_EnableNetworkBrowser            = _WP_NetworkBrowser_EnabledObj;
#endif
#if defined(DEBUG_SHOWUSERFILES)
        data->wped_cm_EnableUserFiles                 = _WP_UserFiles_ShowFileFolderObj;
#endif

#if defined(DEBUG_MULTLINE)
        data->wped_icon_textmultiline                 = _WP_Icon_TextMultilineObj;
        data->wped_icon_multilineonfocus              = _WP_Icon_TextMultilineOnFocusObj;
        data->wped_icon_multilineno                   = _WP_Icon_DisplayedLinesNoObj; 	
#endif
        data->wped_toolbarpreview                     = _WP_Toolbar_PreviewObj;
		
        data->wped_icon_listmode                      = _WP_Icon_ListModeObj;
        data->wped_icon_textmode                      = _WP_Icon_TextModeObj;

        data->wped_icon_textmaxlen                    = _WP_Icon_TextLineMaxLenObj;
        data->wped_toolbarGroup                       = _WP_Toolbar_GroupObj;
        data->wped_Hook_CloseAdvancedOptions.h_Entry  = ( HOOKFUNC )WandererPrefs_Hook_CloseAdvancedOptionsFunc;

        //-- Setup notifications -------------------------------------------
        DoMethod
	(
            data->wped_icon_listmode, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        ); 
        DoMethod
        (
            data->wped_icon_textmode, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );        

#warning "TODO: The toolbar class will become an external module to wanderer with its own prefs"
#if defined(DEBUG_TOOLBARINTERNAL)
	/*	Enhanced navigation depends on the toolbar class for
		control - so we disable it if the toolbar isnt available! */
	DoMethod
        (
            data->wped_cm_ToolbarEnabled, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,  
            (IPTR) data->wped_c_NavigationMethod, 3, MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue
        ); 

	/* Only enable the preview if the toolbar is enabled */
	DoMethod
        (
            data->wped_cm_ToolbarEnabled, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,  
            (IPTR) data->wped_toolbarpreview, 3, MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue
        );

	DoMethod
        (
            data->wped_cm_ToolbarEnabled, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,  
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        ); 

        DoMethod
        (
            data->wped_c_NavigationMethod, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,  
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );

#endif
#if defined(DEBUG_NETWORKBROWSER)
        DoMethod
        (
            data->wped_cm_EnableNetworkBrowser, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
#endif

#if defined(DEBUG_SHOWUSERFILES)
        DoMethod
        (
            data->wped_cm_EnableUserFiles, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
#endif

#if defined(DEBUG_MULTLINE)
        DoMethod
        (
            data->wped_icon_textmultiline, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,  
            (IPTR) data->wped_icon_multilineonfocus, 3, MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue
        );

        DoMethod
        (
            data->wped_icon_textmultiline, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,  
            (IPTR) data->wped_icon_multilineno, 3, MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue
        );

        SET(data->wped_icon_textmultiline, MUIA_Selected, TRUE);
        SET(data->wped_icon_multilineno, MUIA_String_Integer, 3);

        DoMethod
        (
            data->wped_icon_textmultiline, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );

        DoMethod
        (
            data->wped_icon_multilineonfocus, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );

        DoMethod
        (
            data->wped_icon_multilineno, MUIM_Notify, MUIA_String_Integer, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
#endif

        // Icon textmode maxlength
        DoMethod ( 
            data->wped_icon_textmaxlen, MUIM_Notify, MUIA_String_Integer, MUIV_EveryTime,  
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE 
        );
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
		Object 	*thisViewImspecGrp = NULL;
		Object 	*thisViewAdvancedGrp = NULL;

D(bug("[WPEditor] WPEditor__OM_NEW: Adding ViewSetting Objects for '%s' to Prefs GUI ..\n", _viewSettings_Node->wpedbo_ViewName));

		thisViewImspecGrp = GroupObject,
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
			thisViewAdvancedGrp = GroupObject,
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
			DoMethod(
				 _viewSettings_Node->wpedbo_ImageSpecObject,
				 MUIM_Notify, MUIA_Imagedisplay_Spec, MUIV_EveryTime,
				 (IPTR)self, 3, MUIM_CallHook, 
				 &_viewSettings_Node->wpedbo_Hook_CheckImage, _viewSettings_Node
				);

			if (_viewSettings_Node->wpedbo_AdvancedOptionsObject)
			{
				_viewSettings_Node->wpedbo_Hook_OpenAdvancedOptions.h_Entry = ( HOOKFUNC )WandererPrefs_Hook_OpenAdvancedOptionsFunc;
				
				DoMethod(
				         _viewSettings_Node->wpedbo_AdvancedOptionsObject, MUIM_Notify, MUIA_Pressed, FALSE,
					 (IPTR)self, 3, MUIM_CallHook, &_viewSettings_Node->wpedbo_Hook_OpenAdvancedOptions, _viewSettings_Node
					);

				SET(_viewSettings_Node->wpedbo_AdvancedOptionsObject, MUIA_Disabled, TRUE);
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
	if (advancedView_data->wpedabwd_Window_WindowObj) DoMethod(advancedView_data->wpedabwd_Window_WindowObj, OM_DISPOSE);
		if (self) DoMethod(self, OM_DISPOSE);

		self = NULL;
     }
/*--------------*/

     return self;
}


/*BOOL WPEditor_ProccessGlobalChunk(): read a TagItem global_chunk (from global setting) 
 *and memorize its value into correspondent attribute of an object of the gui...;
 */
BOOL WPEditor_ProccessGlobalChunk(Class *CLASS, Object *self, struct TagItem *global_chunk)
{
    SETUP_WPEDITOR_INST_DATA;

	int i = 0;
	BOOL cont = TRUE;
	//BOOL state = FALSE;

D(bug("[WPEditor] WPEditor_ProccessGlobalChunk()\n"));

	for (i = 0; i < WP_GLOBALTAGCOUNT; i++)
	{
		if (cont)
		{
			switch ((int)global_chunk[i].ti_Tag)
			{
				case MUIA_IconWindowExt_Toolbar_Enabled:
				{
D(bug("[WPEditor] WPEditor_ProccessGlobalChunk: Tag %d = MUIA_IconWindowExt_Toolbar_Enabled, val = %d\n", i, global_chunk[i].ti_Data));
					SET(data->wped_cm_ToolbarEnabled, MUIA_Selected, (BOOL)global_chunk[i].ti_Data); 
					break;
				}
				case MUIA_IconWindowExt_Toolbar_NavigationMethod:
				{
D(bug("[WPEditor] WPEditor_ProccessGlobalChunk: Tag %d = MUIA_IconWindowExt_Toolbar_NavigationMethod, val = %d\n", i, global_chunk[i].ti_Data));
					SET(data->wped_c_NavigationMethod, MUIA_Cycle_Active, (IPTR)global_chunk[i].ti_Data);   
					break;
				}
#if defined(DEBUG_SHOWUSERFILES)
				case MUIA_IconWindowExt_UserFiles_ShowFilesFolder:
				{
D(bug("[WPEditor] WPEditor_ProccessGlobalChunk: Tag %d = MUIA_IconWindowExt_UserFiles_ShowFilesFolder, val = %d\n", i, global_chunk[i].ti_Data));
					SET(data->wped_cm_EnableUserFiles, MUIA_Selected, (BOOL)global_chunk[i].ti_Data);
					break;
				}
#endif

#if defined(DEBUG_CHANGEMENUBAR)
				case MUIA_IconWindowExt_Menubar_String:
				{
D(bug("[WPEditor] WPEditor_ProccessGlobalChunk: Tag %d = MUIA_IconWindowExt_Menubar_String, val = %d\n", i, global_chunk[i].ti_Data));
					SET(data->wped_s_menubar, MUIA_String_Contents, (STRPTR)global_chunk[i].ti_Data);
					
//D(bug("[WPEditor] WPEditor_ProccessGlobalChunk MUIA_IconWindowExt_Menubar_String in memory: '%s'\n",global_chunk[i].ti_Data));
					break;
				}
#endif

/* The Following attributes will be moved to the ViewSettings Specific Chunks */
				case MUIA_IconList_IconListMode:
				{
D(bug("[WPEditor] WPEditor_ProccessGlobalChunk: Tag %d = MUIA_IconList_IconListMode, val = %d\n", i, global_chunk[i].ti_Data));
					SET( data->wped_icon_listmode, MUIA_Cycle_Active, (IPTR)global_chunk[i].ti_Data);
					break;
				}
				case MUIA_IconList_LabelText_Mode:
				{
D(bug("[WPEditor] WPEditor_ProccessGlobalChunk: Tag %d = MUIA_IconList_LabelText_Mode, val = %d\n", i, global_chunk[i].ti_Data));
					SET( data->wped_icon_textmode, MUIA_Cycle_Active, (IPTR)global_chunk[i].ti_Data);
					break;
				}
				case MUIA_IconList_LabelText_MaxLineLen:
				{
D(bug("[WPEditor] WPEditor_ProccessGlobalChunk: Tag %d = MUIA_IconList_LabelText_MaxLineLen, val = %d\n", i, global_chunk[i].ti_Data));
					SET(data->wped_icon_textmaxlen, MUIA_String_Integer, (IPTR)global_chunk[i].ti_Data);
					break;
				}
#if defined(DEBUG_MULTLINE)
				case MUIA_IconList_LabelText_MultiLine:
				{
D(bug("[WPEditor] WPEditor_ProccessGlobalChunk: Tag %d = MUIA_IconList_LabelText_MultiLine, val = %d\n", i, global_chunk[i].ti_Data));
					if ((IPTR)global_chunk[i].ti_Data > 1)
					{
						SET(data->wped_icon_multilineno, MUIA_String_Integer, (IPTR)global_chunk[i].ti_Data);
						SET(data->wped_icon_textmultiline, MUIA_Selected, TRUE);
					}
					else
					{
						SET(data->wped_icon_multilineno, MUIA_String_Integer, 1);
						SET(data->wped_icon_textmultiline, MUIA_Selected, FALSE);
					}
					break;
				}
				case MUIA_IconList_LabelText_MultiLineOnFocus:
				{
D(bug("[WPEditor] WPEditor_ProccessGlobalChunk: Tag %d = MUIA_IconList_LabelText_MultiLineOnFocus, val = %d\n", i, global_chunk[i].ti_Data));
					SET(data->wped_icon_multilineonfocus, MUIA_Selected, (BOOL)global_chunk[i].ti_Data);
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

	struct TagItem *network_tags = _viewSettings_Chunk;
	SET(data->wped_cm_EnableNetworkBrowser, MUIA_Selected, network_tags[0].ti_Data);

	return TRUE;
}
#endif

/*Renabled WPEditor_ProccessViewSettingsChunk() as Nic Andrews (nicja@yahoo.com) has asked...;
 *I don't understand why
 *it must rebuild a new ViewSetting object to add an image to its PopImage gadget...;
 *All this function sounds like redundant...;
 *I've added 3 lines of codes (now disabled) to WPEditor__MUIM_PrefsEditor_ImportFH() for updating
 * the viewsetting objects without call this redundant function...;
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
D(bug("[WPEditor] WPEditor_ProccessViewSettingsChunk: Updating Existing node @ %x\n", _viewSettings_Node));
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

		Object 		*thisViewImspecGrp = NULL;
		Object 		*thisViewAdvancedGrp = NULL;

D(bug("[WPEditor] WPEditor_ProccessViewSettingsChunk: Adding ViewSetting Objects for '%s' to Prefs GUI ..\n", _viewSettings_Node->wpedbo_ViewName));

		thisViewImspecGrp = GroupObject,
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
			thisViewAdvancedGrp = GroupObject,
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
				DoMethod (
					  _viewSettings_Node->wpedbo_AdvancedOptionsObject, MUIM_Notify, MUIA_Pressed, FALSE,
					  (IPTR)data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_WindowObj, 3, MUIM_Set, MUIA_Window_Open, TRUE
					 );

			SET(_viewSettings_Node->wpedbo_AdvancedOptionsObject, MUIA_Disabled, TRUE);
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
D(bug("[WPEditor] WPEditor_ProccessViewSettingsChunk: Freeing old ViewSetting Tag data @ %x\n", _viewSettings_Node->wpedbo_Options));
			FreeVec(_viewSettings_Node->wpedbo_Options);
		}

		int tag_count = (chunk_size - _viewSettings_TagOffset)/sizeof(struct TagItem);

		_viewSettings_Node->wpedbo_Options = AllocVec((tag_count + 1) * sizeof(struct TagItem), 
							      MEMF_ANY|MEMF_CLEAR
							     );
		if (_viewSettings_Node->wpedbo_Options)
		{
D(bug("[WPEditor] WPEditor_ProccessViewSettingsChunk: Allocated new Tag storage @ %x [%d bytes] \n", _viewSettings_Node->wpedbo_Options, chunk_size - _viewSettings_TagOffset));
			CopyMem(_viewSettings_Chunk + _viewSettings_TagOffset, _viewSettings_Node->wpedbo_Options, tag_count * sizeof(struct TagItem));
			_viewSettings_Node->wpedbo_Options[tag_count].ti_Tag = TAG_DONE;
		}
	}

	SET(_viewSettings_Node->wpedbo_ImageSpecObject, MUIA_Imagedisplay_Spec, _viewSettings_Chunk);

	return TRUE;
}
*/


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
    
    struct ContextNode     *context;
    struct IFFHandle       *handle;
    BOOL                   success = TRUE;
    LONG                   error;
    IPTR                   iff_parse_mode = IFFPARSE_SCAN;
    UBYTE                  chunk_buffer[WP_IFF_CHUNK_BUFFER_SIZE];

D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH()\n"));

    if (!(handle = AllocIFF()))
        return FALSE;

D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH: Iff current handle %x, msg handle %x\n", handle->iff_Stream, message->fh));

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
				context = CurrentChunk(handle);
				iff_parse_mode = IFFPARSE_STEP;

D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH: Context %x\n", context));
					
				if ((error = ReadChunkBytes(handle, chunk_buffer, WP_IFF_CHUNK_BUFFER_SIZE)))
				{
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH: ReadChunkBytes() Chunk matches Prefs Header size ..\n"));
					struct WandererPrefsIFFChunkHeader *this_header = (struct WandererPrefsIFFChunkHeader *) chunk_buffer;
					char                               *this_chunk_name = NULL;
					IPTR                               this_chunk_size = this_header->wpIFFch_ChunkSize;
						
					this_chunk_name = AllocVec(strlen(this_header->wpIFFch_ChunkType) +1,
								   MEMF_ANY|MEMF_CLEAR
								  );
					if (this_chunk_name)
					{
						strcpy(this_chunk_name, this_header->wpIFFch_ChunkType);
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH: Prefs Header for '%s' data size %d bytes\n", this_chunk_name, this_chunk_size));

						if ((error = ParseIFF(handle, IFFPARSE_STEP)) == IFFERR_EOC)
						{
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH: End of header chunk ..\n"));

							if ((error = ParseIFF(handle, IFFPARSE_STEP)) == 0)
							{
								context = CurrentChunk(handle);

D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH: Context %x\n", context));

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
										WPEditor_ProccessGlobalChunk(CLASS, self,(struct TagItem *) chunk_buffer);
									}
								#if defined(DEBUG_NETWORKBROWSER)
									else if ((strcmp(this_chunk_name, "wanderer:network")) == 0)
									{
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH: Process data for wanderer network config chunk ..\n"));
										WPEditor_ProccessNetworkChunk(CLASS, self, chunk_buffer);
									}
								#endif
									else if ((strncmp(this_chunk_name, "wanderer:viewsettings", strlen("wanderer:viewsettings"))) == 0)
									{
										char *view_name = this_chunk_name + strlen("wanderer:viewsettings") + 1;
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH: Process data for wanderer background config chunk '%s'..\n", view_name));
										WPEditor_ProccessViewSettingsChunk(CLASS, self, view_name, chunk_buffer, this_chunk_size);
										/*these 3 following lines replace the call to
										 *WPEditor_ProccessViewSettingsChunk() that seems redundant...;
										 */
										//struct WPEditor_ViewSettingsObject  *_viewSettings_Node = NULL;
										//_viewSettings_Node = WPEditor__FindViewSettingObjects(view_name);
										//SET(_viewSettings_Node->wpedbo_ImageSpecObject, MUIA_Imagedisplay_Spec, chunk_buffer);
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
		//success = FALSE;// this brokes cancel button
	}//END if ((error = StopChunk(handle, ID_PREF, ID_WANDR)) == 0)

        CloseIFF(handle);
    }
    else
    {
D(bug("[WPEditor] Failed to open stream!, returncode %ld!\n", error));
        //ShowError(_(MSG_CANT_OPEN_STREAM));
	success = FALSE;
    }//END if ((error = OpenIFF(handle, IFFF_READ)) == 0)

    FreeIFF(handle);
    
    return success;
}

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
    int positionTemp =0;

D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH()\n"));

    if ((handle = AllocIFF()))
    {
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: Iff current handle %x, msg handle %x\n", handle->iff_Stream, message->fh));

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
			struct TagItem	*_wp_GlobalTags = AllocVec(((WP_GLOBALTAGCOUNT + 1) * sizeof(struct TagItem)), MEMF_ANY|MEMF_CLEAR);
			ULONG           _wp_GlobalTagCounter = 0;

			_wp_GlobalTags[_wp_GlobalTagCounter].ti_Tag = MUIA_IconWindowExt_Toolbar_Enabled;
			GET(data->wped_cm_ToolbarEnabled, MUIA_Selected, &_wp_GlobalTags[_wp_GlobalTagCounter].ti_Data);
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'global' MUIA_IconWindowExt_Toolbar_Enabled @ Tag %d, data = %d\n", _wp_GlobalTagCounter, _wp_GlobalTags[_wp_GlobalTagCounter].ti_Data));
			_wp_GlobalTagCounter += 1;

			if ((BOOL)_wp_GlobalTags[_wp_GlobalTagCounter - 1].ti_Data == TRUE)
			{
				/* save navigation bahaviour */
				_wp_GlobalTags[_wp_GlobalTagCounter].ti_Tag = MUIA_IconWindowExt_Toolbar_NavigationMethod;
				GET(data->wped_c_NavigationMethod, MUIA_Cycle_Active, &_wp_GlobalTags[_wp_GlobalTagCounter].ti_Data);
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'global' MUIA_IconWindowExt_Toolbar_NavigationMethod @ Tag %d, data = %d\n", _wp_GlobalTagCounter, _wp_GlobalTags[_wp_GlobalTagCounter].ti_Data));
				_wp_GlobalTagCounter += 1;
			}
			
			/* save the icon listing method */
			_wp_GlobalTags[_wp_GlobalTagCounter].ti_Tag = MUIA_IconList_IconListMode;
			GET(data->wped_icon_listmode, MUIA_Cycle_Active, &_wp_GlobalTags[_wp_GlobalTagCounter].ti_Data);
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'global' MUIA_IconList_IconListMode @ Tag %d, data = %d\n", _wp_GlobalTagCounter, _wp_GlobalTags[_wp_GlobalTagCounter].ti_Data));
			_wp_GlobalTagCounter += 1;

			/* save the icon text mode */
			_wp_GlobalTags[_wp_GlobalTagCounter].ti_Tag = MUIA_IconList_LabelText_Mode;
			GET(data->wped_icon_textmode, MUIA_Cycle_Active, &_wp_GlobalTags[_wp_GlobalTagCounter].ti_Data);
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'global' MUIA_IconList_LabelText_Mode @ Tag %d, data = %d\n", _wp_GlobalTagCounter, _wp_GlobalTags[_wp_GlobalTagCounter].ti_Data));
			_wp_GlobalTagCounter += 1;

			/* save the max length of icons */
			_wp_GlobalTags[_wp_GlobalTagCounter].ti_Tag = MUIA_IconList_LabelText_MaxLineLen;
			GET(data->wped_icon_textmaxlen, MUIA_String_Integer, &_wp_GlobalTags[_wp_GlobalTagCounter].ti_Data);
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'global' MUIA_IconList_LabelText_MaxLineLen @ Tag %d, data = %d\n", _wp_GlobalTagCounter, _wp_GlobalTags[_wp_GlobalTagCounter ].ti_Data));
			_wp_GlobalTagCounter += 1;

#if defined(DEBUG_SHOWUSERFILES)
			_wp_GlobalTags[_wp_GlobalTagCounter].ti_Tag = MUIA_IconWindowExt_UserFiles_ShowFilesFolder;
			GET(data->wped_cm_EnableUserFiles, MUIA_Selected, &_wp_GlobalTags[_wp_GlobalTagCounter].ti_Data);
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'global' MUIA_IconWindowExt_UserFiles_ShowFilesFolder @ Tag %d, data = %d\n", _wp_GlobalTagCounter, _wp_GlobalTags[_wp_GlobalTagCounter].ti_Data));
			_wp_GlobalTagCounter += 1;
#endif


#if defined(DEBUG_CHANGEMENUBAR)
			_wp_GlobalTags[_wp_GlobalTagCounter].ti_Tag = MUIA_IconWindowExt_Menubar_String;
			GET(data->wped_s_menubar, MUIA_String_Contents, &_wp_GlobalTags[_wp_GlobalTagCounter].ti_Data);
			
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'global' MUIA_IconWindowExt_Menubar_String @ Tag %d, data = %d\n", _wp_GlobalTagCounter, _wp_GlobalTags[_wp_GlobalTagCounter].ti_Data));
			
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'global' MUIA_IconWindowExt_Menubar_String in memory: '%s'\n",_wp_GlobalTags[_wp_GlobalTagCounter].ti_Data));
			positionTemp=_wp_GlobalTagCounter;
			_wp_GlobalTagCounter += 1;
			
#endif


#if defined(DEBUG_MULTLINE)
			_wp_GlobalTags[_wp_GlobalTagCounter].ti_Tag = MUIA_IconList_LabelText_MultiLine;
			GET(data->wped_icon_textmultiline, MUIA_Selected, &_wp_GlobalTags[_wp_GlobalTagCounter].ti_Data);
			if ((BOOL)_wp_GlobalTags[_wp_GlobalTagCounter].ti_Data == TRUE)
			{
				GET(data->wped_icon_multilineno, MUIA_String_Integer, &_wp_GlobalTags[_wp_GlobalTagCounter].ti_Data);
				if ((IPTR)_wp_GlobalTags[_wp_GlobalTagCounter].ti_Data < 2)
					_wp_GlobalTags[_wp_GlobalTagCounter].ti_Data = 1;
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'global' MUIA_IconList_LabelText_MultiLine @ Tag %d, data = %d\n", _wp_GlobalTagCounter, _wp_GlobalTags[_wp_GlobalTagCounter].ti_Data));
					_wp_GlobalTagCounter += 1;

				_wp_GlobalTags[_wp_GlobalTagCounter].ti_Tag = MUIA_IconList_LabelText_MultiLineOnFocus;
				GET(data->wped_icon_multilineonfocus, MUIA_Selected, &_wp_GlobalTags[_wp_GlobalTagCounter].ti_Data);

				_wp_GlobalTagCounter += 1;

D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'global' MUIA_IconList_LabelText_MultiLineOnFocus @ Tag %d, data = %d\n", _wp_GlobalTagCounter - 1, _wp_GlobalTags[_wp_GlobalTagCounter - 1].ti_Data));
			}
			else
			{
				_wp_GlobalTags[_wp_GlobalTagCounter].ti_Data = 1;
				
				_wp_GlobalTagCounter += 1;
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'global' MUIA_IconList_LabelText_MultiLine @ Tag %d, data = %d\n", _wp_GlobalTagCounter - 1, _wp_GlobalTags[_wp_GlobalTagCounter - 1].ti_Data));
			}
#endif
			_wp_GlobalTags[_wp_GlobalTagCounter].ti_Tag = TAG_DONE;
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'global' Marked Tag %d as TAG_DONE\n", _wp_GlobalTagCounter));

			ULONG globaldatasize = (_wp_GlobalTagCounter + 1) * sizeof(struct TagItem);

D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: Write 'global' Wanderer Prefs Header Chunk ... \n"));
			if ((error = PushChunk(handle, ID_PREF, ID_WANDR, sizeof(struct WandererPrefsIFFChunkHeader))) == 0)
			{
				sprintf(wanderer_chunkdata.wpIFFch_ChunkType, "%s" , "wanderer:global");
				wanderer_chunkdata.wpIFFch_ChunkSize = globaldatasize;
				
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
				wanderer_chunkdata.wpIFFch_ChunkSize = sizeof(struct TagItem);
				
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
				struct TagItem __wp_networkconfig[2];

				/* save network options*/
				__wp_networkconfig[0].ti_Tag = MUIA_IconWindowExt_NetworkBrowser_Show;
				GET(data->wped_cm_EnableNetworkBrowser, MUIA_Selected, &__wp_networkconfig[0].ti_Data);

				error = WriteChunkBytes(handle, __wp_networkconfig, sizeof(struct TagItem));
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

			struct WPEditor_ViewSettingsObject *_viewSettings_Node = NULL;
			ForeachNode(&_wpeditor_intern_ViewSettings, _viewSettings_Node)
			{
				IPTR   				_viewSettings_ChunkSize = 0, _viewSettings_TagCount = 0;
				struct TagItem 		_viewSettings_TagList[WP_MAX_BG_TAG_COUNT];

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

						const struct TagItem 	*tstate = NULL;
						struct TagItem 			*tag = NULL;

						switch (background_type)
						{
							case 5:
							{
								//Picture type -> store appropriate tags ..
							}
							case 0:
							{
								//Pattern type -> store appropriate tags ..
								tstate = _viewSettings_Node->wpedbo_Options;

								_viewSettings_TagList[_viewSettings_TagCount].ti_Tag   = MUIA_IconWindowExt_ImageBackFill_BGRenderMode;
								_viewSettings_TagList[_viewSettings_TagCount].ti_Data = GetTagData(MUIA_IconWindowExt_ImageBackFill_BGRenderMode, IconWindowExt_ImageBackFill_RenderMode_Tiled, tstate);
								_viewSettings_TagCount += 1;

								while ((tag = NextTagItem(&tstate)) != NULL)
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
					}
					_viewSettings_ChunkSize += (_viewSettings_TagCount * sizeof(struct TagItem));

					wanderer_chunkdata.wpIFFch_ChunkSize = _viewSettings_ChunkSize;

					WriteChunkBytes(handle, &wanderer_chunkdata, sizeof(struct WandererPrefsIFFChunkHeader));

					PopChunk(handle);

D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: Write 'ViewSettings' Wanderer Prefs Data Chunk  for '%s' ... \n", _viewSettings_Node->wpedbo_ViewName));

					if ((error = PushChunk(handle, ID_PREF, ID_WANDR, _viewSettings_ChunkSize)) == 0)
					{
						UBYTE *_viewSettings_ChunkData = AllocMem(_viewSettings_ChunkSize, MEMF_ANY|MEMF_CLEAR);
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'ViewSettings' Chunk Data storage @ %x, %d bytes\n", _viewSettings_ChunkData, _viewSettings_ChunkSize));

						sprintf(_viewSettings_ChunkData, "%s", background_value);
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'ViewSettings' MUIA_Background = '%s'\n", _viewSettings_ChunkData));
						if ((_viewSettings_Node->wpedbo_AdvancedOptionsObject) && ((_viewSettings_Node->wpedbo_Options)&&(_viewSettings_TagCount > 0)))
						{
							struct TagItem 	*dest_tag =(struct TagItem *) (_viewSettings_ChunkData + _viewSettings_TagOffset);
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ExportFH: 'ViewSettings' Writing data for %d Tags @ %x\n", _viewSettings_TagCount, dest_tag));
							do
							{
								dest_tag[_viewSettings_TagCount - 1].ti_Tag = _viewSettings_TagList[_viewSettings_TagCount - 1].ti_Tag;
								dest_tag[_viewSettings_TagCount - 1].ti_Data = _viewSettings_TagList[_viewSettings_TagCount - 1].ti_Data;
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


IPTR WPEditor__MUIM_Setup
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_WPEDITOR_INST_DATA;
	
D(bug("[WPEditor] WPEditor__MUIM_Setup()\n"));

    if (!DoSuperMethodA(CLASS, self, message)) return FALSE;
	
#if defined(DEBUG_ADVANCEDIMAGEOPTIONS)
		DoMethod(
			 _app(self), 
			 OM_ADDMEMBER, 
			 data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_WindowObj
			);
#endif
	DoMethod (
			data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_RenderModeObj,
        		MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
			(IPTR)self, 3, 
				MUIM_CallHook,
        			&data->wped_AdvancedViewSettings_WindowData->wpedabwd_Hook_DrawModeChage, 
				CLASS
		  );

	DoMethod (
			data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_UseObj,
			MUIM_Notify, MUIA_Pressed, FALSE,
			(IPTR)self, 3, 
				MUIM_CallHook,
        			&data->wped_Hook_CloseAdvancedOptions, 
				TRUE
		 );

	DoMethod (
			data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_CancelObj,
			MUIM_Notify, MUIA_Pressed, FALSE,
			(IPTR)self, 3, 
				MUIM_CallHook,
        			&data->wped_Hook_CloseAdvancedOptions, 
				FALSE
		 );

	DoMethod (
			data->wped_AdvancedViewSettings_WindowData->wpedabwd_Window_WindowObj,
			MUIM_Notify, MUIA_Window_CloseRequest, TRUE, 
			(IPTR)self, 3, 
				MUIM_CallHook,
        			&data->wped_Hook_CloseAdvancedOptions, 
				FALSE
		 );

	return TRUE;
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
		
D(bug("[WPEditor] WPEditor__MUIM_Show: WindowObj @ %x, Real Window @ %x, %d, %d [%d x %d]\n", 
			this_Win,
			thisWin_Window,
			thisWin_X, thisWin_Y,
			thisWin_Width, thisWin_Height));

D(bug("[WPEditor] WPEditor__MUIM_Show: ViewSettings Group height = %d\n", viewsettingsbox_Height));

		if ((thisWin_Window) && (viewsettingsbox_Height > 0))
		{
			LONG 	NEWHEIGHT = 0,
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

					ChangeWindowBox(thisWin_Window,	thisWin_X, thisWin_Y, thisWin_Width, thisWin_Height);
					data->wped_DimensionsSet = TRUE;
				}
			}
		}
	}
#endif
	return TRUE;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_5
(
    WPEditor, NULL, MUIC_PrefsEditor, NULL,
    OM_NEW,                                struct opSet *,
    MUIM_Setup,                            Msg,
    MUIM_Show,                             Msg,
    MUIM_PrefsEditor_ImportFH,             struct MUIP_PrefsEditor_ImportFH *,
    MUIM_PrefsEditor_ExportFH,             struct MUIP_PrefsEditor_ExportFH *
);
