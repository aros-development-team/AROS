/*
    Copyright  2004-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#define ICONWINDOW_OPTION_NOSEARCHBUTTON

#define DEBUG 0
#include <aros/debug.h>

#include <exec/types.h>
#include <libraries/mui.h>
#include <zune/customclasses.h>

#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/muimaster.h>
#include <proto/exec.h>
#include <proto/datatypes.h>

#include <dos/dos.h>
#include <proto/dos.h>

#include <stdio.h>
#include <string.h>

#include <intuition/screens.h>
#include <datatypes/pictureclass.h>
#include <clib/macros.h>
#include <clib/alib_protos.h>

#include <prefs/wanderer.h>

#include "../../libs/muimaster/classes/iconlist.h"
#include "../../libs/muimaster/classes/iconlist_attributes.h"
#include "wanderer.h"
#include "wandererprefs.h"

#include "iconwindow.h"
#include "iconwindow_attributes.h"
#include "iconwindowcontents.h"
#include "iconwindowbackfill.h"

static char __intern_wintitle_wanderer[] = "Wanderer";

/*** Private Global Data *********************************************************/

static struct List                     iconwindow_BackFillNodes;
struct IconWindow_BackFill_Descriptor  *iconwindow_BackFill_Active = NULL;

/*** Hook functions *********************************************************/

AROS_UFH3(
    void, IconWindow__HookFunc_ToolbarLocationStringFunc,
    AROS_UFHA(struct Hook *,    hook,   A0),
    AROS_UFHA(APTR *,           obj,    A2),
    AROS_UFHA(APTR,             param,  A1)
)
{
    AROS_USERFUNC_INIT

    /* Get data */
    Object                *self = ( Object *)obj;
    Class                 *CLASS = *( Class **)param;
	STRPTR                str = NULL;
    BPTR                  fp = NULL;
    struct FileInfoBlock  fib;

    SETUP_ICONWINDOW_INST_DATA;

    /* Only change dir if it is a valid directory/volume */
    GET(data->iwd_Toolbar_LocationStringObj, MUIA_String_Contents, &str);

#warning "TODO: Signal that it is a wrong path"
    /* so that the user understands (here where we abort with return) */
    if (!(fp = Lock(str, &fib)))
        return;

    if (!(Examine(fp, &fib)))
    {
        UnLock (fp );
        return;
    }

    /* Change directory! */
    if (fib.fib_DirEntryType >= 0)
        SET(self, MUIA_IconWindow_Location, (IPTR)str);

    UnLock(fp);
    
    AROS_USERFUNC_EXIT
}


AROS_UFH3(
    void, IconWindow__HookFunc_PrefsUpdatedFunc,
    AROS_UFHA(struct Hook *,    hook,   A0),
    AROS_UFHA(APTR *,           obj,    A2),
    AROS_UFHA(APTR,             param,  A1)
)
{
    AROS_USERFUNC_INIT
    
    /* Get our private data */
    Object *self = ( Object *)obj;
    Class *CLASS = *( Class **)param;

    SETUP_ICONWINDOW_INST_DATA;

    D(bug("[IconWindow] IconWindow__HookFunc_PrefsUpdatedFunc()\n"));

	IPTR changed_state = 0;
	GET(self, MUIA_IconWindow_Changed, &changed_state);

	if ((changed_state) && (data->iwd_IconListObj))
	{
        D(bug("[IconWindow] IconWindow__HookFunc_PrefsUpdatedFunc: Window contents have changed .. updating display ..\n"));
		DoMethod(data->iwd_IconListObj, MUIM_IconList_Update);
		SET(self, MUIA_IconWindow_Changed, FALSE);
    }

    AROS_USERFUNC_EXIT
}


AROS_UFH3(
    void, IconWindow__HookFunc_ProcessBackgroundFunc,
    AROS_UFHA(struct Hook *,    hook,   A0),
    AROS_UFHA(APTR *,           obj,    A2),
    AROS_UFHA(APTR,             param,  A1)
)
{
    AROS_USERFUNC_INIT
    
    /* Get our private data */
    Object *self = ( Object *)obj,
	       *prefs = NULL;
    Class *CLASS = *( Class **)param;

    SETUP_ICONWINDOW_INST_DATA;

    D(bug("[IconWindow] IconWindow__HookFunc_ProcessBackgroundFunc()\n"));

	DoMethod(self, MUIM_IconWindow_BackFill_ProcessBackground, data->iwd_BackFillInfo, data->iwd_RootViewObj);

	GET(_app(self), MUIA_Wanderer_Prefs, &prefs);

	if (prefs)
	{
		BOOL    options_changed = FALSE;
		IPTR	prefs_Processing = 0;

		GET(prefs, MUIA_WandererPrefs_Processing, &prefs_Processing);
		if (!prefs_Processing)
		{
#warning "TODO: We arent in prefs-processing so cause an update!"
		}
	}

    AROS_USERFUNC_EXIT
}

