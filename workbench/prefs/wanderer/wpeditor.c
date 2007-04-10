/*
    Copyright  2004, The AROS Development Team. All rights reserved.
    This file is part of the Wanderer Preferences program, which is distributed
    under the terms of version 2 of the GNU General Public License.
    
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#define DEBUG 0

#define WP_MAX_BG_TAG_COUNT                20
#define WP_IFF_CHUNK_BUFFER_SIZE           1024
#define WP_DISABLE_ADVANCEDIMAGEOPTIONS

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
#define MUIM_WPrefsEditor_ShowAdvanced          (MUIB_WandererPrefs | 0x00000201)

/*** Instance Data **********************************************************/
struct WPEditor_BackgroundObject
{
	struct Node    wpedbo_Node;
	char           *wpedbo_BackgroundName;
	Object         *wpedbo_ImageSpecObject;
	Object         *wpedbo_AdvancedOptionsObject;
	IPTR           *wpedbo_Type;
	STRPTR   	   wpedbo_drawmodes[4];
    struct TagItem *wpedbo_Options;
    struct Hook    wpedbo_CheckImageHook;
};

struct WPEditor_DATA
{
	Object                          *wped_Background_GroupObj,
									*wped_Background_SpacerObj,
									*wped_AdvancedBackgroundOptions_WindowObj,
									*wped_AdvancedBackgroundOptions_DrawModeObj,
									*wped_AdvancedBackgroundOptions_PageObj;
    Object                          *wped_c_NavigationMethod,
                                      *wped_cm_ToolbarEnabled, 
                                      *wped_toolbarpreview;
	Object                            *wped_background_drawmode;
    Object                            *wped_icon_listmode;
    Object                            *wped_icon_textmode;
    Object                            *wped_icon_textmaxlen;
    Object                            *wped_toolbarGroup;
    struct Hook                       wped_EnhancedNavHook;
};

//static struct Hook navichangehook;
static STRPTR   navigationtypelabels[3];
static STRPTR   iconlistmodes[3];
static STRPTR   icontextmodes[3];
static STRPTR   registerpages[4];
static STRPTR   drawmodes[4];

static Class         *_wpeditor_intern_CLASS;
static struct List   _wpeditor_intern_Backgrounds;

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct WPEditor_DATA *data = INST_DATA(CLASS, self)

/*** Hook functions *********************************************************/

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
	
	UBYTE                  *ImageSelector_Spec = (UBYTE *)XGET(this_Background->wpedbo_ImageSpecObject, MUIA_Imagedisplay_Spec);
	
