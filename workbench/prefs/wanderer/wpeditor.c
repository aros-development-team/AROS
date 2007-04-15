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
//#define     WP_DISABLE_ADVANCEDIMAGEOPTIONS
//#define       DEBUG_NETWORKBROWSER
//#define       DEBUG_SHOWUSERFILES

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

/*** Identifier Base ********************************************************/
#define MUIB_WandererPrefs                      (TAG_USER | 0x12000000)

/*** Private Methods ********************************************************/
#define MUIA_WPrefsEditor_ActiveBGround         (MUIB_WandererPrefs | 0x00000101)

/*** Instance Data **********************************************************/
struct WPEditor_BackgroundObject
{
	struct Node                                     wpedbo_Node;
	char                                            *wpedbo_BackgroundName;
	Object                                          *wpedbo_ImageSpecObject;
	Object                                          *wpedbo_AdvancedOptionsObject;
	IPTR                                            *wpedbo_Type;
    struct TagItem                                  *wpedbo_Options;
    struct Hook                                     wpedbo_Hook_CheckImage;
    struct Hook                                     wpedbo_Hook_OpenAdvancedOptions;
	IPTR                                            wpedbo_state_AdvancedDisabled;
};

struct WPEditor_AdvancedBackgroundWindow_DATA
{
	Object                                          *wpedabwd_Window_WindowObj,
									                *wpedabwd_Window_UseObj,
									                *wpedabwd_Window_CancelObj,
									                *wpedabwd_Window_RenderModeGrpObj,
									                *wpedabwd_Window_RenderModeObj,
									                *wpedabwd_Window_PageObj,
									                *wpedabwd_Window_TileModeObj,
									                *wpedabwd_Window_XOffsetObj,
									                *wpedabwd_Window_YOffsetObj;

	STRPTR                                          *wpedabwd_RenderModeObj_RenderModes;
	IPTR                                            *wpedabwd_RenderModeObj_RenderPages;
    struct Hook                                     wpedabwd_Hook_DrawModeChage;
};

struct WPEditor_DATA
{
	struct WPEditor_AdvancedBackgroundWindow_DATA   *wped_AdvancedBG_WindowData;
	struct WPEditor_BackgroundObject                *wped_Background_Current;
	Object                                          *wped_Background_GroupObj,
									                *wped_Background_SpacerObj,
                                                    *wped_c_NavigationMethod,
                                                    *wped_cm_ToolbarEnabled, 
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
                                                    *wped_toolbarGroup;
    struct Hook                                     wped_Hook_EnhancedNav,
		                                            wped_Hook_CloseAdvancedOptions;
};

//static struct Hook navichangehook;
static STRPTR   navigationtypelabels[3];
static STRPTR   iconlistmodes[3];
static STRPTR   icontextmodes[3];
static STRPTR   registerpages[4];

static STRPTR   _wpeditor_intern_Base_BackgroundImage_RenderModeNames[WP_MAX_RENDER_MODES];
static IPTR     _wpeditor_intern_Base_BackgroundImage_RenderModeValues[WP_MAX_RENDER_MODES];
static STRPTR   _wpeditor_intern_Base_BackgroundImage_TileModeNames[WP_MAX_TILE_MODES];
static IPTR     _wpeditor_intern_Base_BackgroundImage_TileModeValues[WP_MAX_TILE_MODES];

static Class         *_wpeditor_intern_CLASS = NULL;
static struct List   _wpeditor_intern_Backgrounds;

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct WPEditor_DATA *data = INST_DATA(CLASS, self)

/*** Hook functions *********************************************************/

IPTR GetRenderModeTag(char *rendermode_name)
{
	int i = 0;
	
	for (i = 0; i < WP_MAX_RENDER_MODES; i++)
	{
		if (strcmp(_wpeditor_intern_Base_BackgroundImage_RenderModeNames[i], rendermode_name) == 0)
		{
			return _wpeditor_intern_Base_BackgroundImage_RenderModeValues[i];
		}
	}
	return -1;
}