AROS_UFH3(
    void, IconWindow__HookFunc_WandererBackFillFunc,
    AROS_UFHA(struct Hook *,        Hook,   A0),
    AROS_UFHA(struct RastPort *,    RP,    A2),
    AROS_UFHA(struct BackFillMsg *, BFM,  A1)
)
{
    AROS_USERFUNC_INIT
	
    D(bug("[IconWindow] IconWindow__HookFunc_WandererBackFillFunc()\n"));
	struct IconWindow_BackFillHookData *HookData = NULL;
		
	if ((HookData = Hook->h_Data) && (iconwindow_BackFill_Active != NULL))
	{
		Class                              *CLASS = HookData->bfhd_IWClass;
		Object 							   *self = HookData->bfhd_IWObject;

		SETUP_ICONWINDOW_INST_DATA;

		struct RastPort     		  *DrawBackGround_RastPort = _rp(data->iwd_RootViewObj);
		struct IconWindowBackFillMsg  DrawBackGround_BackFillMsg;

		DrawBackGround_BackFillMsg.Layer = DrawBackGround_RastPort->Layer;
		
		DrawBackGround_BackFillMsg.AreaBounds.MinX = _mleft(data->iwd_RootViewObj);
		DrawBackGround_BackFillMsg.AreaBounds.MinY = _mtop(data->iwd_RootViewObj);
		DrawBackGround_BackFillMsg.AreaBounds.MaxX = _mleft(data->iwd_RootViewObj) + _mwidth(data->iwd_RootViewObj) - 1;
		DrawBackGround_BackFillMsg.AreaBounds.MaxY = _mtop(data->iwd_RootViewObj) + _mheight(data->iwd_RootViewObj) - 1;

		DrawBackGround_BackFillMsg.DrawBounds.MinX = BFM->Bounds.MinX;
		DrawBackGround_BackFillMsg.DrawBounds.MinY = BFM->Bounds.MinY;
		DrawBackGround_BackFillMsg.DrawBounds.MaxX = BFM->Bounds.MaxX;
		DrawBackGround_BackFillMsg.DrawBounds.MaxY = BFM->Bounds.MaxY;

		/* Offset into source image (ala scroll bar position) */
		DrawBackGround_BackFillMsg.OffsetX = BFM->OffsetX;
		DrawBackGround_BackFillMsg.OffsetY = BFM->OffsetY;
		return DoMethod(self, MUIM_IconWindow_BackFill_DrawBackground, data->iwd_BackFillInfo, &DrawBackGround_BackFillMsg, DrawBackGround_RastPort);
	}

    AROS_USERFUNC_EXIT
}

/*** Methods ****************************************************************/
void IconWindow__SetupToolbar(Class *CLASS, Object *self, Object *prefs)
{
    SETUP_ICONWINDOW_INST_DATA;

    D(bug("[IconWindow] IconWindow__SetupToolbar()\n"));

    Object          *strObj = NULL,
                    *bt_dirup = NULL,
                    *bt_search = NULL;

    D(bug("[IconWindow] IconWindow__SetupToolbar: App PrefsObj @ %x\n", prefs));

	if (prefs != NULL)
	{
		data->iwd_Toolbar_PrefsNotificationObject = DoMethod(prefs,
															MUIM_WandererPrefs_ViewSettings_GetNotifyObject,
															"Toolbar");
		//Set up our prefs notification handlers ..

	}

    /* Create the "ToolBar" panel object .. */
    Object *toolbarPanel = VGroup,
        InnerSpacing(0, 0),
        MUIA_Frame, MUIV_Frame_None,
        Child, (IPTR)HGroup,
            InnerSpacing(4, 4),
	        MUIA_Frame, MUIV_Frame_None,
            MUIA_Weight, 100,
			Child, (IPTR)HGroup,
				InnerSpacing(0, 0),
				MUIA_Weight, 100,
				Child, (IPTR)( strObj = StringObject,
					MUIA_String_Contents, (IPTR)"",
					MUIA_CycleChain, 1,
					MUIA_Frame, MUIV_Frame_String,
				End ),
			End,
			Child, (IPTR)HGroup,
				InnerSpacing(0, 0),
				MUIA_HorizWeight,   0,
				MUIA_VertWeight,    100,
				Child, (IPTR) (bt_dirup = ImageButton("", "THEME:Images/Gadgets/Prefs/Revert")),
#if !defined(ICONWINDOW_OPTION_NOSEARCHBUTTON)
				Child, (IPTR) (bt_search = ImageButton("", "THEME:Images/Gadgets/Prefs/Test")),
#endif
			End,
		End,
		Child, (IPTR)HGroup,
			InnerSpacing(0, 0),
			MUIA_Group_Spacing, 0,
			MUIA_FixHeight, 1,
	        MUIA_Frame, MUIV_Frame_None,
			MUIA_Background, MUII_SHADOW,
            Child, (IPTR)RectangleObject,
				MUIA_Frame, MUIV_Frame_None,
			End,
		End,
	End;
    
    /* Got a toolbarpanel? setup notifies and other values are 
       copied to the data of the object */
    if ( toolbarPanel != NULL )
    {
        SET(bt_dirup, MUIA_Background, XGET( toolbarPanel, MUIA_Background ) );
		SET(bt_dirup, MUIA_CycleChain, 1);
        SET(bt_dirup, MUIA_Frame, MUIV_Frame_None );
#if !defined(ICONWINDOW_OPTION_NOSEARCHBUTTON)
        SET(bt_search, MUIA_Background, XGET( toolbarPanel, MUIA_Background ) );
		SET(bt_search, MUIA_CycleChain, 1);
        SET(bt_search, MUIA_Frame, MUIV_Frame_None );
#endif

        if (DoMethod( data->iwd_ExtensionGroupObj, MUIM_Group_InitChange ))
		{
			DoMethod(data->iwd_ExtensionGroupObj, OM_ADDMEMBER, (IPTR)toolbarPanel);
			if (data->iwd_ExtensionGroupSpacerObj)
			{
				DoMethod(data->iwd_ExtensionGroupObj, OM_REMMEMBER, (IPTR)data->iwd_ExtensionGroupSpacerObj);
				data->iwd_ExtensionGroupSpacerObj = NULL;
			}

			DoMethod(data->iwd_ExtensionGroupObj, MUIM_Group_ExitChange);
			data->iwd_Toolbar_PanelObj = toolbarPanel;
		}

		if (data->iwd_Toolbar_PanelObj)
		{
			DoMethod( 
				bt_dirup, MUIM_Notify, MUIA_Pressed, FALSE, 
				(IPTR)self, 1, MUIM_IconWindow_DirectoryUp
			);

			data->iwd_Toolbar_LocationStringObj = strObj;

			data->iwd_pathStrHook.h_Entry = ( HOOKFUNC )IconWindow__HookFunc_ToolbarLocationStringFunc;
			SET(
				data->iwd_Toolbar_LocationStringObj, MUIA_String_Contents, 
				XGET(data->iwd_IconListObj, MUIA_IconDrawerList_Drawer)
			);

			/* Make changes to string contents change dir on enter */
			DoMethod ( 
				strObj, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, 
				(IPTR)self, 3, MUIM_CallHook, &data->iwd_pathStrHook, (IPTR)CLASS
			);
		}
    }
    else
    {
        data->iwd_Toolbar_PanelObj = NULL;
    }
}