D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: Object @ %x reports image spec '%s'\n", this_Background->wpedbo_ImageSpecObject, (char *)ImageSelector_Spec));

	this_Background->wpedbo_Type = (IPTR)(ImageSelector_Spec[0] - 48);
	
	if ((this_Background->wpedbo_Type == 5)||(this_Background->wpedbo_Type == 0))
	{
D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: Image-type spec (%d) - Enabling Advanced options ..\n", this_Background->wpedbo_Type));
#if !defined(WP_DISABLE_ADVANCEDIMAGEOPTIONS)
		SET(this_Background->wpedbo_AdvancedOptionsObject, MUIA_Disabled, FALSE);
#endif
    }
	else
	{
D(bug("[WPEditor] WandererPrefs_Hook_CheckImageFunc: Not an Image-type spec - Disabling Advanced options ..\n"));
#if !defined(WP_DISABLE_ADVANCEDIMAGEOPTIONS)
		SET(this_Background->wpedbo_AdvancedOptionsObject, MUIA_Disabled, TRUE);
#endif
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
		this_Background->wpedbo_drawmodes[WPD_BackgroundRenderMode_Tiled - 1] = "Tiled";
		this_Background->wpedbo_drawmodes[WPD_BackgroundRenderMode_Scale - 1] = "Scaled";

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
				
				this_Background->wpedbo_CheckImageHook.h_Entry = ( HOOKFUNC )WandererPrefs_Hook_CheckImageFunc;
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
    
    Object 	*_WP_Background_GroupObj = NULL,
			*_WP_Background_SpacerObj = NULL,
			*_WP_AdvancedBackgroundOptions_WindowObj = NULL,
			*_WP_AdvancedBackgroundOptions_DrawModeObj = NULL,
			*_WP_AdvancedBackgroundOptions_PageObj = NULL,
			*c_navitype = NULL, 
			*bt_dirup = NULL,
            *bt_search = NULL,
            *cm_toolbarenabled = NULL, 
			*toolbarpreview = NULL,
			*wped_icon_listmode = NULL,
            *wped_icon_textmode = NULL, 
			*wped_icon_textmaxlen = NULL,
			*toolbarGroup = NULL,
			*prefs_pages = NULL;

D(bug("[WPEditor] WPEditor__OM_NEW()\n"));

	NewList(&_wpeditor_intern_Backgrounds);
	
	struct WPEditor_BackgroundObject *background_Workbench = NULL;
	struct WPEditor_BackgroundObject *background_Drawer    = NULL;

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

	drawmodes[WPD_BackgroundRenderMode_Tiled - 1] = "Tiled";
	drawmodes[WPD_BackgroundRenderMode_Scale - 1] = "Scaled";
	
    c_navitype = MUI_MakeObject(MUIO_Cycle, NULL, navigationtypelabels);

	wped_icon_listmode = MUI_MakeObject(MUIO_Cycle, NULL, iconlistmodes);
    wped_icon_textmode = MUI_MakeObject(MUIO_Cycle, NULL, icontextmodes);
    cm_toolbarenabled = MUI_MakeObject(MUIO_Checkmark,NULL);
    wped_icon_textmaxlen = MUI_MakeObject(MUIO_String,NULL,4);

	_WP_AdvancedBackgroundOptions_WindowObj = WindowObject,
										MUIA_Window_Title, (IPTR)"Advanced Options ..",
										WindowContents, (IPTR)VGroup,
											Child, (IPTR)(_WP_AdvancedBackgroundOptions_DrawModeObj = MUI_MakeObject(MUIO_Cycle, NULL, drawmodes)),
											Child, (IPTR)(_WP_AdvancedBackgroundOptions_PageObj = GroupObject,
												MUIA_Group_PageMode, TRUE,
												Child, (IPTR) GroupObject,
													Child, HVSpace,
													Child, (IPTR) Label1("Page 1"),
												End,
												Child, (IPTR) GroupObject,
													Child, HVSpace,
													Child, (IPTR) Label1("Page 2"),
												End,
											End),
										End,
									 End;

D(bug("[WPEditor] WPEditor__OM_NEW: 'Advanced' Window Object @ %x\n", _WP_AdvancedBackgroundOptions_WindowObj));

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
							Child, (IPTR) HVSpace,
							Child, (IPTR) HVSpace,
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

    if ((self != NULL) && (_WP_AdvancedBackgroundOptions_WindowObj != NULL))
    {
        data = INST_DATA(CLASS, self);

D(bug("[WPEditor] WPEditor__OM_NEW: Prefs Object (self) @ %x\n", self));

		_wpeditor_intern_CLASS = CLASS;

        data->wped_AdvancedBackgroundOptions_WindowObj         = _WP_AdvancedBackgroundOptions_WindowObj;
        data->wped_AdvancedBackgroundOptions_DrawModeObj       = _WP_AdvancedBackgroundOptions_DrawModeObj;
        data->wped_AdvancedBackgroundOptions_PageObj           = _WP_AdvancedBackgroundOptions_PageObj;

        data->wped_Background_GroupObj                         = _WP_Background_GroupObj;
        data->wped_Background_SpacerObj                        = _WP_Background_SpacerObj;
		
        data->wped_c_NavigationMethod                          = c_navitype;
        data->wped_cm_ToolbarEnabled                           = cm_toolbarenabled;
        data->wped_toolbarpreview                              = toolbarpreview;
		
        data->wped_icon_listmode                               = wped_icon_listmode;
        data->wped_icon_textmode                               = wped_icon_textmode;

        data->wped_icon_textmaxlen                             = wped_icon_textmaxlen;
        data->wped_toolbarGroup                                = toolbarGroup;
        data->wped_EnhancedNavHook.h_Entry                     = ( HOOKFUNC )WandererPrefs_Hook_EnhancedNavFunc;
		
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

//        DoMethod
//        (
//            wped_background_drawmode, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,  
//            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
//        );

		/* navigation cycle linked to toolbar checkbox, enhanced nevigation sets toolbar */
        DoMethod (
            c_navitype, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_CallHook, &data->wped_EnhancedNavHook, (IPTR)CLASS
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

//        DoMethod(_app(self), OM_ADDMEMBER, (IPTR)_WP_AdvancedBackgroundOptions_WindowObj);

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
					&background_Node->wpedbo_CheckImageHook, background_Node
				);

				DoMethod (
					background_Node->wpedbo_AdvancedOptionsObject, MUIM_Notify, MUIA_Pressed, MUIV_EveryTime,
					(IPTR)_WP_AdvancedBackgroundOptions_WindowObj, 3, MUIM_Set, MUIA_Window_Open, TRUE
				);
				
#if defined(WP_DISABLE_ADVANCEDIMAGEOPTIONS)
				SET(background_Node->wpedbo_AdvancedOptionsObject, MUIA_Disabled, TRUE);
#endif
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
		if (_WP_AdvancedBackgroundOptions_WindowObj) DoMethod(_WP_AdvancedBackgroundOptions_WindowObj, OM_DISPOSE);
		if (self) DoMethod(self, OM_DISPOSE);

		self = NULL;
	}
    return self;
}