IPTR GetRenderModeName(IPTR rendermode_val)
{
	int i = 0;
	
	for (i = 0; i < WP_MAX_RENDER_MODES; i++)
	{
		if (_wpeditor_intern_Base_BackgroundImage_RenderModeValues[i] == rendermode_val)
		{
			return _wpeditor_intern_Base_BackgroundImage_RenderModeNames[i];
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

IPTR SetBackgroundTag(struct TagItem * this_Taglist, IPTR Tag_ID, IPTR newTag_Value)
{
	int i = 0;
	do
	{
		if (this_Taglist[i].ti_Tag == Tag_ID)
		{
			if (this_Taglist[i].ti_Data == newTag_Value) return -1;
			this_Taglist[i].ti_Data = newTag_Value;
			return TRUE;
		}
		i ++;
	}while( i < WP_MAX_BG_TAG_COUNT);

	return FALSE;
}

AROS_UFH3(
    void, WandererPrefs_Hook_EnhancedNavFunc,
    AROS_UFHA(struct Hook *,    hook,   A0),
    AROS_UFHA(APTR *,           obj,    A2),
    AROS_UFHA(APTR,             param,  A1)
)
{
    AROS_USERFUNC_INIT
    
    Object *self = ( Object *)obj;
    Class *CLASS = *( Class **)param;

    SETUP_INST_DATA;
    
    SET( 
        data->wped_cm_ToolbarEnabled, MUIA_Selected,
        XGET( data->wped_c_NavigationMethod, MUIA_Cycle_Active )
    );
    
    AROS_USERFUNC_EXIT
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

    Object                           *self            = ( Object *)obj;
	struct WPEditor_BackgroundObject *this_Background = *( struct WPEditor_BackgroundObject **)param;
    Class                            *CLASS           = _wpeditor_intern_CLASS;

    SETUP_INST_DATA;

	data->wped_Background_Current = this_Background;

	UBYTE                           *ImageSelector_Spec = NULL;
	char 							*Image_Spec = NULL;

	GET(this_Background->wpedbo_ImageSpecObject, MUIA_Imagedisplay_Spec, &ImageSelector_Spec);

	if (Image_Spec = AllocVec(strlen(ImageSelector_Spec) + 1, MEMF_CLEAR|MEMF_PUBLIC))
	{
		strcpy(Image_Spec, ImageSelector_Spec);
		SET(this_Background->wpedbo_ImageSpecObject, MUIA_Imagedisplay_Spec, Image_Spec);

		struct WPEditor_BackgroundObject *background_Node = NULL;

		ForeachNode(&_wpeditor_intern_Backgrounds, background_Node)
		{
			GET(background_Node->wpedbo_AdvancedOptionsObject, MUIA_Disabled, &background_Node->wpedbo_state_AdvancedDisabled);
		}
		
		SET(data->wped_Background_GroupObj, MUIA_Disabled, TRUE);
		
		SET(data->wped_AdvancedBG_WindowData->wpedabwd_Window_WindowObj, MUIA_Window_Open, TRUE);
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
	struct WPEditor_BackgroundObject *background_Node = NULL;
    Class                            *CLASS  = _wpeditor_intern_CLASS;
    BOOL                             settings_changed = FALSE;
    BOOL                             success = FALSE;

    SETUP_INST_DATA;

D(bug("[WPEditor] WandererPrefs_Hook_CloseAdvancedOptionsFunc()\n"));
	
	SET(data->wped_Background_GroupObj, MUIA_Disabled, FALSE);

	ForeachNode(&_wpeditor_intern_Backgrounds, background_Node)
	{
		SET(background_Node->wpedbo_AdvancedOptionsObject, MUIA_Disabled, background_Node->wpedbo_state_AdvancedDisabled);
	}

	if ((use_settings) && (data->wped_Background_Current))
	{
D(bug("[WPEditor] WandererPrefs_Hook_CloseAdvancedOptionsFunc: Updating tags for Background ..\n"));
		IPTR current_rendermode = 0;
		GET(data->wped_AdvancedBG_WindowData->wpedabwd_Window_RenderModeObj, MUIA_Cycle_Active, &current_rendermode);
		char *current_rendermode_name = data->wped_AdvancedBG_WindowData->wpedabwd_RenderModeObj_RenderModes[current_rendermode];

D(bug("[WPEditor] WandererPrefs_Hook_CloseAdvancedOptionsFunc: Render Mode '%s'\n", current_rendermode_name));
		current_rendermode = GetRenderModeTag(current_rendermode_name);
		if (current_rendermode == -1)
		{
D(bug("[WPEditor] WandererPrefs_Hook_CloseAdvancedOptionsFunc: ERROR: Unknown Render mode string!?!?!\n"));
		}
		else
		{
			success = SetBackgroundTag(data->wped_Background_Current->wpedbo_Options, MUIA_WandererPrefs_Background_RenderMode, current_rendermode);
			if (success == FALSE)
			{
D(bug("[WPEditor] WandererPrefs_Hook_CloseAdvancedOptionsFunc: No MUIA_WandererPrefs_Background_RenderMode TAG - Adding ..\n"));
#warning "TODO: Allocate extra storage for our tags.."
			}
			else if (success == TRUE)
			{
				settings_changed = TRUE;
			}

			switch (current_rendermode)
			{
				case WPD_BackgroundRenderMode_Scale:
					break;

				case WPD_BackgroundRenderMode_Tiled:
					success = SetBackgroundTag(data->wped_Background_Current->wpedbo_Options, MUIA_WandererPrefs_Background_XOffset, XGET(data->wped_AdvancedBG_WindowData->wpedabwd_Window_XOffsetObj, MUIA_String_Integer));
					if (success == FALSE)
					{
D(bug("[WPEditor] WandererPrefs_Hook_CloseAdvancedOptionsFunc: No MUIA_WandererPrefs_Background_XOffset TAG - Adding ..\n"));
#warning "TODO: Allocate extra storage for our tags.."
					}
					else if (success == TRUE)
					{
						settings_changed = TRUE;
					}
					
					success = SetBackgroundTag(data->wped_Background_Current->wpedbo_Options, MUIA_WandererPrefs_Background_YOffset, XGET(data->wped_AdvancedBG_WindowData->wpedabwd_Window_YOffsetObj, MUIA_String_Integer));
					if (success == FALSE)
					{
D(bug("[WPEditor] WandererPrefs_Hook_CloseAdvancedOptionsFunc: No MUIA_WandererPrefs_Background_YOffset TAG - Adding ..\n"));
#warning "TODO: Allocate extra storage for our tags.."
					}
					else if (success == TRUE)
					{
						settings_changed = TRUE;
					}

					IPTR current_tilemode = _wpeditor_intern_Base_BackgroundImage_TileModeValues[XGET(data->wped_AdvancedBG_WindowData->wpedabwd_Window_TileModeObj, MUIA_Cycle_Active)];
					success = SetBackgroundTag(data->wped_Background_Current->wpedbo_Options, MUIA_WandererPrefs_Background_TileMode, current_tilemode);
					if (success == FALSE)
					{
D(bug("[WPEditor] WandererPrefs_Hook_CloseAdvancedOptionsFunc: No MUIA_WandererPrefs_Background_TileMode TAG - Adding ..\n"));
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

	SET(data->wped_AdvancedBG_WindowData->wpedabwd_Window_WindowObj, MUIA_Window_Open, FALSE);
	
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

    SETUP_INST_DATA;

D(bug("[WPEditor] WandererPrefs_Hook_DrawModeChangeFunc()\n"));
	
	if (data->wped_Background_Current)
	{
		IPTR drawmode_no = 0;

		GET(data->wped_AdvancedBG_WindowData->wpedabwd_Window_RenderModeObj, MUIA_Cycle_Active, &drawmode_no);

D(bug("[WPEditor] WandererPrefs_Hook_DrawModeChangeFunc: Active DrawMode = %d, Page = %d\n", drawmode_no, data->wped_AdvancedBG_WindowData->wpedabwd_RenderModeObj_RenderPages[drawmode_no]));

		SET((IPTR)data->wped_AdvancedBG_WindowData->wpedabwd_Window_PageObj,
				MUIA_Group_ActivePage, data->wped_AdvancedBG_WindowData->wpedabwd_RenderModeObj_RenderPages[drawmode_no]);
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

    Object                           *self            = ( Object *)obj;
	struct WPEditor_BackgroundObject *this_Background = *( struct WPEditor_BackgroundObject **)param;
    Class                            *CLASS           = _wpeditor_intern_CLASS;

    SETUP_INST_DATA;
	
	UBYTE                           *ImageSelector_Spec = NULL;
	
	GET(this_Background->wpedbo_ImageSpecObject, MUIA_Imagedisplay_Spec, &ImageSelector_Spec);
	
D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: Object @ %x reports image spec '%s'\n", this_Background->wpedbo_ImageSpecObject, (char *)ImageSelector_Spec));

	IPTR                             this_Background_type = (IPTR)(ImageSelector_Spec[0] - 48);
	
	this_Background->wpedbo_Type = this_Background_type;
	data->wped_Background_Current = this_Background;

	if ((this_Background->wpedbo_Type == 5)||(this_Background->wpedbo_Type == 0))
	{
D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: Image-type spec (%d) - Enabling Advanced options ..\n", this_Background->wpedbo_Type));
#if !defined(WP_DISABLE_ADVANCEDIMAGEOPTIONS)
		SET(this_Background->wpedbo_AdvancedOptionsObject, MUIA_Disabled, FALSE);
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

		if (this_Background->wpedbo_Options)
		{
D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: Existing options @ %x\n", this_Background->wpedbo_Options));
		}

		switch ((int)this_Background->wpedbo_Type)
		{
			case 5:
			{
				if ((strcmp(this_Background->wpedbo_BackgroundName, "Workbench")) == 0)
				{
					newBG_RenderModes[newBG_DrawModeCount] = _wpeditor_intern_Base_BackgroundImage_RenderModeNames[WPD_BackgroundRenderMode_Scale - 1];
					newBG_DrawModePages[newBG_DrawModeCount++] = 0;

D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: DrawMode %d = '%s'\n", newBG_DrawModeCount -1, newBG_RenderModes[newBG_DrawModeCount-1]));
				}
			}
			case 0:
			{
				newBG_RenderModes[newBG_DrawModeCount]	= _wpeditor_intern_Base_BackgroundImage_RenderModeNames[WPD_BackgroundRenderMode_Tiled - 1];
				newBG_DrawModePages[newBG_DrawModeCount++] = 1;

				newBG_Options[newBG_OptionCount].ti_Tag = MUIA_WandererPrefs_Background_RenderMode;
				if (this_Background->wpedbo_Options)
					newBG_Options[newBG_OptionCount++].ti_Data = GetTagData(MUIA_WandererPrefs_Background_RenderMode, WPD_BackgroundRenderMode_Tiled, this_Background->wpedbo_Options);
				else
				 newBG_Options[newBG_OptionCount++].ti_Data = WPD_BackgroundRenderMode_Tiled;

				newBG_Options[newBG_OptionCount].ti_Tag = MUIA_WandererPrefs_Background_TileMode;
				if (this_Background->wpedbo_Options)
					newBG_Options[newBG_OptionCount++].ti_Data = GetTagData(MUIA_WandererPrefs_Background_TileMode, WPD_BackgroundTileMode_Float, this_Background->wpedbo_Options);
				else
					newBG_Options[newBG_OptionCount++].ti_Data = WPD_BackgroundTileMode_Float;
				
				newBG_Options[newBG_OptionCount].ti_Tag = MUIA_WandererPrefs_Background_XOffset;
				if (this_Background->wpedbo_Options)
					newBG_Options[newBG_OptionCount++].ti_Data = GetTagData(MUIA_WandererPrefs_Background_XOffset, 0, this_Background->wpedbo_Options);
				else
					newBG_Options[newBG_OptionCount++].ti_Data = 0;

				newBG_Options[newBG_OptionCount].ti_Tag = MUIA_WandererPrefs_Background_YOffset;
				if (this_Background->wpedbo_Options)
					newBG_Options[newBG_OptionCount++].ti_Data = GetTagData(MUIA_WandererPrefs_Background_YOffset, 0, this_Background->wpedbo_Options);
				else
					newBG_Options[newBG_OptionCount++].ti_Data = 0;

D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: DrawMode %d = '%s'\n", newBG_DrawModeCount -1, newBG_RenderModes[newBG_DrawModeCount-1]));

				break;
			}
		}

		if (newBG_OptionCount > 0)
		{
			IPTR old_bg_options = this_Background->wpedbo_Options;

			this_Background->wpedbo_Options = NULL;

			this_Background->wpedbo_Options = AllocVec((sizeof(struct TagItem) * newBG_OptionCount + 1), MEMF_CLEAR|MEMF_PUBLIC);
			do
			{
				this_Background->wpedbo_Options[newBG_OptionCount - 1].ti_Tag = newBG_Options[newBG_OptionCount - 1].ti_Tag;
				this_Background->wpedbo_Options[newBG_OptionCount - 1].ti_Data = newBG_Options[newBG_OptionCount - 1].ti_Data;
				newBG_OptionCount --;
			}while(newBG_OptionCount > 0);

			SET(data->wped_AdvancedBG_WindowData->wpedabwd_Window_TileModeObj, MUIA_Cycle_Active, GetTagData(MUIA_WandererPrefs_Background_TileMode, WPD_BackgroundTileMode_Float, this_Background->wpedbo_Options) - 1);
			SET(data->wped_AdvancedBG_WindowData->wpedabwd_Window_XOffsetObj, MUIA_String_Integer, GetTagData(MUIA_WandererPrefs_Background_XOffset, 0, this_Background->wpedbo_Options));
			SET(data->wped_AdvancedBG_WindowData->wpedabwd_Window_YOffsetObj, MUIA_String_Integer, GetTagData(MUIA_WandererPrefs_Background_YOffset, 0, this_Background->wpedbo_Options));

			if (old_bg_options)
				FreeVec(old_bg_options);
		}

		if (newBG_DrawModeCount > 0)
		{
			IPTR setpage_active = newBG_DrawModeCount - 1;

			IPTR old_bg_drawmodes = data->wped_AdvancedBG_WindowData->wpedabwd_RenderModeObj_RenderModes;
			IPTR old_bg_drawpages = data->wped_AdvancedBG_WindowData->wpedabwd_RenderModeObj_RenderPages;

D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: Old RenderModes @ %x, pages @ %x\n", old_bg_drawmodes, old_bg_drawpages));

			data->wped_AdvancedBG_WindowData->wpedabwd_RenderModeObj_RenderModes = NULL;
			data->wped_AdvancedBG_WindowData->wpedabwd_RenderModeObj_RenderPages = NULL;

			data->wped_AdvancedBG_WindowData->wpedabwd_RenderModeObj_RenderModes = AllocVec((sizeof(STRPTR) * (newBG_DrawModeCount + 1)), MEMF_CLEAR|MEMF_PUBLIC);
			data->wped_AdvancedBG_WindowData->wpedabwd_RenderModeObj_RenderPages = AllocVec((sizeof(IPTR) * newBG_DrawModeCount), MEMF_CLEAR|MEMF_PUBLIC);

D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: Allocated new RenderModes Array @ %x, page mappings @ %x\n", data->wped_AdvancedBG_WindowData->wpedabwd_RenderModeObj_RenderModes, data->wped_AdvancedBG_WindowData->wpedabwd_RenderModeObj_RenderPages));

			for (i = 0; i < newBG_DrawModeCount; i ++)
			{
				data->wped_AdvancedBG_WindowData->wpedabwd_RenderModeObj_RenderModes[i] = newBG_RenderModes[i];
				data->wped_AdvancedBG_WindowData->wpedabwd_RenderModeObj_RenderPages[i] = newBG_DrawModePages[i];
			}

			if ((this_Background->wpedbo_Options) && (newBG_DrawModeCount > 1))
			{
				setpage_active = GetRenderModePage(
				                                    GetRenderModeName(GetTagData(MUIA_WandererPrefs_Background_RenderMode, WPD_BackgroundRenderMode_Tiled, this_Background->wpedbo_Options)),
                                     				data->wped_AdvancedBG_WindowData->wpedabwd_RenderModeObj_RenderModes,
												    data->wped_AdvancedBG_WindowData->wpedabwd_RenderModeObj_RenderPages);
			}

D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: Setting advanced page to  %d = '%s'\n", setpage_active, data->wped_AdvancedBG_WindowData->wpedabwd_RenderModeObj_RenderModes[i]));

			Object *new_RenderModeObj = NULL;

D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: Creating RenderModes Cycle gadget\n"));

			if (new_RenderModeObj = MUI_MakeObject(MUIO_Cycle, NULL, data->wped_AdvancedBG_WindowData->wpedabwd_RenderModeObj_RenderModes))
			{
D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: Object @ %x\n", new_RenderModeObj));
				if (DoMethod(data->wped_AdvancedBG_WindowData->wpedabwd_Window_RenderModeGrpObj, MUIM_Group_InitChange))
				{
					DoMethod(data->wped_AdvancedBG_WindowData->wpedabwd_Window_RenderModeGrpObj, OM_REMMEMBER, data->wped_AdvancedBG_WindowData->wpedabwd_Window_RenderModeObj);
					DoMethod(data->wped_AdvancedBG_WindowData->wpedabwd_Window_RenderModeGrpObj, OM_ADDMEMBER, new_RenderModeObj);
					DoMethod(data->wped_AdvancedBG_WindowData->wpedabwd_Window_RenderModeGrpObj, MUIM_Group_ExitChange);
					
					data->wped_AdvancedBG_WindowData->wpedabwd_Window_RenderModeObj = new_RenderModeObj;
					
					DoMethod (
						data->wped_AdvancedBG_WindowData->wpedabwd_Window_RenderModeObj,
						MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
						(IPTR)self, 3, MUIM_CallHook,
						&data->wped_AdvancedBG_WindowData->wpedabwd_Hook_DrawModeChage, CLASS
					);
				}

D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: Setting cycle active \n"));

				SET(data->wped_AdvancedBG_WindowData->wpedabwd_Window_RenderModeObj, MUIA_Cycle_Active, setpage_active);
			}
			if (old_bg_drawmodes)
				FreeVec(old_bg_drawmodes);
		}
    }
	else
	{
D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: Not an Image-type spec - Disabling Advanced options ..\n"));
		SET(this_Background->wpedbo_AdvancedOptionsObject, MUIA_Disabled, TRUE);
	}
	SET(self, MUIA_PrefsEditor_Changed, TRUE);

    AROS_USERFUNC_EXIT
}

struct WPEditor_BackgroundObject *WPEditor__FindBackgroundObjects(char * this_Name)
{
	struct WPEditor_BackgroundObject *this_Background = NULL;

	ForeachNode(&_wpeditor_intern_Backgrounds, this_Background)
	{
		if ((strcmp(this_Background->wpedbo_BackgroundName, this_Name)) == 0) return this_Background;
	}
}

struct WPEditor_BackgroundObject *WPEditor__NewBackgroundObjects(char * this_Name)
{
	struct WPEditor_BackgroundObject *this_Background = NULL;

D(bug("[WPEditor] WPEditor__NewBackgroundObjects('%s')\n", this_Name));
	
	if (this_Background = AllocMem(sizeof(struct WPEditor_BackgroundObject), MEMF_CLEAR|MEMF_PUBLIC))
	{
		if (this_Background->wpedbo_BackgroundName = AllocVec(strlen(this_Name) + 1, MEMF_CLEAR | MEMF_PUBLIC))
		{
			strcpy(this_Background->wpedbo_BackgroundName, this_Name);
			
			if (this_Background->wpedbo_ImageSpecObject = PopimageObject,
										MUIA_FixWidth, 50,
										MUIA_FixHeight, 50,
										MUIA_Window_Title, __(MSG_SELECT_WORKBENCH_BACKGROUND),
										MUIA_Imageadjust_Type, MUIV_Imageadjust_Type_Background,
										MUIA_CycleChain,       1,
									End)
			{
				this_Background->wpedbo_AdvancedOptionsObject = SimpleButton("Advanced");

				this_Background->wpedbo_Hook_CheckImage.h_Entry = ( HOOKFUNC )WandererPrefs_Hook_CheckImageFunc;
				AddTail(&_wpeditor_intern_Backgrounds, &this_Background->wpedbo_Node);
D(bug("[WPEditor] WPEditor__NewBackgroundObjects: Successfully created\n"));
				goto nbo_Done;
			}
			FreeVec(this_Background->wpedbo_BackgroundName);
		}

		FreeMem(this_Background, sizeof(struct WPEditor_BackgroundObject));
		this_Background = NULL;
	}
D(bug("[WPEditor] WPEditor__NewBackgroundObjects: Failed to create objects\n"));
	
nbo_Done:
	return this_Background;
}

/*** Methods ****************************************************************/
Object *WPEditor__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    struct WPEditor_DATA *data = NULL;
    
    Object 	                                       *_WP_Background_GroupObj = NULL,
			                                       *_WP_Background_SpacerObj = NULL,
			                                       *c_navitype = NULL, 
			                                       *bt_dirup = NULL,
                                                   *bt_search = NULL,
                                                   *cm_toolbarenabled = NULL, 
#if defined(DEBUG_NETWORKBROWSER)
												   *cm_EnableNetworkBrowser, 
#endif
#if defined(DEBUG_SHOWUSERFILES)
												   *cm_EnableUserFiles, 
#endif
			                                       *toolbarpreview = NULL,
			                                       *wped_icon_listmode = NULL,
                                                   *wped_icon_textmode = NULL, 
			                                       *wped_icon_textmaxlen = NULL,
			                                       *toolbarGroup = NULL,
			                                       *prefs_pages = NULL;

	struct WPEditor_AdvancedBackgroundWindow_DATA  *_WP_AdvancedBackgroundOptions_WindowData = NULL;
D(bug("[WPEditor] WPEditor__OM_NEW()\n"));

	NewList(&_wpeditor_intern_Backgrounds);
	
	struct WPEditor_BackgroundObject               *background_Workbench = NULL;
	struct WPEditor_BackgroundObject               *background_Drawer    = NULL;

	background_Workbench = WPEditor__NewBackgroundObjects("Workbench");
	background_Drawer = WPEditor__NewBackgroundObjects("Drawer");

    //Object *cm_searchenabled;

    registerpages[WPD_GENERAL] = (STRPTR)_(MSG_GENERAL);
    registerpages[WPD_APPEARANCE] = (STRPTR)_(MSG_APPEARANCE);
    registerpages[WPD_TOOLBAR] = (STRPTR)_(MSG_TOOLBAR);

    iconlistmodes[WPD_ICONLISTMODE_GRID] = (STRPTR)_(MSG_ICONLISTMODE_GRID);
    iconlistmodes[WPD_ICONLISTMODE_PLAIN] = (STRPTR)_(MSG_ICONLISTMODE_PLAIN);

    icontextmodes[WPD_ICONTEXTMODE_OUTLINE] = (STRPTR)_(MSG_ICONTEXTMODE_OUTLINE);
    icontextmodes[WPD_ICONTEXTMODE_PLAIN] = (STRPTR)_(MSG_ICONTEXTMODE_PLAIN);

    navigationtypelabels[WPD_NAVIGATION_CLASSIC] = (STRPTR)_(MSG_CLASSIC);
    navigationtypelabels[WPD_NAVIGATION_ENHANCED] = (STRPTR)_(MSG_ENHANCED);

	_wpeditor_intern_Base_BackgroundImage_RenderModeNames[WPD_BackgroundRenderMode_Tiled - 1] = "Tiled";
	_wpeditor_intern_Base_BackgroundImage_RenderModeValues[WPD_BackgroundRenderMode_Tiled - 1] = WPD_BackgroundRenderMode_Tiled;
	_wpeditor_intern_Base_BackgroundImage_RenderModeNames[WPD_BackgroundRenderMode_Scale - 1] = "Scaled";
	_wpeditor_intern_Base_BackgroundImage_RenderModeValues[WPD_BackgroundRenderMode_Scale - 1] = WPD_BackgroundRenderMode_Scale;

	_wpeditor_intern_Base_BackgroundImage_TileModeNames[WPD_BackgroundTileMode_Float - 1] = "Floating";
	_wpeditor_intern_Base_BackgroundImage_TileModeValues[WPD_BackgroundTileMode_Float - 1] = WPD_BackgroundTileMode_Float;
	_wpeditor_intern_Base_BackgroundImage_TileModeNames[WPD_BackgroundTileMode_Fixed - 1] = "Fixed";
	_wpeditor_intern_Base_BackgroundImage_TileModeValues[WPD_BackgroundTileMode_Fixed - 1] = WPD_BackgroundTileMode_Fixed;

    c_navitype = MUI_MakeObject(MUIO_Cycle, NULL, navigationtypelabels);

	wped_icon_listmode = MUI_MakeObject(MUIO_Cycle, NULL, iconlistmodes);
    wped_icon_textmode = MUI_MakeObject(MUIO_Cycle, NULL, icontextmodes);
    cm_toolbarenabled = MUI_MakeObject(MUIO_Checkmark,NULL);
    wped_icon_textmaxlen = MUI_MakeObject(MUIO_String,NULL,4);

	_WP_AdvancedBackgroundOptions_WindowData = AllocMem(sizeof(struct WPEditor_AdvancedBackgroundWindow_DATA), MEMF_CLEAR|MEMF_PUBLIC);

	_WP_AdvancedBackgroundOptions_WindowData->wpedabwd_Hook_DrawModeChage.h_Entry = ( HOOKFUNC )WandererPrefs_Hook_DrawModeChangeFunc;
	
	_WP_AdvancedBackgroundOptions_WindowData->wpedabwd_Window_WindowObj = WindowObject,
	                                    MUIA_Window_CloseGadget, FALSE,
										MUIA_Window_Title, (IPTR)"Advanced Options ..",
										WindowContents, (IPTR)VGroup,
											Child, (IPTR)(_WP_AdvancedBackgroundOptions_WindowData->wpedabwd_Window_RenderModeGrpObj = HGroup,
												Child, (IPTR) Label1("Draw Mode : "),
												Child, (IPTR)(_WP_AdvancedBackgroundOptions_WindowData->wpedabwd_Window_RenderModeObj = MUI_MakeObject(MUIO_Cycle, NULL, _wpeditor_intern_Base_BackgroundImage_RenderModeNames)),
											End),
											Child, (IPTR)(_WP_AdvancedBackgroundOptions_WindowData->wpedabwd_Window_PageObj = GroupObject,
												MUIA_Group_PageMode, TRUE,
												Child, (IPTR) GroupObject,
													Child, GroupObject,
														MUIA_Group_SameSize, FALSE,
														MUIA_FrameTitle, "Scale Mode Options ..",
														MUIA_Frame, MUIV_Frame_Group,
															Child, HVSpace,
													End,
												End,
												Child, (IPTR) GroupObject,
													Child, GroupObject,
														MUIA_Group_SameSize, FALSE,
														MUIA_FrameTitle, "Tile Mode Options ..",
														MUIA_Frame, MUIV_Frame_Group,
														MUIA_Group_Columns, 2,
	
														Child, (IPTR) Label1("Tile Mode : "),
														Child, (IPTR)(_WP_AdvancedBackgroundOptions_WindowData->wpedabwd_Window_TileModeObj = MUI_MakeObject(MUIO_Cycle, NULL, _wpeditor_intern_Base_BackgroundImage_TileModeNames)),
	
														Child, (IPTR) Label1("X Offset : "),
														Child, (_WP_AdvancedBackgroundOptions_WindowData->wpedabwd_Window_XOffsetObj = StringObject,
															MUIA_String_MaxLen, 3,
															MUIA_String_Accept, "0123456789",
														End),

														Child, (IPTR) Label1("Y Offset : "),
														Child, (_WP_AdvancedBackgroundOptions_WindowData->wpedabwd_Window_YOffsetObj = StringObject,
															MUIA_String_MaxLen, 3,
															MUIA_String_Accept, "0123456789",
														End),
													End,
												End,
											End),
											Child, (IPTR)HGroup,
												Child, (IPTR) (_WP_AdvancedBackgroundOptions_WindowData->wpedabwd_Window_UseObj = ImageButton("Use", "THEME:Images/Gadgets/Prefs/Use")),
												Child, (IPTR) (_WP_AdvancedBackgroundOptions_WindowData->wpedabwd_Window_CancelObj = ImageButton("Cancel", "THEME:Images/Gadgets/Prefs/Cancel")),
											End,
										End,
									 End;

D(bug("[WPEditor] WPEditor__OM_NEW: 'Advanced' Window Object @ %x\n", _WP_AdvancedBackgroundOptions_WindowData->wpedabwd_Window_WindowObj));

    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
            
            MUIA_PrefsEditor_Name, __(MSG_NAME),
            MUIA_PrefsEditor_Path, (IPTR) "SYS/Wanderer.prefs",
            
            Child, (IPTR) (prefs_pages = RegisterObject,
                MUIA_Register_Titles, (IPTR) registerpages,
                Child, (IPTR) GroupObject,
                    Child, (IPTR) HGroup,                    // general 
                        MUIA_FrameTitle, __(MSG_NAVIGATION),
                        MUIA_Group_SameSize, TRUE,
                        MUIA_Frame, MUIV_Frame_Group,
						MUIA_Group_Columns, 2,
						Child, (IPTR) HGroup,
							MUIA_Group_Columns, 2,
							MUIA_Group_SameSize, FALSE,
                            Child, (IPTR) Label1(_(MSG_METHOD)),
                            Child, (IPTR) c_navitype,
							Child, (IPTR) HVSpace,
							Child, (IPTR) HVSpace,
						End,
						Child, (IPTR) HGroup,
							MUIA_Group_Columns, 2,
							MUIA_Group_SameSize, FALSE,
#if defined(DEBUG_NETWORKBROWSER)
							Child, (IPTR) Label1("Network Browser on Workbench"),
							Child, (IPTR) (cm_EnableNetworkBrowser = MUI_MakeObject(MUIO_Checkmark,NULL)),
#else
							Child, (IPTR) HVSpace,
							Child, (IPTR) HVSpace,
#endif
#if defined(DEBUG_SHOWUSERFILES)
							Child, (IPTR) Label1("User Files Folder on Workbench"),
							Child, (IPTR) (cm_EnableUserFiles = MUI_MakeObject(MUIO_Checkmark,NULL)),
#else
							Child, (IPTR) HVSpace,
							Child, (IPTR) HVSpace,
#endif
							Child, (IPTR) HVSpace,
							Child, (IPTR) HVSpace,
						End,
                    End,
                End,
                Child, (IPTR) (GroupObject,                     // appearance 
                    MUIA_Group_SameSize, FALSE,
                    MUIA_Group_Horiz, TRUE,
                    
                    Child, (IPTR) (_WP_Background_GroupObj = GroupObject,
						MUIA_Group_SameSize, FALSE,
						MUIA_FrameTitle, __(MSG_BACKGROUNDS),
						MUIA_Frame, MUIV_Frame_Group,

						Child, (IPTR) (_WP_Background_SpacerObj = HVSpace),
					End),

                    Child, (IPTR) (GroupObject,
						MUIA_Group_SameSize, FALSE,
                        MUIA_FrameTitle, __(MSG_ICONSPREFS),
                        MUIA_Frame, MUIV_Frame_Group,
						MUIA_Group_Columns, 2,
						Child, (IPTR) Label1(_(MSG_ICONLISTMODE)),
						Child, (IPTR) wped_icon_listmode,
						Child, (IPTR) Label1(_(MSG_ICONTEXTMODE)),
						Child, (IPTR) wped_icon_textmode,
						Child, (IPTR) Label1(_(MSG_ICONTEXTLENGTH)),
						Child, (IPTR) wped_icon_textmaxlen,
						Child, (IPTR) HVSpace,
						Child, (IPTR) HVSpace,
                    End),
                End),
                Child, (IPTR) (toolbarGroup = GroupObject,                     // toolbar 
					MUIA_Group_SameSize, FALSE,
                    Child, (IPTR) HGroup,
                        MUIA_FrameTitle,  __(MSG_OBJECTS),
                        MUIA_Group_SameSize, TRUE,
                        MUIA_Frame, MUIV_Frame_Group,
						MUIA_Group_Columns, 2,
						Child, (IPTR) HGroup,
							MUIA_Group_Columns, 2,
							MUIA_Group_SameSize, FALSE,
							Child, (IPTR) Label1(_(MSG_TOOLBAR_ENABLED)),
							Child, (IPTR) cm_toolbarenabled,
							Child, (IPTR) HVSpace,
							Child, (IPTR) HVSpace,
						End,
						Child, (IPTR) HGroup,
							MUIA_Group_Columns, 2,
							MUIA_Group_SameSize, FALSE,
							//Child, Label1("search"),
							//Child, cm_searchenabled = MUI_MakeObject(MUIO_Checkmark,NULL),
							Child, (IPTR) HVSpace,
							Child, (IPTR) HVSpace,
							Child, (IPTR) HVSpace,
							Child, (IPTR) HVSpace,
						End,
                    End,
                    Child, (IPTR) (toolbarpreview = HGroup,
                        MUIA_FrameTitle, __(MSG_PREVIEW),
                        MUIA_Frame, MUIV_Frame_Group,
                        MUIA_Group_SameSize, FALSE,
						Child, (IPTR) HVSpace,
                        Child, (IPTR) HGroup,
						    MUIA_HorizWeight, 0,
							MUIA_Group_SameSize, TRUE,
                            Child, (IPTR) (bt_dirup = ImageButton("", "THEME:Images/Gadgets/Prefs/Revert")),
                            Child, (IPTR) (bt_search = ImageButton("", "THEME:Images/Gadgets/Prefs/Test")),
                        End,
                    End),
                End),          
            End),
        TAG_DONE
    );

    if ((self != NULL) && (_WP_AdvancedBackgroundOptions_WindowData->wpedabwd_Window_WindowObj))
    {
        data = INST_DATA(CLASS, self);

D(bug("[WPEditor] WPEditor__OM_NEW: Prefs Object (self) @ %x\n", self));

		_wpeditor_intern_CLASS = CLASS;

		data->wped_AdvancedBG_WindowData                       = _WP_AdvancedBackgroundOptions_WindowData;
		
        data->wped_Background_GroupObj                         = _WP_Background_GroupObj;
        data->wped_Background_SpacerObj                        = _WP_Background_SpacerObj;
		
        data->wped_c_NavigationMethod                          = c_navitype;
        data->wped_cm_ToolbarEnabled                           = cm_toolbarenabled;
#if defined(DEBUG_NETWORKBROWSER)
        data->wped_cm_EnableNetworkBrowser                     = cm_EnableNetworkBrowser;
#endif
#if defined(DEBUG_SHOWUSERFILES)
        data->wped_cm_EnableUserFiles                          = cm_EnableUserFiles;
#endif
        data->wped_toolbarpreview                              = toolbarpreview;
		
        data->wped_icon_listmode                               = wped_icon_listmode;
        data->wped_icon_textmode                               = wped_icon_textmode;

        data->wped_icon_textmaxlen                             = wped_icon_textmaxlen;
        data->wped_toolbarGroup                                = toolbarGroup;
        data->wped_Hook_EnhancedNav.h_Entry                    = ( HOOKFUNC )WandererPrefs_Hook_EnhancedNavFunc;
        data->wped_Hook_CloseAdvancedOptions.h_Entry           = ( HOOKFUNC )WandererPrefs_Hook_CloseAdvancedOptionsFunc;

        //-- Setup notifications -------------------------------------------
        DoMethod
        (
            c_navitype, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,  
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );

		DoMethod
        (
            cm_toolbarenabled, MUIM_Notify, MUIA_Pressed, MUIV_EveryTime,  
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        ); 
        
        DoMethod
        (
            wped_icon_listmode, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        ); 
        DoMethod
        (
            wped_icon_textmode, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );        
                
        /* toolbar enabled checkmark */
        DoMethod
        (
            cm_toolbarenabled, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
        DoMethod
        (
            cm_toolbarenabled, MUIM_Notify, MUIA_Selected, FALSE,  
            (IPTR) toolbarpreview, 3, MUIM_Set, MUIA_Disabled, TRUE
        );    
        DoMethod
        (
            cm_toolbarenabled, MUIM_Notify, MUIA_Selected, TRUE,  
            (IPTR) toolbarpreview, 3, MUIM_Set, MUIA_Disabled, FALSE
        );

		/* navigation cycle linked to toolbar checkbox, enhanced nevigation sets toolbar */
        DoMethod (
            c_navitype, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_CallHook, &data->wped_Hook_EnhancedNav, (IPTR)CLASS
        );
        // Icon textmode maxlength
        SET( wped_icon_textmaxlen, MUIA_String_Integer, ICON_TEXT_MAXLEN_DEFAULT );
        SET( wped_icon_textmaxlen, MUIA_String_MaxLen, 3 );
        SET( wped_icon_textmaxlen, MUIA_String_Format, MUIV_String_Format_Right );
        SET( wped_icon_textmaxlen, MUIA_String_Accept, ( IPTR )"0123456789" ); 
        DoMethod ( 
            wped_icon_textmaxlen, MUIM_Notify, MUIA_String_Integer, MUIV_EveryTime,  
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE 
        );
		
		SET( prefs_pages, MUIA_Group_ActivePage, 1);  //Goto the Appearance page by default..

        /* Add the advanced window to the application, and set our background(s) notifications */
D(bug("[WPEditor] WPEditor__OM_NEW: Adding the advanced bg options Window Object to our app ..\n"));

		struct WPEditor_BackgroundObject *background_Node = NULL;

		ForeachNode(&_wpeditor_intern_Backgrounds, background_Node)
		{
			Object 		*thisBGImspecGrp = NULL;
			Object 		*thisBGAdvancedGrp = NULL;

D(bug("[WPEditor] WPEditor__OM_NEW: Adding Background Objects for '%s' to Prefs GUI ..\n", background_Node->wpedbo_BackgroundName));

			thisBGImspecGrp = GroupObject,
							MUIA_Group_SameSize, FALSE,
							MUIA_Frame, MUIV_Frame_None,
							MUIA_Group_Columns, 2,

							Child, (IPTR) HVSpace,
							Child, (IPTR) HVSpace,
							
							Child, (IPTR) Label1(background_Node->wpedbo_BackgroundName),
							Child, (IPTR) background_Node->wpedbo_ImageSpecObject,
							Child, (IPTR) HVSpace,
							Child, (IPTR) HVSpace,
						End;
						
			thisBGAdvancedGrp = GroupObject,
							MUIA_Group_SameSize, FALSE,
							MUIA_Frame, MUIV_Frame_None,
							MUIA_Group_Columns, 2,

							Child, (IPTR) HVSpace,
							Child, (IPTR) background_Node->wpedbo_AdvancedOptionsObject,
						End;
			
			if ((thisBGImspecGrp) && (thisBGAdvancedGrp))
			{
D(bug("[WPEditor] WPEditor__OM_NEW: GUI Objects Created ..\n"));

				if (DoMethod(_WP_Background_GroupObj, MUIM_Group_InitChange))
				{
					DoMethod(_WP_Background_GroupObj, OM_ADDMEMBER, thisBGImspecGrp);
					DoMethod(_WP_Background_GroupObj, OM_ADDMEMBER, thisBGAdvancedGrp);

					DoMethod(_WP_Background_GroupObj, MUIM_Group_ExitChange);
				}

D(bug("[WPEditor] WPEditor__OM_NEW: GUI Objects inserted in Prefs GUI ..\n"));

				DoMethod (
					background_Node->wpedbo_ImageSpecObject,
					MUIM_Notify, MUIA_Imagedisplay_Spec, MUIV_EveryTime,
					(IPTR)self, 3, MUIM_CallHook, 
					&background_Node->wpedbo_Hook_CheckImage, background_Node
				);

				background_Node->wpedbo_Hook_OpenAdvancedOptions.h_Entry = ( HOOKFUNC )WandererPrefs_Hook_OpenAdvancedOptionsFunc;
				
				DoMethod (
					background_Node->wpedbo_AdvancedOptionsObject, MUIM_Notify, MUIA_Pressed, FALSE,
					(IPTR)self, 3, MUIM_CallHook, &background_Node->wpedbo_Hook_OpenAdvancedOptions, background_Node
				);

				SET(background_Node->wpedbo_AdvancedOptionsObject, MUIA_Disabled, TRUE);
D(bug("[WPEditor] WPEditor__OM_NEW: GUI Objects Notifications set ..\n"));
			}
			else
			{
D(bug("[WPEditor] WPEditor__OM_NEW: Failed to create objects ..\n"));
				if (thisBGAdvancedGrp) DoMethod(thisBGAdvancedGrp, OM_DISPOSE);
					
				if (thisBGImspecGrp) DoMethod(thisBGImspecGrp, OM_DISPOSE);
			}
		}
    }
	else
	{
D(bug("[WPEditor] WPEditor__OM_NEW: Failed to create GUI ..\n"));
		if (_WP_AdvancedBackgroundOptions_WindowData->wpedabwd_Window_WindowObj) DoMethod(_WP_AdvancedBackgroundOptions_WindowData->wpedabwd_Window_WindowObj, OM_DISPOSE);
		if (self) DoMethod(self, OM_DISPOSE);

		self = NULL;
	}
    return self;
}

BOOL WPEditor_ProccessGlobalChunk(Class *CLASS, Object *self, struct WandererPrefs *global_chunk)
{
    SETUP_INST_DATA;
	
D(bug("[WPEditor] WPEditor_ProccessGlobalChunk()\n"));
#warning "TODO: fix problems with endian-ness?"
	//SMPByteSwap(global_chunk);

	 /* set navigation type */   
	SET(data->wped_c_NavigationMethod, MUIA_Cycle_Active, (IPTR)global_chunk->wpd_NavigationMethod);    

	/* check if toolbar set */
	if (global_chunk->wpd_ToolbarEnabled == FALSE)
	{
		SET(data->wped_toolbarpreview, MUIA_Disabled, TRUE);
		SET(data->wped_cm_ToolbarEnabled, MUIA_Selected, FALSE);
		DoMethod ( data->wped_toolbarGroup, MUIM_Group_InitChange );
		DoMethod ( data->wped_toolbarGroup, MUIM_Group_ExitChange );
	}
	else
	{
		SET(data->wped_toolbarpreview, MUIA_Disabled, FALSE);
		SET(data->wped_cm_ToolbarEnabled, MUIA_Selected, TRUE);
		DoMethod ( data->wped_toolbarGroup, MUIM_Group_InitChange );
		DoMethod ( data->wped_toolbarGroup, MUIM_Group_ExitChange );
	}        

	/* Icon listmode */
	SET( data->wped_icon_listmode, MUIA_Cycle_Active, (IPTR)global_chunk->wpd_IconListMode );
	
	/* Icon textmode */
	SET( data->wped_icon_textmode, MUIA_Cycle_Active, (IPTR)global_chunk->wpd_IconTextMode );
	
	/* set max text length */
	SET(data->wped_icon_textmaxlen, MUIA_String_Integer, (IPTR)global_chunk->wpd_IconTextMaxLen);

	return TRUE;
}

#if defined(DEBUG_NETWORKBROWSER)
BOOL WPEditor_ProccessNetworkChunk(Class *CLASS, Object *self, UBYTE *background_chunk)
{
    SETUP_INST_DATA;

	struct TagItem *network_tags = background_chunk;
	SET(data->wped_cm_EnableNetworkBrowser, MUIA_Selected, network_tags[0].ti_Data);

	return TRUE;
}
#endif

BOOL WPEditor_ProccessBackgroundChunk(Class *CLASS, Object *self, char *background_name, UBYTE *background_chunk, IPTR chunk_size)
{
    SETUP_INST_DATA;

D(bug("[WPEditor] WPEditor_ProccessBackgroundChunk()\n"));
	BOOL                              background_node_found = FALSE;
	struct WPEditor_BackgroundObject  *background_Node = NULL;

	background_Node = WPEditor__FindBackgroundObjects(background_name);

	if (background_Node)
	{
D(bug("[WPEditor] WPEditor_ProccessBackgroundChunk: Updating Existing node @ %x\n", background_Node));
	}
	else
	{
D(bug("[WPEditor] WPEditor_ProccessBackgroundChunk: Creating new Objects for '%s'\n", background_name));
		background_Node = WPEditor__NewBackgroundObjects(background_name);

		Object 		*thisBGImspecGrp = NULL;
		Object 		*thisBGAdvancedGrp = NULL;

D(bug("[WPEditor] WPEditor_ProccessBackgroundChunk: Adding Background Objects for '%s' to Prefs GUI ..\n", background_Node->wpedbo_BackgroundName));

		thisBGImspecGrp = GroupObject,
						MUIA_Group_SameSize, FALSE,
						MUIA_Frame, MUIV_Frame_None,
						MUIA_Group_Columns, 2,

						Child, (IPTR) HVSpace,
						Child, (IPTR) HVSpace,
						
						Child, (IPTR) Label1(background_Node->wpedbo_BackgroundName),
						Child, (IPTR) background_Node->wpedbo_ImageSpecObject,
						Child, (IPTR) HVSpace,
						Child, (IPTR) HVSpace,
					End;
					
		thisBGAdvancedGrp = GroupObject,
						MUIA_Group_SameSize, FALSE,
						MUIA_Frame, MUIV_Frame_None,
						MUIA_Group_Columns, 2,

						Child, (IPTR) HVSpace,
						Child, (IPTR) background_Node->wpedbo_AdvancedOptionsObject,
					End;
		
		if ((thisBGImspecGrp) && (thisBGAdvancedGrp))
		{
D(bug("[WPEditor] WPEditor_ProccessBackgroundChunk: GUI Objects Created ..\n"));

			if (DoMethod(data->wped_Background_GroupObj, MUIM_Group_InitChange))
			{
				DoMethod(data->wped_Background_GroupObj, OM_ADDMEMBER, thisBGImspecGrp);
				DoMethod(data->wped_Background_GroupObj, OM_ADDMEMBER, thisBGAdvancedGrp);

				DoMethod(data->wped_Background_GroupObj, MUIM_Group_ExitChange);
			}

D(bug("[WPEditor] WPEditor_ProccessBackgroundChunk: GUI Objects inserted in Prefs GUI ..\n"));

			DoMethod (
				background_Node->wpedbo_ImageSpecObject,
				MUIM_Notify, MUIA_Imagedisplay_Spec, MUIV_EveryTime,
				(IPTR)self, 3, MUIM_CallHook, 
				&background_Node->wpedbo_Hook_CheckImage, background_Node
			);

			DoMethod (
				background_Node->wpedbo_AdvancedOptionsObject, MUIM_Notify, MUIA_Pressed, FALSE,
				(IPTR)data->wped_AdvancedBG_WindowData->wpedabwd_Window_WindowObj, 3, MUIM_Set, MUIA_Window_Open, TRUE
			);

			SET(background_Node->wpedbo_AdvancedOptionsObject, MUIA_Disabled, TRUE);
D(bug("[WPEditor] WPEditor_ProccessBackgroundChunk: GUI Objects Notifications set ..\n"));
		}
		else
		{
D(bug("[WPEditor] WPEditor_ProccessBackgroundChunk: Failed to create objects ..\n"));
			if (thisBGAdvancedGrp) DoMethod(thisBGAdvancedGrp, OM_DISPOSE);
				
			if (thisBGImspecGrp) DoMethod(thisBGImspecGrp, OM_DISPOSE);
		}
	}

	if (chunk_size > (strlen(background_chunk) + 1))
	{
D(bug("[WPEditor] WPEditor_ProccessBackgroundChunk: Chunk has options Tag data ..\n"));
		UBYTE bgtag_offset = ((strlen(background_chunk)  + 1)/4);

		if ((bgtag_offset * 4) != (strlen(background_chunk)  + 1))
		{
			bgtag_offset = (bgtag_offset + 1) * 4;
D(bug("[WPEditor] WPEditor_ProccessBackgroundChunk: String length unalined - rounding up (length %d, rounded %d) \n", strlen(background_chunk) + 1, bgtag_offset ));
		}
		else
		{
			bgtag_offset = bgtag_offset * 4;
D(bug("[WPEditor] WPEditor_ProccessBackgroundChunk: String length doesnt need aligned (length %d) \n", strlen(background_chunk) + 1));
		}
		
		if (background_Node->wpedbo_Options)
		{
D(bug("[WPEditor] WPEditor_ProccessBackgroundChunk: Freeing old Background Tag data @ %x\n", background_Node->wpedbo_Options));
			FreeVec(background_Node->wpedbo_Options);
		}

		if (background_Node->wpedbo_Options = AllocVec(chunk_size - bgtag_offset, MEMF_CLEAR | MEMF_PUBLIC))
		{
D(bug("[WPEditor] WPEditor_ProccessBackgroundChunk: Allocated new Tag storage @ %x [%d bytes] \n", background_Node->wpedbo_Options, chunk_size - bgtag_offset));
			CopyMem(background_chunk + bgtag_offset, background_Node->wpedbo_Options, chunk_size - bgtag_offset);
		}
	}

	SET(background_Node->wpedbo_ImageSpecObject, MUIA_Imagedisplay_Spec, background_chunk);

	return TRUE;
}

IPTR WPEditor__MUIM_PrefsEditor_ImportFH
(
    Class *CLASS, Object *self, 
    struct MUIP_PrefsEditor_ImportFH *message
)
{
    SETUP_INST_DATA;
    
    struct ContextNode     *context;
    struct IFFHandle       *handle;
    BOOL                    success = TRUE;
    LONG                    error;
	IPTR                    iff_parse_mode = IFFPARSE_SCAN;
	
	UBYTE                    chunk_buffer[WP_IFF_CHUNK_BUFFER_SIZE];
	
    if (!(handle = AllocIFF()))
        return FALSE;
    
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
					
					error = ReadChunkBytes
					(
						handle, chunk_buffer, WP_IFF_CHUNK_BUFFER_SIZE
					);
					
					if (error == sizeof(struct WandererPrefsIFFChunkHeader))
					{
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH: ReadChunkBytes() Chunk matches Prefs Header size ..\n"));
						struct WandererPrefsIFFChunkHeader *this_header = chunk_buffer;
						char                               *this_chunk_name = NULL;
						IPTR                               this_chunk_size = this_header->wpIFFch_ChunkSize;
						
						if (this_chunk_name = AllocVec(strlen(this_header->wpIFFch_ChunkType) +1,MEMF_CLEAR|MEMF_PUBLIC))
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

									error = ReadChunkBytes
									(
										handle, chunk_buffer, this_chunk_size
									);
									
									if (error == this_chunk_size)
									{
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH: ReadChunkBytes() Chunk matches Prefs Data size .. (%d)\n", error));
										if ((strcmp(this_chunk_name, "wanderer:global")) == 0)
										{
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH: Process data for wanderer global config chunk ..\n"));
											WPEditor_ProccessGlobalChunk(CLASS, self, chunk_buffer);
										}
#if defined(DEBUG_NETWORKBROWSER)
										else if ((strcmp(this_chunk_name, "wanderer:network")) == 0)
										{
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH: Process data for wanderer network config chunk ..\n"));
											WPEditor_ProccessNetworkChunk(CLASS, self, chunk_buffer);
										}
#endif
										else if ((strncmp(this_chunk_name, "wanderer:background", strlen("wanderer:background"))) == 0)
										{
											char *bg_name = this_chunk_name + strlen("wanderer:background") + 1;
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH: Process data for wanderer background config chunk '%s'..\n", bg_name));
											WPEditor_ProccessBackgroundChunk(CLASS, self, bg_name, chunk_buffer, this_chunk_size);
										}
									}	
									if ((error = ParseIFF(handle, IFFPARSE_STEP)) == IFFERR_EOC)
									{
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH: End of Data chunk ..\n"));
									}
								}
							}				
						}
					}
				}
				else
				{
D(bug("[WPEditor] ParseIFF() failed, returncode %ld!\n", error));
					success = FALSE;
					//break;
				}

			} while (error != IFFERR_EOF);
		}
		else
		{
D(bug("[WPEditor] StopChunk() failed, returncode %ld!\n", error));
			success = FALSE;
		}

        CloseIFF(handle);
    }
    else
    {
D(bug("[WPEditor] Failed to open stream!, returncode %ld!\n", error));
        //ShowError(_(MSG_CANT_OPEN_STREAM));
    }

    FreeIFF(handle);

    return success;
}


IPTR WPEditor__MUIM_PrefsEditor_ExportFH
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ExportFH *message
)
{
    SETUP_INST_DATA;
    
    struct IFFHandle                       *handle;
    struct PrefHeader                      header = { 0 };
	struct WandererPrefsIFFChunkHeader     wanderer_chunkdata = { };
    BOOL                                   success = TRUE;
    LONG                                   error   = 0;
        
    if ((handle = AllocIFF()))
    {
        handle->iff_Stream = (IPTR) message->fh;
        
        InitIFFasDOS(handle);
        
        if (!(error = OpenIFF(handle, IFFF_WRITE))) /* NULL = successful! */
        {
            struct WandererPrefs wpd;    
            memset(&wpd, 0, sizeof(wpd));
                
            BYTE i;
			
D(bug("[WPEditor] Write IFF FORM Header Chunk ... \n")); /* FIXME: IFFSIZE_UNKNOWN? */
			if ((error = PushChunk(handle, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN)) != 0)
			{
D(bug("[WPEditor] IFF FORM Header Chunk : Error! %d \n", error));
				goto exportFH_CloseIFF;
			}
            
D(bug("[WPEditor] Write Preference File Header Chunk ... \n")); /* FIXME: IFFSIZE_UNKNOWN? */
			if ((error = PushChunk(handle, ID_PREF, ID_PRHD, IFFSIZE_UNKNOWN)) == 0)
			{
				header.ph_Version = PHV_CURRENT;
				header.ph_Type    = 0;
				
				WriteChunkBytes(handle, &header, sizeof(struct PrefHeader));
				
				if ((error = PopChunk(handle)) != 0)
				{
D(bug("[WPEditor] Preference File Header PopChunk() = %ld\n", error));
					goto exportFH_CloseFORM;
				}     
			}
			else
			{
D(bug("[WPEditor] Preference File Header Chunk : Error! %d \n", error));
				goto exportFH_CloseFORM;
			}

D(bug("[WPEditor] Write 'global' Wanderer Prefs Header Chunk ... \n"));
			if ((error = PushChunk(handle, ID_PREF, ID_WANDR, sizeof(struct WandererPrefsIFFChunkHeader))) == 0)
			{
				sprintf(wanderer_chunkdata.wpIFFch_ChunkType, "%s" , "wanderer:global");
				wanderer_chunkdata.wpIFFch_ChunkSize = sizeof(struct WandererPrefs);
				
				WriteChunkBytes(handle, &wanderer_chunkdata, sizeof(struct WandererPrefsIFFChunkHeader));
				
				if ((error = PopChunk(handle)) != 0)
				{
D(bug("[WPEditor] 'global' Header PopChunk() = %ld\n", error));
					goto exportFH_CloseFORM;
				}
			}
			else
			{
D(bug("[WPEditor] 'global' Wanderer Prefs Header Chunk : Error! %d \n", error));
				goto exportFH_CloseFORM;
			}	

D(bug("[WPEditor] Write 'global' Wanderer Prefs Data Chunk ... \n"));
			if ((error = PushChunk(handle, ID_PREF, ID_WANDR, sizeof(struct WandererPrefs))) == 0) 
			{
				/* save toolbar state*/
				GET(data->wped_cm_ToolbarEnabled, MUIA_Selected, &wpd.wpd_ToolbarEnabled);

				/* save navigation bahaviour */
				GET(data->wped_c_NavigationMethod, MUIA_Cycle_Active, &wpd.wpd_NavigationMethod);

				/* save the icon listing method */
				GET(data->wped_icon_listmode, MUIA_Cycle_Active, &wpd.wpd_IconListMode);

				/* save the icon text mode */
				GET(data->wped_icon_textmode, MUIA_Cycle_Active, &wpd.wpd_IconTextMode);

				/* save the max length of icons */
				GET(data->wped_icon_textmaxlen, MUIA_String_Integer, &wpd.wpd_IconTextMaxLen);

#warning "TODO: fix problems with endian-ness?"
				//SMPByteSwap(&wpd); 

				error = WriteChunkBytes(handle, &wpd, sizeof(struct WandererPrefs));
D(bug("[WPEditor] 'global' Data Chunk | Wrote %d bytes (data size = %d bytes)\n", error, sizeof(struct WandererPrefs)));
				if ((error = PopChunk(handle)) != 0)
				{
D(bug("[WPEditor] 'global' PopChunk() = %ld\n", error));
					goto exportFH_CloseFORM;
				}
			}
			else
			{
D(bug("[WPEditor] 'global' PushChunk() = %ld failed\n", error));
				goto exportFH_CloseFORM;
			}

#if defined(DEBUG_NETWORKBROWSER)
D(bug("[WPEditor] Write 'network' Wanderer Prefs Header Chunk ... \n"));
			if ((error = PushChunk(handle, ID_PREF, ID_WANDR, sizeof(struct WandererPrefsIFFChunkHeader))) == 0)
			{
				sprintf(wanderer_chunkdata.wpIFFch_ChunkType, "%s" , "wanderer:network");
				wanderer_chunkdata.wpIFFch_ChunkSize = sizeof(struct TagItem);
				
				WriteChunkBytes(handle, &wanderer_chunkdata, sizeof(struct WandererPrefsIFFChunkHeader));
				
				if ((error = PopChunk(handle)) != 0)
				{
D(bug("[WPEditor] 'network' Header PopChunk() = %ld\n", error));
					goto exportFH_CloseFORM;
				}
			}
			else
			{
D(bug("[WPEditor] 'network' Wanderer Prefs Header Chunk : Error! %d \n", error));
				goto exportFH_CloseFORM;
			}	

D(bug("[WPEditor] Write 'network' Wanderer Prefs Data Chunk ... \n"));
			if ((error = PushChunk(handle, ID_PREF, ID_WANDR, sizeof(struct TagItem))) == 0) 
			{
				struct TagItem __wp_networkconfig[2];

				/* save network options*/
				__wp_networkconfig[0].ti_Tag = MUIA_WandererPrefs_ShowNetworkBrowser;
				GET(data->wped_cm_EnableNetworkBrowser, MUIA_Selected, &__wp_networkconfig[0].ti_Data);

				error = WriteChunkBytes(handle, __wp_networkconfig, sizeof(struct TagItem));
D(bug("[WPEditor] 'network' Data Chunk | Wrote %d bytes (data size = %d bytes)\n", error, sizeof(struct TagItem)));
				if ((error = PopChunk(handle)) != 0)
				{
D(bug("[WPEditor] 'network' PopChunk() = %ld\n", error));
					goto exportFH_CloseFORM;
				}
			}
			else
			{
D(bug("[WPEditor] 'network' PushChunk() = %ld failed\n", error));
				goto exportFH_CloseFORM;
			}
#endif

			struct WPEditor_BackgroundObject *background_Node = NULL;
			ForeachNode(&_wpeditor_intern_Backgrounds, background_Node)
			{
				IPTR   				background_chunksize = 0, background_tagcounter = 0;
				struct TagItem 		background_taglist[WP_MAX_BG_TAG_COUNT];

				sprintf(wanderer_chunkdata.wpIFFch_ChunkType, "%s.%s" , "wanderer:background", background_Node->wpedbo_BackgroundName);

D(bug("[WPEditor] Write 'background' Wanderer Prefs Header Chunk  for '%s' ... \n", background_Node->wpedbo_BackgroundName));

				char *background_value = NULL;
				GET(background_Node->wpedbo_ImageSpecObject, MUIA_Imagedisplay_Spec, &background_value);
				
				if (background_value)
				{
					PushChunk(handle, ID_PREF, ID_WANDR, sizeof(struct WandererPrefsIFFChunkHeader));

					UBYTE bgtag_offset = ((strlen(background_value)  + 1)/4);

					if ((bgtag_offset * 4) != (strlen(background_value)  + 1))
					{
						bgtag_offset = (bgtag_offset + 1) * 4;
D(bug("[WPEditor] Write 'background' String length unalined - rounding up (length %d, rounded %d) \n", strlen(background_value) + 1, bgtag_offset ));
					}
					else
					{
						bgtag_offset = bgtag_offset * 4;
D(bug("[WPEditor] Write 'background' String length doesnt need aligned (length %d) \n", strlen(background_value) + 1));
					}

					background_chunksize += bgtag_offset;

					if (background_Node->wpedbo_Options)
					{
						UBYTE *background_typepointer = background_value;
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
								tstate = background_Node->wpedbo_Options;

								background_taglist[background_tagcounter].ti_Tag   = MUIA_WandererPrefs_Background_RenderMode;
								background_taglist[background_tagcounter++].ti_Data = GetTagData(MUIA_WandererPrefs_Background_RenderMode, WPD_BackgroundRenderMode_Tiled, tstate);
								
								while ((tag = NextTagItem(&tstate)) != NULL)
								{
									switch (tag->ti_Tag)
									{
										case MUIA_WandererPrefs_Background_TileMode:
										case MUIA_WandererPrefs_Background_XOffset:
										case MUIA_WandererPrefs_Background_YOffset:
											background_taglist[background_tagcounter].ti_Tag   = tag->ti_Tag;
											background_taglist[background_tagcounter++].ti_Data = tag->ti_Data;
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
					background_chunksize += (background_tagcounter * sizeof(struct TagItem));

					wanderer_chunkdata.wpIFFch_ChunkSize = background_chunksize;

					WriteChunkBytes(handle, &wanderer_chunkdata, sizeof(struct WandererPrefsIFFChunkHeader));

					PopChunk(handle);

D(bug("[WPEditor] Write 'background' Wanderer Prefs Data Chunk  for '%s' ... \n", background_Node->wpedbo_BackgroundName));

					if ((error = PushChunk(handle, ID_PREF, ID_WANDR, background_chunksize)) == 0)
					{
						UBYTE *background_chunkdata = AllocMem(background_chunksize, MEMF_CLEAR|MEMF_PUBLIC);
D(bug("[WPEditor] 'background' Chunk Data storage @ %x, %d bytes\n", background_chunkdata, background_chunksize));

						sprintf(background_chunkdata, "%s", background_value);
D(bug("[WPEditor] 'background' MUIA_Background = '%s'\n", background_chunkdata));
						if ((background_Node->wpedbo_Options)&&(background_tagcounter > 0))
						{
							struct TagItem 			*dest_tag = background_chunkdata + bgtag_offset;
D(bug("[WPEditor] 'background' Writing data for %d Tags @ %x\n", background_tagcounter, dest_tag));
							do
							{
								dest_tag[background_tagcounter - 1].ti_Tag = background_taglist[background_tagcounter - 1].ti_Tag;
								dest_tag[background_tagcounter - 1].ti_Data = background_taglist[background_tagcounter - 1].ti_Data;
								background_tagcounter --;
							}while(background_tagcounter > 0);
						}

						error = WriteChunkBytes(handle, background_chunkdata, background_chunksize);
D(bug("[WPEditor] 'background' Data Chunk | Wrote %d bytes (data size = %d bytes)\n", error, background_chunksize));
						if ((error = PopChunk(handle)) != 0) // TODO: We need some error checking here!
						{
D(bug("[WPEditor] 'background' Data PopChunk() = %ld\n", error));
						}
					}
					else
					{
D(bug("[WPEditor] 'background' Data PushChunk() = %ld failed\n", error));
					}
				}
				else
				{
D(bug("[WPEditor] 'background' Skipping (no value set) ... \n"));
				}
			}

exportFH_CloseFORM:

            /* Terminate the FORM */
            PopChunk(handle);
        }
        else
        {
            //ShowError(_(MSG_CANT_OPEN_STREAM));
D(bug("[WPEditor] Can't open stream!\n"));
            success = FALSE;
        }

exportFH_CloseIFF:

        CloseIFF(handle);
        FreeIFF(handle);
    }
    else // AllocIFF()
    {
        // Do something more here - if IFF allocation has failed, something isn't right
        //ShowError(_(MSG_CANT_ALLOCATE_IFFPTR));
        success = FALSE;
    }

    return success;
}


IPTR WPEditor__MUIM_Setup
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
	
D(bug("[IconWindow] WPEditor__MUIM_Setup()\n"));

    if (!DoSuperMethodA(CLASS, self, message)) return FALSE;
	
#if !defined(WP_DISABLE_ADVANCEDIMAGEOPTIONS)
		DoMethod(_app(self), OM_ADDMEMBER, data->wped_AdvancedBG_WindowData->wpedabwd_Window_WindowObj);
#endif
	DoMethod (
		data->wped_AdvancedBG_WindowData->wpedabwd_Window_RenderModeObj,
        MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
		(IPTR)self, 3, MUIM_CallHook,
        &data->wped_AdvancedBG_WindowData->wpedabwd_Hook_DrawModeChage, CLASS
	);

	DoMethod (
		data->wped_AdvancedBG_WindowData->wpedabwd_Window_UseObj,
		MUIM_Notify, MUIA_Pressed, FALSE,
		(IPTR)self, 3, MUIM_CallHook,
        &data->wped_Hook_CloseAdvancedOptions, TRUE
	);

	DoMethod (
		data->wped_AdvancedBG_WindowData->wpedabwd_Window_CancelObj,
		MUIM_Notify, MUIA_Pressed, FALSE,
		(IPTR)self, 3, MUIM_CallHook,
        &data->wped_Hook_CloseAdvancedOptions, FALSE
	);

	DoMethod (
		data->wped_AdvancedBG_WindowData->wpedabwd_Window_WindowObj,
		MUIM_Notify, MUIA_Window_CloseRequest, TRUE, 
		(IPTR)self, 3, MUIM_CallHook,
        &data->wped_Hook_CloseAdvancedOptions, FALSE
	);
	
	return TRUE;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_4
(
    WPEditor, NULL, MUIC_PrefsEditor, NULL,
    OM_NEW,                                struct opSet *,
	MUIM_Setup,                            Msg,
    MUIM_PrefsEditor_ImportFH,             struct MUIP_PrefsEditor_ImportFH *,
    MUIM_PrefsEditor_ExportFH,             struct MUIP_PrefsEditor_ExportFH *
);