Object *IconWindow__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    struct Screen 	                *_newIconWin__Screen = NULL;
    Object                          *_newIconWin__IconListObj = NULL,
		                            *_newIconWin__RootViewObj = NULL, 
                                    *_newIconWin__ExtensionContainerObj = NULL, // around extension group
                                    *_newIconWin__ExtensionGroupObj = NULL,     // extension group top
                                    *_newIconWin__ExtensionGroupSpacerObj = NULL,     // extension group top
									*prefs = NULL;

    char                            *_newIconWin__Title = NULL;
	
    BOOL                            isRoot = FALSE,
									isBackdrop = FALSE,
                                    hasToolbar = FALSE;

    struct Hook                 	*actionHook = NULL;
    struct TextFont         	    *_newIconWin__WindowFont = NULL;

	struct Hook				        *_newIconWin__BackFillHook = NULL;

	APTR                            WindowBF_TAG = TAG_IGNORE;

#warning "TODO: We should probably obtain the windows coords/dimensions from a .info file ?"
	IPTR                            _newIconWin__WindowWidth = 300;
	IPTR                            _newIconWin__WindowHeight = 300;
	IPTR                            _newIconWin__WindowTop = 0;
	IPTR                            _newIconWin__WindowLeft = 0;

    D(bug("[iconwindow] IconWindow__OM_NEW()\n"));
	
    /* More than one GetTagData is not very efficient, however since this isn't called very often... */

    isBackdrop = (BOOL)GetTagData(MUIA_IconWindow_IsBackdrop, (IPTR)FALSE, message->ops_AttrList);

    if (!(isRoot = (BOOL)GetTagData(MUIA_IconWindow_IsRoot, (IPTR)FALSE, message->ops_AttrList)))
		hasToolbar = (BOOL)GetTagData(MUIA_IconWindowExt_Toolbar_Enabled, (IPTR)FALSE, message->ops_AttrList);

    actionHook = (struct Hook *)GetTagData(MUIA_IconWindow_ActionHook, (IPTR) NULL, message->ops_AttrList);
    _newIconWin__WindowFont = (struct TextFont *)GetTagData(MUIA_IconWindow_Font, (IPTR) NULL, message->ops_AttrList);
	prefs = (Object *)GetTagData(MUIA_Wanderer_Prefs, (IPTR) NULL, message->ops_AttrList);

	/* Request the screen we should use .. */

	if (!(_newIconWin__Screen = (struct Hook *)GetTagData(MUIA_Wanderer_Screen, (IPTR) NULL, message->ops_AttrList)))
	{
        D(bug("[IconWindow] IconWindow__OM_NEW: NO SCREEN SET!\n"));
		return NULL;
	}
    D(bug("[iconwindow] IconWindow__OM_NEW: Screen @ %x\n", _newIconWin__Screen));

	if ((_newIconWin__BackFillHook = AllocVec(sizeof(struct Hook), MEMF_CLEAR|MEMF_PUBLIC))!=NULL)
	{
        D(bug("[IconWindow] IconWindow__OM_NEW: Allocated WindowBackFillHook @ %x\n", _newIconWin__BackFillHook));
		_newIconWin__BackFillHook->h_Entry = ( HOOKFUNC )IconWindow__HookFunc_WandererBackFillFunc;
#if defined(__MORPHOS__)
		WindowBF_TAG = MUIA_Window_BackFillHook;
#else
		WindowBF_TAG = WA_BackFill;
#endif
	}

    if (isRoot)
    {
        _newIconWin__IconListObj = (IPTR)IconWindowIconVolumeListObject,
            MUIA_Font, (IPTR)_newIconWin__WindowFont,
        End;

		_newIconWin__WindowWidth = GetBitMapAttr(_newIconWin__Screen->RastPort.BitMap, BMA_WIDTH);
		_newIconWin__WindowHeight = GetBitMapAttr(_newIconWin__Screen->RastPort.BitMap, BMA_HEIGHT);
        D(bug("[iconwindow] IconWindow__OM_NEW: Screen dimensions ..  %d x %d\n", _newIconWin__WindowWidth, _newIconWin__WindowHeight));

		if (isBackdrop)
		{
            D(bug("[iconwindow] IconWindow__OM_NEW: BACKDROP ROOT Window\n"));
			_newIconWin__Title = NULL;
		}
		else
		{
            D(bug("[iconwindow] IconWindow__OM_NEW: Plain ROOT Window\n"));
			_newIconWin__Title = __intern_wintitle_wanderer;
		}
		
		_newIconWin__WindowTop = _newIconWin__Screen->BarHeight + 1;
		_newIconWin__WindowLeft = 0;
		
		_newIconWin__WindowHeight -= _newIconWin__WindowTop;
    }
    else
    {
        D(bug("[iconwindow] IconWindow__OM_NEW: Directory Window\n"));
        _newIconWin__Title = (STRPTR) GetTagData(MUIA_IconWindow_Location, (IPTR)NULL, message->ops_AttrList);

        _newIconWin__IconListObj = (IPTR)IconWindowIconDrawerListObject,
            MUIA_Font, (IPTR)_newIconWin__WindowFont,
            MUIA_IconDrawerList_Drawer, (IPTR) _newIconWin__Title,
        End;

		_newIconWin__ExtensionGroupObj = GroupObject,
            InnerSpacing(0,0),
            MUIA_Frame, MUIV_Frame_None,
            MUIA_Group_Spacing, 0,
			Child, (_newIconWin__ExtensionGroupSpacerObj = HSpace(0)),
		End;

		if (_newIconWin__ExtensionGroupObj)
		{
			_newIconWin__ExtensionContainerObj = HGroup,
				InnerSpacing(0,0),
				MUIA_HorizWeight,   100,
				MUIA_VertWeight,    0,
				MUIA_Frame,         MUIV_Frame_None,
				MUIA_Group_Spacing, 3,
				/* extension on top of the list */
				Child, (IPTR)_newIconWin__ExtensionGroupObj,
			End;
		}
		_newIconWin__WindowTop = MUIV_Window_TopEdge_Centered;
		_newIconWin__WindowLeft = MUIV_Window_LeftEdge_Centered;
    }
    D(bug("[iconwindow] IconWindow__OM_NEW: Using dimensions ..  %d x %d\n", _newIconWin__WindowWidth, _newIconWin__WindowHeight));

	_newIconWin__RootViewObj = (IPTR) IconListviewObject,
			MUIA_Weight,                           100,
			MUIA_IconListview_UseWinBorder,        TRUE,
			MUIA_IconListview_IconList,     (IPTR) _newIconWin__IconListObj,
		End;

    D(bug("[iconwindow] IconWindow__OM_NEW: Font @ %x\n", _newIconWin__WindowFont));
	
    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,

		MUIA_Window_Screen,                                    _newIconWin__Screen,
		MUIA_Window_Backdrop,                                  isBackdrop ? TRUE : FALSE,
		MUIA_Window_Borderless,                                isBackdrop ? TRUE : FALSE,
        MUIA_Window_Width,                                     _newIconWin__WindowWidth,
        MUIA_Window_Height,                                    _newIconWin__WindowHeight,
        MUIA_Window_LeftEdge,                                  _newIconWin__WindowLeft,
        MUIA_Window_TopEdge,                                   _newIconWin__WindowTop,
        (!isBackdrop) ? MUIA_Window_AltWidth : TAG_IGNORE,     100,
        (!isBackdrop) ? MUIA_Window_AltHeight : TAG_IGNORE,    80,
		MUIA_Window_Title,                  	               (IPTR)_newIconWin__Title,
		
		MUIA_Window_DragBar,                                   (!isBackdrop) ? TRUE : FALSE,
		MUIA_Window_CloseGadget,                               (!isBackdrop) ? TRUE : FALSE,
		MUIA_Window_SizeGadget,                                (!isBackdrop) ? TRUE : FALSE,
		MUIA_Window_DepthGadget,                               (!isBackdrop) ? TRUE : FALSE,