IPTR WPEditor__MUIM_WPrefsEditor_ShowAdvanced
(
    Class *CLASS, Object *self, 
    struct MUIP_PrefsEditor_ImportFH *message
)
{
    SETUP_INST_DATA;
	return TRUE;
}

BOOL WPEditor_ProccessGlobalChunk(Class *CLASS, Object *self, struct WandererPrefs *global_chunk)
{
    SETUP_INST_DATA;
	
D(bug("[WPEDITOR] WPEditor_ProccessGlobalChunk()\n"));
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

BOOL WPEditor_ProccessBackgroundChunk(Class *CLASS, Object *self, char *background_name, UBYTE *background_chunk, IPTR chunk_size)
{
    SETUP_INST_DATA;

D(bug("[WPEDITOR] WPEditor_ProccessBackgroundChunk()\n"));
	BOOL                              background_node_found = FALSE;
	struct WPEditor_BackgroundObject  *background_Node = NULL;

	background_Node = WPEditor__FindBackgroundObjects(background_name);

	if (background_Node)
	{
D(bug("[WPEDITOR] WPEditor_ProccessBackgroundChunk: Updating Existing node @ %x\n", background_Node));
	}
	else
	{
D(bug("[WPEDITOR] WPEditor_ProccessBackgroundChunk: Creating new Objects for '%s'\n", background_name));
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
				&background_Node->wpedbo_CheckImageHook, background_Node
			);

			DoMethod (
				background_Node->wpedbo_AdvancedOptionsObject, MUIM_Notify, MUIA_Pressed, MUIV_EveryTime,
				(IPTR)data->wped_AdvancedBackgroundOptions_WindowObj, 3, MUIM_Set, MUIA_Window_Open, TRUE
			);

#if defined(WP_DISABLE_ADVANCEDIMAGEOPTIONS)
			SET(background_Node->wpedbo_AdvancedOptionsObject, MUIA_Disabled, TRUE);
#endif
D(bug("[WPEditor] WPEditor_ProccessBackgroundChunk: GUI Objects Notifications set ..\n"));
		}
		else
		{
D(bug("[WPEditor] WPEditor_ProccessBackgroundChunk: Failed to create objects ..\n"));
			if (thisBGAdvancedGrp) DoMethod(thisBGAdvancedGrp, OM_DISPOSE);
				
			if (thisBGImspecGrp) DoMethod(thisBGImspecGrp, OM_DISPOSE);
		}
	}

	NNSET(background_Node->wpedbo_ImageSpecObject, MUIA_Imagedisplay_Spec, background_chunk);

	if (chunk_size > (strlen(background_chunk) + 1))
	{
D(bug("[WPEditor] WPEditor_ProccessBackgroundChunk: Chunk has options Tag data ..\n"));
	}
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