#if defined(MUIA_Window_ZoomGadget)
		MUIA_Window_ZoomGadget,                                (!isBackdrop) ? TRUE : FALSE,
#endif
		MUIA_Window_UseBottomBorderScroller,                   (!isBackdrop) ? TRUE : FALSE,
		MUIA_Window_UseRightBorderScroller,                    (!isBackdrop) ? TRUE : FALSE,
        MUIA_Window_IsSubWindow,            	               TRUE,
		WindowBF_TAG,                       	               _newIconWin__BackFillHook,
        MUIA_Window_ScreenTitle,            	               (IPTR) "",
        MUIA_Font,                          	               (IPTR) _newIconWin__WindowFont,

        WindowContents, (IPTR) VGroup,
            MUIA_Group_Spacing,  0,
			MUIA_Group_SameSize, FALSE,
            InnerSpacing(0,0),

            /* "Extension" group */
            _newIconWin__ExtensionContainerObj ? Child : TAG_IGNORE, (IPTR)_newIconWin__ExtensionContainerObj,

            /* icon list */
            Child, (IPTR) _newIconWin__RootViewObj,
            
        End,
        
        TAG_MORE, (IPTR) message->ops_AttrList
    );
    
    if (self != NULL)
    {
        SETUP_ICONWINDOW_INST_DATA;

        D(bug("[iconwindow] IconWindow__OM_NEW: SELF = %x\n", self));

		data->iwd_Title                   = _newIconWin__Title;

		data->iwd_RootViewObj             = _newIconWin__RootViewObj;

        data->iwd_IconListObj             = _newIconWin__IconListObj;

		SET(data->iwd_RootViewObj, MUIA_IconWindow_Window,      self);

        data->iwd_ActionHook              = actionHook;

        data->iwd_ExtensionGroupObj       = _newIconWin__ExtensionGroupObj;
        data->iwd_ExtensionContainerObj   = _newIconWin__ExtensionContainerObj;
        data->iwd_ExtensionGroupSpacerObj = _newIconWin__ExtensionGroupSpacerObj;

        data->iwd_Toolbar_PanelObj         = NULL;

        data->iwd_Flag_ISROOT             = isRoot;
        data->iwd_Flag_ISBACKDROP         = isBackdrop;

        data->iwd_WindowFont              = _newIconWin__WindowFont;        

		data->iwd_ViewSettings_Attrib = data->iwd_Flag_ISROOT 
                    ? "Workbench"
                    : "Drawer";

		if (prefs)
		{
			data->iwd_PrefsUpdated_hook.h_Entry = ( HOOKFUNC )IconWindow__HookFunc_PrefsUpdatedFunc;

			DoMethod
			(
				prefs, MUIM_Notify, MUIA_WandererPrefs_Processing, FALSE,
				(IPTR) self, 3, 
				MUIM_CallHook, &data->iwd_PrefsUpdated_hook, (IPTR)CLASS
			);

			data->iwd_ViewSettings_PrefsNotificationObject = DoMethod(prefs,
                                                         		MUIM_WandererPrefs_ViewSettings_GetNotifyObject,
																data->iwd_ViewSettings_Attrib);

			if (data->iwd_ExtensionContainerObj)
			{
				DoMethod
				(
					prefs, MUIM_Notify, MUIA_IconWindowExt_Toolbar_Enabled, MUIV_EveryTime, 
					(IPTR)self, 3, MUIM_Set, MUIA_IconWindowExt_Toolbar_Enabled, MUIV_TriggerValue
				);
			}
		}

		data->iwd_ProcessBackground_hook.h_Entry = ( HOOKFUNC )IconWindow__HookFunc_ProcessBackgroundFunc;
		if (data->iwd_BackFill_hook = _newIconWin__BackFillHook)
		{
			data->iwd_BackFillHookData.bfhd_IWClass = CLASS;
			data->iwd_BackFillHookData.bfhd_IWObject = self;
			data->iwd_BackFill_hook->h_Data = &data->iwd_BackFillHookData;
		}

		if (iconwindow_BackFill_Active)
		{
			data->iwd_BackFillInfo = DoMethod(self, MUIM_IconWindow_BackFill_Setup);
            D(bug("[iconwindow] IconWindow__OM_NEW: Window BackFill_Data @ %x for '%s'\n", data->iwd_BackFillInfo, iconwindow_BackFill_Active->bfd_BackFillID));
		}

        /* no tool bar when root */
        if (!isRoot && hasToolbar && data->iwd_ExtensionContainerObj)
			IconWindow__SetupToolbar(CLASS, self, prefs);

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

    return self;
}

IPTR IconWindow__OM_SET(Class *CLASS, Object *self, struct opSet *message)
{
    SETUP_ICONWINDOW_INST_DATA;

    struct TagItem  *tstate = message->ops_AttrList, *tag;
    BOOL 			UpdateIconlist = FALSE;
	IPTR 			focusicon = NULL;
    IPTR  			rv = TRUE;

    while ((tag = NextTagItem((const struct TagItem**)&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
		case MUIA_IconWindow_Changed:
			data->iwd_Flag_NEEDSUPDATE = (BOOL)tag->ti_Data;
			break;

		case MUIA_Window_Open:
			{
				/* Commented out for unknown reason - please elaborate here!
				if (data->iwd_WindowFont)
				{
D(bug("[iconwindow] MUIA_Window_Open: Setting Window Font [%x]\n", data->iwd_WindowFont));
					SetFont(_rp(self), data->iwd_WindowFont);
				}
				*/
				break;
			}

		case MUIA_Window_Activate:
			if (data->iwd_IconListObj)
				GET(data->iwd_IconListObj, MUIA_IconList_FocusIcon, &focusicon);

			break;

		case MUIA_IconWindow_Font:
			data->iwd_WindowFont = (struct TextFont  *)tag->ti_Data;
            D(bug("[iconwindow] IconWindow__OM_SET: MUIA_IconWindow_Font [font @ %x]\n", data->iwd_WindowFont));

			if ( data->iwd_WindowFont != 1 )
				SetFont(_rp(self), data->iwd_WindowFont);

			break;

		case MUIA_IconWindow_Location:
			strcpy(data->iwd_DirectoryPath, (IPTR)tag->ti_Data);
            D(bug("[iconwindow] IconWindow__OM_SET: MUIA_IconWindow_Location [drawer '%s']\n", data->iwd_DirectoryPath));

			SET(data->iwd_IconListObj, MUIA_IconDrawerList_Drawer, (IPTR) data->iwd_DirectoryPath);
			if (!data->iwd_Flag_ISROOT)
			{
				SET(self, MUIA_Window_Title, (IPTR)data->iwd_DirectoryPath);
				SET(data->iwd_Toolbar_LocationStringObj, MUIA_String_Contents, (IPTR)data->iwd_DirectoryPath);
			}
			break;

		case MUIA_IconWindow_BackgroundAttrib:
            D(bug("[iconwindow] IconWindow__OM_SET: MUIA_IconWindow_BackgroundAttrib (not implemented)\n"));
			break;

		case MUIA_IconWindowExt_Toolbar_Enabled:   
			if ((!(data->iwd_Flag_ISROOT)) && (data->iwd_ExtensionContainerObj))
			{
				// remove toolbar
				if (!(( BOOL )tag->ti_Data))
				{
					if (data->iwd_Toolbar_PanelObj != NULL)
					{
						data->iwd_ExtensionGroupSpacerObj = HSpace(0);
						
						SET(data->iwd_ExtensionContainerObj, MUIA_Frame, MUIV_Frame_None);
						SET(data->iwd_ExtensionContainerObj, MUIA_Group_Spacing, 0);

						if ((data->iwd_ExtensionGroupSpacerObj) && (DoMethod(data->iwd_ExtensionGroupObj, MUIM_Group_InitChange)))
						{
							DoMethod(data->iwd_ExtensionGroupObj, OM_REMMEMBER, (IPTR)data->iwd_Toolbar_PanelObj);
							DoMethod(data->iwd_ExtensionGroupObj, OM_ADDMEMBER, (IPTR)data->iwd_ExtensionGroupSpacerObj);
							DoMethod(data->iwd_ExtensionGroupObj, MUIM_Group_ExitChange);
							data->iwd_Toolbar_PanelObj = NULL;
						}
					}
					//Force classic navigation when the toolbar is disabled ..
					Object *prefs = NULL;

					GET(_app(self), MUIA_Wanderer_Prefs, &prefs);
					if (prefs)
						SET(prefs, MUIA_IconWindowExt_Toolbar_NavigationMethod, WPD_NAVIGATION_CLASSIC);
				}
				else
				{
					// setup toolbar
					if (data->iwd_Toolbar_PanelObj == NULL)
					{
						Object *prefs = NULL;
						GET(_app(self), MUIA_Wanderer_Prefs, &prefs);
						IconWindow__SetupToolbar(CLASS, self, prefs);
					}
				 }
				 data->iwd_Flag_EXT_TOOLBARENABLED = (IPTR)tag->ti_Data;
			 }
			 break;     
        }
    }

    if (UpdateIconlist)
        DoMethod(data->iwd_IconListObj, MUIM_IconList_Update);

	rv = DoSuperMethodA(CLASS, self, (Msg) message);

	if (focusicon)
	{
        D(bug("[iconwindow] IconWindow__OM_SET: Updating focused icon (@ %x)\n", focusicon));
		DoMethod(data->iwd_IconListObj, MUIM_IconList_DrawEntry, focusicon, ICONENTRY_DRAWMODE_PLAIN);
		DoMethod(data->iwd_IconListObj, MUIM_IconList_DrawEntryLabel, focusicon, ICONENTRY_DRAWMODE_PLAIN);
	}

    return rv;
}


IPTR IconWindow__OM_GET(Class *CLASS, Object *self, struct opGet *message)
{
    SETUP_ICONWINDOW_INST_DATA;

    IPTR *store = message->opg_Storage;
    IPTR  rv    = TRUE;
    
    switch (message->opg_AttrID)
    {
		case MUIA_IconWindow_Changed:
			*store = (IPTR)data->iwd_Flag_NEEDSUPDATE;
			break;

		case MUIA_IconWindow_Window:
            *store = (IPTR)self;
            break;

        case MUIA_IconWindow_Location:
            *store = !data->iwd_Flag_ISROOT
                ? XGET(data->iwd_IconListObj, MUIA_IconDrawerList_Drawer)
                : (IPTR)NULL;
            break;

        case MUIA_IconWindow_IconList:
            *store = (IPTR)data->iwd_IconListObj;
            break;

        case MUIA_IconWindowExt_Toolbar_Enabled:
            *store = (IPTR)data->iwd_Flag_EXT_TOOLBARENABLED;
            break;

        case MUIA_IconWindow_IsRoot:
            *store = (IPTR)data->iwd_Flag_ISROOT;
            break;

		case MUIA_IconWindow_IsBackdrop:
			*store = (IPTR)data->iwd_Flag_ISBACKDROP;
			break;

		case MUIA_IconWindow_BackFillData:
			*store = (IPTR)data->iwd_BackFillInfo;
            break;

		case MUIA_IconWindow_BackgroundAttrib:
			*store = (IPTR)data->iwd_ViewSettings_Attrib;
            break;

        default:
            rv = DoSuperMethodA(CLASS, self, (Msg) message);
    }
    
    return rv;
}

IPTR IconWindow__MUIM_Window_Setup
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_ICONWINDOW_INST_DATA;
	
    D(bug("[IconWindow] IconWindow__MUIM_Window_Setup()\n"));
	
	Object *prefs = NULL;
	
    if (!DoSuperMethodA(CLASS, self, message)) return FALSE;

	GET(_app(self), MUIA_Wanderer_Prefs, &prefs);

    D(bug("[IconWindow] IconWindow__MUIM_Window_Setup: App PrefsObj @ %x\n", prefs));
	
	if ((prefs) && (data->iwd_ViewSettings_PrefsNotificationObject))
	{
        D(bug("[IconWindow] IconWindow__MUIM_Window_Setup: Setting up window background change hook\n"));

		/* Set-up a hook to call ProcessBackground on prefs notification */
		DoMethod
		(
			data->iwd_ViewSettings_PrefsNotificationObject, MUIM_Notify, MUIA_Background, MUIV_EveryTime,
			(IPTR) self, 3, 
			MUIM_CallHook, &data->iwd_ProcessBackground_hook, (IPTR)CLASS
		);
		
		if (data->iwd_Flag_ISROOT)
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
	}
	
    D(bug("[iconwindow] IconWindow__MUIM_Window_Setup: Setup complete!\n"));
	
    return TRUE;
}