D(bug("[WPEDITOR] WPEditor__MUIM_PrefsEditor_ImportFH: Context %x\n", context));
					
					error = ReadChunkBytes
					(
						handle, chunk_buffer, WP_IFF_CHUNK_BUFFER_SIZE
					);
					
					if (error == sizeof(struct WandererPrefsIFFChunkHeader))
					{
D(bug("[WPEDITOR] WPEditor__MUIM_PrefsEditor_ImportFH: ReadChunkBytes() Chunk matches Prefs Header size ..\n"));
						struct WandererPrefsIFFChunkHeader *this_header = chunk_buffer;
						char                               *this_chunk_name = NULL;
						IPTR                               this_chunk_size = this_header->wpIFFch_ChunkSize;
						
						if (this_chunk_name = AllocVec(strlen(this_header->wpIFFch_ChunkType) +1,MEMF_CLEAR|MEMF_PUBLIC))
						{
							strcpy(this_chunk_name, this_header->wpIFFch_ChunkType);
D(bug("[WPEDITOR] WPEditor__MUIM_PrefsEditor_ImportFH: Prefs Header for '%s' data size %d bytes\n", this_chunk_name, this_chunk_size));

							if ((error = ParseIFF(handle, IFFPARSE_STEP)) == IFFERR_EOC)
							{
D(bug("[WPEDITOR] WPEditor__MUIM_PrefsEditor_ImportFH: End of header chunk ..\n"));

								if ((error = ParseIFF(handle, IFFPARSE_STEP)) == 0)
								{
									context = CurrentChunk(handle);

D(bug("[WPEDITOR] WPEditor__MUIM_PrefsEditor_ImportFH: Context %x\n", context));

									error = ReadChunkBytes
									(
										handle, chunk_buffer, this_chunk_size
									);
									
									if (error == this_chunk_size)
									{
D(bug("[WPEDITOR] WPEditor__MUIM_PrefsEditor_ImportFH: ReadChunkBytes() Chunk matches Prefs Data size .. (%d)\n", error));
										if ((strcmp(this_chunk_name, "wanderer:global")) == 0)
										{
D(bug("[WPEDITOR] WPEditor__MUIM_PrefsEditor_ImportFH: Process data for wanderer global chunk ..\n"));
											WPEditor_ProccessGlobalChunk(CLASS, self, chunk_buffer);
										}
										else if ((strncmp(this_chunk_name, "wanderer:background", strlen("wanderer:background"))) == 0)
										{
											char *bg_name = this_chunk_name + strlen("wanderer:background") + 1;
D(bug("[WPEDITOR] WPEditor__MUIM_PrefsEditor_ImportFH: Process data for wanderer background chunk '%s'..\n", bg_name));
											WPEditor_ProccessBackgroundChunk(CLASS, self, bg_name, chunk_buffer, this_chunk_size);
										}
									}	
									if ((error = ParseIFF(handle, IFFPARSE_STEP)) == IFFERR_EOC)
									{
D(bug("[WPEDITOR] WPEditor__MUIM_PrefsEditor_ImportFH: End of Data chunk ..\n"));
									}
								}
							}				
						}
					}
				}
				else
				{
D(bug("[WPEDITOR] ParseIFF() failed, returncode %ld!\n", error));
					success = FALSE;
					//break;
				}

			} while (error != IFFERR_EOF);
		}
		else
		{
D(bug("[WPEDITOR] StopChunk() failed, returncode %ld!\n", error));
			success = FALSE;
		}

        CloseIFF(handle);
    }
    else
    {
D(bug("[WPEDITOR] Failed to open stream!, returncode %ld!\n", error));
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
			
D(bug("[WPEDITOR] Write IFF FORM Header Chunk ... \n")); /* FIXME: IFFSIZE_UNKNOWN? */
			if ((error = PushChunk(handle, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN)) != 0)
			{
D(bug("[WPEDITOR] IFF FORM Header Chunk : Error! %d \n", error));
				goto exportFH_CloseIFF;
			}
            
D(bug("[WPEDITOR] Write Preference File Header Chunk ... \n")); /* FIXME: IFFSIZE_UNKNOWN? */
			if ((error = PushChunk(handle, ID_PREF, ID_PRHD, IFFSIZE_UNKNOWN)) == 0)
			{
				header.ph_Version = PHV_CURRENT;
				header.ph_Type    = 0;
				
				WriteChunkBytes(handle, &header, sizeof(struct PrefHeader));
				
				if ((error = PopChunk(handle)) != 0)
				{
D(bug("[WPEDITOR] Preference File Header PopChunk() = %ld\n", error));
					goto exportFH_CloseFORM;
				}     
			}
			else
			{
D(bug("[WPEDITOR] Preference File Header Chunk : Error! %d \n", error));
				goto exportFH_CloseFORM;
			}

D(bug("[WPEDITOR] Write 'global' Wanderer Prefs Header Chunk ... \n"));
			if ((error = PushChunk(handle, ID_PREF, ID_WANDR, sizeof(struct WandererPrefsIFFChunkHeader))) == 0)
			{
				sprintf(wanderer_chunkdata.wpIFFch_ChunkType, "%s" , "wanderer:global");
				wanderer_chunkdata.wpIFFch_ChunkSize = sizeof(struct WandererPrefs);
				
				WriteChunkBytes(handle, &wanderer_chunkdata, sizeof(struct WandererPrefsIFFChunkHeader));
				
				if ((error = PopChunk(handle)) != 0)
				{
D(bug("[WPEDITOR] 'global' Header PopChunk() = %ld\n", error));
					goto exportFH_CloseFORM;
				}
			}
			else
			{
D(bug("[WPEDITOR] 'global' Wanderer Prefs Header Chunk : Error! %d \n", error));
				goto exportFH_CloseFORM;
			}	

D(bug("[WPEDITOR] Write 'global' Wanderer Prefs Data Chunk ... \n"));
			if ((error = PushChunk(handle, ID_PREF, ID_WANDR, sizeof(struct WandererPrefs))) == 0) 
			{
				/* save toolbar state*/
				get(data->wped_cm_ToolbarEnabled, MUIA_Selected, &wpd.wpd_ToolbarEnabled);

				/* save navigation bahaviour */
				get(data->wped_c_NavigationMethod, MUIA_Cycle_Active, &wpd.wpd_NavigationMethod);

				/* save the icon listing method */
				get(data->wped_icon_listmode, MUIA_Cycle_Active, &wpd.wpd_IconListMode);

				/* save the icon text mode */
				get(data->wped_icon_textmode, MUIA_Cycle_Active, &wpd.wpd_IconTextMode);

				/* save the max length of icons */
				get(data->wped_icon_textmaxlen, MUIA_String_Integer, &wpd.wpd_IconTextMaxLen);

#warning "TODO: fix problems with endian-ness?"
				//SMPByteSwap(&wpd); 

				error = WriteChunkBytes(handle, &wpd, sizeof(struct WandererPrefs));
D(bug("[WPEDITOR] 'global' Data Chunk | Wrote %d bytes (data size = %d bytes)\n", error, sizeof(struct WandererPrefs)));
				if ((error = PopChunk(handle)) != 0)
				{
D(bug("[WPEDITOR] 'global' PopChunk() = %ld\n", error));
					goto exportFH_CloseFORM;
				}
			}
			else
			{
D(bug("[WPEDITOR] 'global' PushChunk() = %ld failed\n", error));
				goto exportFH_CloseFORM;
			}

			struct WPEditor_BackgroundObject *background_Node = NULL;
			ForeachNode(&_wpeditor_intern_Backgrounds, background_Node)
			{
				IPTR   				background_chunksize = 0, background_tagcounter = 0;
				struct TagItem 		background_taglist[WP_MAX_BG_TAG_COUNT];

				sprintf(wanderer_chunkdata.wpIFFch_ChunkType, "%s.%s" , "wanderer:background", background_Node->wpedbo_BackgroundName);

D(bug("[WPEDITOR] Write 'background' Wanderer Prefs Header Chunk  for '%s' ... \n", background_Node->wpedbo_BackgroundName));

				char *background_value = (char *)XGET(background_Node->wpedbo_ImageSpecObject, MUIA_Imagedisplay_Spec);
				
				if (background_value)
				{
					PushChunk(handle, ID_PREF, ID_WANDR, sizeof(struct WandererPrefsIFFChunkHeader));

					background_chunksize += strlen(background_value) +1;

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
								tstate = background_Node->wpedbo_Options;

								while ((tag = NextTagItem(&tstate)) != NULL)
								{
									switch (tag->ti_Tag)
									{
										case MUIA_WandererPrefs_Background_RenderMode:
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
							}
							case 0:
							{
								//Pattern type -> store appropriate tags ..
								tstate = background_Node->wpedbo_Options;

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
							//Colour/Gradient type -> we dont store tags ..
								break;
						}
					}
					background_chunksize += background_tagcounter * sizeof(struct TagItem);

					wanderer_chunkdata.wpIFFch_ChunkSize = background_chunksize;

					WriteChunkBytes(handle, &wanderer_chunkdata, sizeof(struct WandererPrefsIFFChunkHeader));

					PopChunk(handle);

D(bug("[WPEDITOR] Write 'background' Wanderer Prefs Data Chunk  for '%s' ... \n", background_Node->wpedbo_BackgroundName));

					if ((error = PushChunk(handle, ID_PREF, ID_WANDR, background_chunksize)) == 0)
					{
						UBYTE *background_chunkdata = AllocMem(background_chunksize, MEMF_CLEAR|MEMF_PUBLIC);
						sprintf(background_chunkdata, "%s", background_value);
						if ((background_Node->wpedbo_Options)&&(background_tagcounter > 0))
						{
							struct TagItem 			*dest_tag = background_chunkdata + strlen(background_value) +2;
							do
							{
								dest_tag[background_tagcounter - 1].ti_Tag = background_taglist[background_tagcounter - 1].ti_Tag;
								dest_tag[background_tagcounter - 1].ti_Data = background_taglist[background_tagcounter - 1].ti_Data;
								background_tagcounter --;
							}while(background_tagcounter > 0);
						}

						error = WriteChunkBytes(handle, background_chunkdata, background_chunksize);
D(bug("[WPEDITOR] 'background' Data Chunk | Wrote %d bytes (data size = %d bytes)\n", error, background_chunksize));
						if ((error = PopChunk(handle)) != 0) // TODO: We need some error checking here!
						{
D(bug("[WPEDITOR] 'background' Data PopChunk() = %ld\n", error));
						}
					}
					else
					{
D(bug("[WPEDITOR] 'background' Data PushChunk() = %ld failed\n", error));
					}
				}
				else
				{
D(bug("[WPEDITOR] 'background' Skipping (no value set) ... \n"));
				}
			}

exportFH_CloseFORM:

            /* Terminate the FORM */
            PopChunk(handle);
        }
        else
        {
            //ShowError(_(MSG_CANT_OPEN_STREAM));
D(bug("[WPEDITOR] Can't open stream!\n"));
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



/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_4
(
    WPEditor, NULL, MUIC_PrefsEditor, NULL,
    OM_NEW,                                struct opSet *,
    MUIM_WPrefsEditor_ShowAdvanced,        Msg,
    MUIM_PrefsEditor_ImportFH,             struct MUIP_PrefsEditor_ImportFH *,
    MUIM_PrefsEditor_ExportFH,             struct MUIP_PrefsEditor_ExportFH *
);