IPTR IconWindow__MUIM_Window_Cleanup
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_ICONWINDOW_INST_DATA;
	
    D(bug("[IconWindow] IconWindow__MUIM_Window_Cleanup()\n"));
	
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

		if (data->iwd_Flag_ISROOT)
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
	}
    return DoSuperMethodA(CLASS, self, message);
}

IPTR IconWindow__MUIM_IconWindow_DoubleClicked
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_ICONWINDOW_INST_DATA;
	
    D(bug("[IconWindow] IconWindow__MUIM_IconWindow_DoubleClicked()\n"));
	
    if (data->iwd_ActionHook)
    {
        struct IconWindow_ActionMsg msg;
        msg.type     = ICONWINDOW_ACTION_OPEN;
        msg.iconlist = data->iwd_IconListObj;
        msg.isroot   = data->iwd_Flag_ISROOT;
        msg.click    = NULL;
        CallHookPkt(data->iwd_ActionHook, self, &msg);
    }
    
    return TRUE;
}

IPTR IconWindow__MUIM_IconWindow_Clicked
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_ICONWINDOW_INST_DATA;
	
    D(bug("[IconWindow] IconWindow__MUIM_IconWindow_Clicked()\n"));
	
    if (data->iwd_ActionHook)
    {
        struct IconWindow_ActionMsg msg;
        msg.type     = ICONWINDOW_ACTION_CLICK;
        msg.iconlist = data->iwd_IconListObj;
        msg.isroot   = data->iwd_Flag_ISROOT;
		GET(data->iwd_IconListObj, MUIA_IconList_Clicked, &msg.click);
        CallHookPkt(data->iwd_ActionHook, self, &msg);
    }
    
    return TRUE;
}

IPTR IconWindow__MUIM_IconWindow_IconsDropped
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_ICONWINDOW_INST_DATA;
	
    D(bug("[IconWindow] IconWindow__MUIM_IconWindow_IconsDropped()\n"));
	
    if (data->iwd_ActionHook)
    {
        struct IconWindow_ActionMsg msg;
        msg.type     = ICONWINDOW_ACTION_ICONDROP;
        msg.iconlist = data->iwd_IconListObj;
        msg.isroot   = data->iwd_Flag_ISROOT;
        GET(data->iwd_IconListObj, MUIA_IconList_IconsDropped, &msg.drop);
        CallHookPkt(data->iwd_ActionHook, self, &msg);
    }
    
    return TRUE;
}

IPTR IconWindow__MUIM_IconWindow_AppWindowDrop
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_ICONWINDOW_INST_DATA;
	
D(bug("[IconWindow] IconWindow__MUIM_IconWindow_AppWindowDrop()\n"));
	
    if (data->iwd_ActionHook)
    {
        struct IconWindow_ActionMsg msg;
        msg.type     = ICONWINDOW_ACTION_APPWINDOWDROP;
        msg.iconlist = data->iwd_IconListObj;
        msg.isroot   = data->iwd_Flag_ISROOT;
        GET(data->iwd_IconListObj, MUIA_IconList_IconsDropped, &msg.drop);
        CallHookPkt(data->iwd_ActionHook, self, &msg);
    }
    
    return TRUE;
}

IPTR IconWindow__MUIM_IconWindow_Open
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_ICONWINDOW_INST_DATA;
	
    D(bug("[IconWindow] IconWindow__MUIM_IconWindow_Open()\n"));
	
    if (!XGET(self, MUIA_Window_Open))
    {
        DoMethod(data->iwd_IconListObj, MUIM_IconList_Clear);
        SET(self, MUIA_Window_Open, TRUE);

        D(bug("[IconWindow] IconWindow__MUIM_IconWindow_Open: Process the background ..\n"));
		DoMethod(self, MUIM_IconWindow_BackFill_ProcessBackground, data->iwd_BackFillInfo, data->iwd_RootViewObj);
    }
    D(bug("[IconWindow] IconWindow__MUIM_IconWindow_Open: Force an update of the list ..\n"));
    DoMethod(data->iwd_IconListObj, MUIM_IconList_Update);

    D(bug("[IconWindow] IconWindow__MUIM_IconWindow_Open: Setting window as active ..\n"));
    SET(self, MUIA_Window_Activate, TRUE);

    D(bug("[IconWindow] IconWindow__MUIM_IconWindow_Open: All done\n"));
    return TRUE;
}

IPTR IconWindow__MUIM_IconWindow_DirectoryUp
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_ICONWINDOW_INST_DATA;
	
    D(bug("[IconWindow] IconWindow__MUIM_IconWindow_DirectoryUp()\n"));
	
    if (data->iwd_ActionHook)
    {
        struct IconWindow_ActionMsg msg;
        msg.type     = ICONWINDOW_ACTION_DIRUP;
        msg.iconlist = data->iwd_IconListObj;
        msg.isroot   = data->iwd_Flag_ISROOT;
        msg.click    = NULL;
        CallHookPkt(data->iwd_ActionHook, self, &msg);
        
    }
    
    return TRUE;
}

IPTR IconWindow__MUIM_IconWindow_UnselectAll
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_ICONWINDOW_INST_DATA;
	
    D(bug("[IconWindow] IconWindow__MUIM_IconWindow_UnselectAll()\n"));
	
    DoMethod(data->iwd_IconListObj, MUIM_IconList_UnselectAll);
    
    return TRUE;
}

IPTR IconWindow__MUIM_IconWindow_Remove
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_ICONWINDOW_INST_DATA;
	
    D(bug("[IconWindow] IconWindow__MUIM_IconWindow_Remove()\n"));
	
    // Remove window
    SET( _window(self), MUIA_Window_Open, FALSE );
    DoMethod ( _app(self), OM_REMMEMBER, _window(self) );
    DoMethod ( _app(self), OM_REMMEMBER, self );
    DoMethod ( _window(self), OM_DISPOSE );
    
    return TRUE;
}

/*** Stubs for Backfill Hooks ******************************************************************/

IPTR IconWindow__MUIM_IconWindow_BackFill_Register
(
    Class *CLASS, Object *self, struct MUIP_IconWindow_BackFill_Register *message
)
{
//    SETUP_ICONWINDOW_INST_DATA;
	
    D(bug("[IconWindow] IconWindow__MUIM_IconWindow_BackFill_Register('%s')\n", message->register_Node->bfd_BackFillID));

	AddTail(&iconwindow_BackFillNodes, message->register_Node);
	if (iconwindow_BackFill_Active == NULL) iconwindow_BackFill_Active = message->register_Node;

    return TRUE;
}

IPTR IconWindow__MUIM_IconWindow_BackFill_Setup
(
    Class *CLASS, Object *self, struct MUIP_IconWindow_BackFill_Setup *message
)
{
    SETUP_ICONWINDOW_INST_DATA;
	
    D(bug("[IconWindow] IconWindow__MUIM_IconWindow_BackFill_Setup()\n"));

	if (iconwindow_BackFill_Active == NULL) return FALSE;

	return (iconwindow_BackFill_Active->bfd_MUIM_IconWindow_BackFill_Setup)(CLASS, self, message);
}

IPTR IconWindow__MUIM_IconWindow_BackFill_Cleanup
(
    Class *CLASS, Object *self, struct MUIP_IconWindow_BackFill_Cleanup *message
)
{
    SETUP_ICONWINDOW_INST_DATA;
	
    D(bug("[IconWindow] IconWindow__MUIM_IconWindow_BackFill_Cleanup()\n"));

	if (iconwindow_BackFill_Active == NULL) return FALSE;

	return (iconwindow_BackFill_Active->bfd_MUIM_IconWindow_BackFill_Cleanup)(CLASS, self, message);
}

IPTR IconWindow__MUIM_IconWindow_BackFill_ProcessBackground
(
    Class *CLASS, Object *self, struct MUIP_IconWindow_BackFill_ProcessBackground *message
)
{
    SETUP_ICONWINDOW_INST_DATA;
	
	IPTR retVal = (IPTR)FALSE;

    D(bug("[IconWindow] IconWindow__MUIM_IconWindow_BackFill_ProcessBackground()\n"));

	if (iconwindow_BackFill_Active != NULL)
	{
        D(bug("[IconWindow] IconWindow__MUIM_IconWindow_BackFill_ProcessBackground: Asking module @ %x to process ..\n", iconwindow_BackFill_Active));
		retVal = (iconwindow_BackFill_Active->bfd_MUIM_IconWindow_BackFill_ProcessBackground)(CLASS, self, message);
	}
	
	if ((retVal == (IPTR)FALSE) && (data->iwd_RootViewObj != NULL))
	{
		Object                *IconWindowPB_PrefsObj = NULL;

        D(bug("[IconWindow] IconWindow__MUIM_IconWindow_BackFill_ProcessBackground: No BackFill module/ module cant render mode\n"));
        D(bug("[IconWindow] IconWindow__MUIM_IconWindow_BackFill_ProcessBackground: Using default MUI functions ..\n"));
		
		GET(_app(self), MUIA_Wanderer_Prefs, &IconWindowPB_PrefsObj);
		if (IconWindowPB_PrefsObj)
		{
			IPTR IconWindowPB_Background = 0;
			IPTR IconWindowPB_BGMode     = 0;
			IPTR IconWindowPB_BGTileMode = 0;

			if ((IconWindowPB_Background = DoMethod(IconWindowPB_PrefsObj, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwd_ViewSettings_Attrib, MUIA_Background)) != -1)
			{
				if ((IconWindowPB_BGMode = DoMethod(IconWindowPB_PrefsObj, MUIM_WandererPrefs_ViewSettings_GetAttribute,
												data->iwd_ViewSettings_Attrib, MUIA_IconWindowExt_ImageBackFill_BGRenderMode)) == -1)
					IconWindowPB_BGMode = IconWindowExt_ImageBackFill_RenderMode_Tiled;

				if ((IconWindowPB_BGTileMode = DoMethod(IconWindowPB_PrefsObj, MUIM_WandererPrefs_ViewSettings_GetAttribute,
																	data->iwd_ViewSettings_Attrib, MUIA_IconWindowExt_ImageBackFill_BGTileMode)) == -1)
					IconWindowPB_BGTileMode = IconWindowExt_ImageBackFill_TileMode_Float;
				
				SET(data->iwd_RootViewObj, MUIA_Background, IconWindowPB_Background);

				char *bgmode_string = IconWindowPB_Background;
				BYTE this_mode = bgmode_string[0] - 48;

                D(bug("[IconWindow] IconWindow__MUIM_IconWindow_BackFill_ProcessBackground: MUI BG Mode = %d\n", this_mode));
				
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

IPTR IconWindow__MUIM_IconWindow_BackFill_DrawBackground
(
    Class *CLASS, Object *self, struct MUIP_IconWindow_BackFill_DrawBackground *message
)
{
    SETUP_ICONWINDOW_INST_DATA;
	
    D(bug("[IconWindow] IconWindow__MUIM_IconWindow_BackFill_DrawBackground()\n"));

	if (iconwindow_BackFill_Active == NULL) return FALSE;
	
	return (iconwindow_BackFill_Active->bfd_MUIM_IconWindow_BackFill_DrawBackground)(CLASS, self, message);
}

BOOL IconWindow__SetupClass()
{
    D(bug("[IconWindow] IconWindow__SetupClass()\n"));
	
	NewList(&iconwindow_BackFillNodes);
	iconwindow_BackFill_Active = NULL;
	
	return TRUE;
}

/*** Setup ******************************************************************/
ICONWINDOW_CUSTOMCLASS
(
    IconWindow, NULL, MUIC_Window, NULL,
    OM_NEW,                                     struct opSet *,
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
    MUIM_IconWindow_BackFill_Register,          struct MUIP_IconWindow_BackFill_Register *,
    MUIM_IconWindow_BackFill_Setup,             struct MUIP_IconWindow_BackFill_Setup *,
    MUIM_IconWindow_BackFill_Cleanup,           struct MUIP_IconWindow_BackFill_Cleanup *,
	MUIM_IconWindow_BackFill_ProcessBackground, struct MUIP_IconWindow_BackFill_ProcessBackground *,
	MUIM_IconWindow_BackFill_DrawBackground,    struct MUIP_IconWindow_BackFill_DrawBackground *
);
