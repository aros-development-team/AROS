/*
	Copyright  2004-2006, The AROS Development Team. All rights reserved.
	$Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

//#define DEBUG_NETWORKBROWSER
//#define DEBUG_SHOWUSERFILES
#define TXTBUFF_LEN 1024

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
#include <proto/icon.h>

#include <dos/dos.h>
#include <proto/dos.h>

#include <stdio.h>
#include <string.h>

#include <intuition/screens.h>
#include <datatypes/pictureclass.h>
#include <clib/macros.h>

#include "../../libs/muimaster/classes/iconlist.h"
#include "../../libs/muimaster/classes/iconlist_attributes.h"

#include "wanderer.h"
#include "wandererprefs.h"
#include "iconwindow.h"
#include "iconwindowcontents.h"

extern struct IconWindow_BackFill_Descriptor  *iconwindow_BackFill_Active;

#define	BG_DRAWFLAG   0xf00dd00f

/*** Instance Data **********************************************************/

struct IconWindowIconList_DATA
{
	Object                       *iwcd_IconWindow;
	struct MUI_EventHandlerNode  iwcd_EventHandlerNode;
	struct Hook					 iwcd_ProcessIconListPrefs_hook;
	IPTR                         iwcd_ViewPrefs_ID;
	Object                       *iwcd_ViewPrefs_NotificationObject;
};

struct IconWindowIconVolumeList_DATA
{
	Object                       *iwcd_IconWindow;
	struct MUI_EventHandlerNode  iwcd_EventHandlerNode;
	struct Hook					 iwcd_ProcessIconListPrefs_hook;
	IPTR                         iwcd_ViewPrefs_ID;
	Object                       *iwcd_ViewPrefs_NotificationObject;
	struct Hook                  iwvcd_UpdateNetworkPrefs_hook;
	IPTR						 iwvcd_ShowNetworkBrowser;
	IPTR						 iwvcd_ShowUserFolder;
	char                         *iwvcd_UserFolderPath;

};

struct IconWindowIconNetworkBrowserList_DATA
{
	Object                       *iwcd_IconWindow;
	struct MUI_EventHandlerNode  iwcd_EventHandlerNode;
	struct Hook					 iwcd_ProcessIconListPrefs_hook;
	IPTR                         iwcd_ViewPrefs_ID;
	Object                       *iwcd_ViewPrefs_NotificationObject;
	struct Hook                  iwnbcd_UpdateNetworkPrefs_hook;
	struct List                  iwnbcd_NetworkClasses;
};

static char __icwc_intern_TxtBuff[TXTBUFF_LEN];

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct IconWindowIconList_DATA *data = INST_DATA(CLASS, self)

#define IconWindowIconDrawerList_DATA             IconWindowIconList_DATA

/*** Hook functions *********************************************************/
AROS_UFH3(
	void, IconWindowIconList__HookFunc_ProcessIconListPrefsFunc,
	AROS_UFHA(struct Hook *,    hook,   A0),
	AROS_UFHA(APTR *,           obj,    A2),
	AROS_UFHA(APTR,             param,  A1)
)
{
	AROS_USERFUNC_INIT
	
	/* Get our private data */
	Object *self = ( Object *)obj;
	Object *prefs = NULL;
	Class *CLASS = *( Class **)param;

	SETUP_INST_DATA;

	D(bug("[IconWindowIconList] IconWindowIconList__HookFunc_ProcessIconListPrefsFunc()\n"));

	GET(_app(self), MUIA_Wanderer_Prefs, &prefs);

	if (prefs)
	{
		D(bug("[IconWindowIconList] IconWindowIconList__HookFunc_ProcessIconListPrefsFunc: Setting IconList options ..\n"));
		BOOL    options_changed = FALSE;

		IPTR   	current_ListMode = 0,
				current_TextMode = 0,
				current_TextMaxLen = 0,
				current_MultiLine = 0,
				current_MultiLineOnFocus = 0,
				current_IconHorizontalSpacing = 0,
				current_IconVerticalSpacing = 0,
				current_IconImageSpacing = 0,
				current_LabelTextHorizontalPadding = 0,
				current_LabelTextVerticalPadding = 0,
				current_LabelTextBorderWidth = 0,
				current_LabelTextBorderHeight = 0;

		GET(self, MUIA_IconList_IconListMode, &current_ListMode);
		GET(self, MUIA_IconList_LabelText_Mode, &current_TextMode);
		GET(self, MUIA_IconList_LabelText_MaxLineLen, &current_TextMaxLen);

		GET(self, MUIA_IconList_LabelText_MultiLine, &current_MultiLine);
		GET(self, MUIA_IconList_LabelText_MultiLineOnFocus, &current_MultiLineOnFocus);
		GET(self, MUIA_IconList_Icon_HorizontalSpacing, &current_IconHorizontalSpacing);
		GET(self, MUIA_IconList_Icon_VerticalSpacing, &current_IconVerticalSpacing);
		GET(self, MUIA_IconList_Icon_ImageSpacing, &current_IconImageSpacing);
		GET(self, MUIA_IconList_LabelText_HorizontalPadding, &current_LabelTextHorizontalPadding);
		GET(self, MUIA_IconList_LabelText_VerticalPadding, &current_LabelTextVerticalPadding);
		GET(self, MUIA_IconList_LabelText_BorderWidth, &current_LabelTextBorderWidth);
		GET(self, MUIA_IconList_LabelText_BorderHeight, &current_LabelTextBorderHeight);

//D(bug("[IconWindowIconList] IconWindowIconList__HookFunc_ProcessIconListPrefsFunc: Current = %d %d %d\n", current_ListMode, current_TextMode, current_TextMaxLen));

		IPTR    prefs_ListMode = 0,
				prefs_TextMode = 0,
				prefs_TextMaxLen = 0,
				prefs_MultiLine = 0,
				prefs_MultiLineOnFocus = 0,
				prefs_IconHorizontalSpacing = 0,
				prefs_IconVerticalSpacing = 0,
				prefs_IconImageSpacing = 0,
				prefs_LabelTextHorizontalPadding = 0,
				prefs_LabelTextVerticalPadding = 0,
				prefs_LabelTextBorderWidth = 0,
				prefs_LabelTextBorderHeight = 0,
				prefs_Processing = 0;

		prefs_ListMode = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_IconListMode);
		prefs_TextMode = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_LabelText_Mode);
		prefs_TextMaxLen = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_LabelText_MaxLineLen);

		prefs_MultiLine = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_LabelText_MultiLine);
		prefs_MultiLineOnFocus = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_LabelText_MultiLineOnFocus);
		prefs_IconHorizontalSpacing = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_Icon_HorizontalSpacing);
		prefs_IconVerticalSpacing = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_Icon_VerticalSpacing);
		prefs_IconImageSpacing = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_Icon_ImageSpacing);
		prefs_LabelTextHorizontalPadding = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_LabelText_HorizontalPadding);
		prefs_LabelTextVerticalPadding = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_LabelText_VerticalPadding);
		prefs_LabelTextBorderWidth = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_LabelText_BorderWidth);
		prefs_LabelTextBorderHeight = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_LabelText_BorderHeight);

		GET(prefs, MUIA_WandererPrefs_Processing, &prefs_Processing);

//D(bug("[IconWindowIconList] IconWindowIconList__HookFunc_ProcessIconListPrefsFunc: Prefs = %d %d %d\n", prefs_ListMode, prefs_TextMode, prefs_TextMaxLen));

		if ((prefs_ListMode != -1) && (current_ListMode != prefs_ListMode))
		{
			D(bug("[IconWindowIconList] IconWindowIconList__HookFunc_ProcessIconListPrefsFunc: IconList ListMode changed - updating ..\n"));
			options_changed = TRUE;
			SET(self, MUIA_IconList_IconListMode, prefs_ListMode);
		}
		if ((prefs_TextMode != -1) && (current_TextMode != prefs_TextMode))
		{
			D(bug("[IconWindowIconList] IconWindowIconList__HookFunc_ProcessIconListPrefsFunc: IconList TextRenderMode changed - updating ..\n"));
			options_changed = TRUE;
			SET(self, MUIA_IconList_LabelText_Mode, prefs_TextMode);
		}
		if ((prefs_TextMaxLen != -1) && (current_TextMaxLen != prefs_TextMaxLen))
		{
			D(bug("[IconWindowIconList] IconWindowIconList__HookFunc_ProcessIconListPrefsFunc: IconList Max Text Length changed - updating ..\n"));
			options_changed = TRUE;
			SET(self, MUIA_IconList_LabelText_MaxLineLen, prefs_TextMaxLen);
		}
		if ((prefs_MultiLine != -1) && (current_MultiLine != prefs_MultiLine))
		{
			D(bug("[IconWindowIconList] IconWindowIconList__HookFunc_ProcessIconListPrefsFunc: IconList Multi-Line changed - updating ..\n"));
			options_changed = TRUE;
			SET(self, MUIA_IconList_LabelText_MultiLine, prefs_MultiLine);
		}
		if ((prefs_MultiLineOnFocus != -1) && (current_MultiLineOnFocus != prefs_MultiLineOnFocus))
		{
			D(bug("[IconWindowIconList] IconWindowIconList__HookFunc_ProcessIconListPrefsFunc: Multi-Line on Focus changed - updating ..\n"));
			options_changed = TRUE;
			SET(self, MUIA_IconList_LabelText_MultiLineOnFocus, prefs_MultiLineOnFocus);
		}
		if ((prefs_IconHorizontalSpacing != -1) && (current_IconHorizontalSpacing != prefs_IconHorizontalSpacing))
		{
			D(bug("[IconWindowIconList] IconWindowIconList__HookFunc_ProcessIconListPrefsFunc: Icon Horizontal Spacing changed - updating ..\n"));
			options_changed = TRUE;
			SET(self, MUIA_IconList_Icon_HorizontalSpacing, prefs_IconHorizontalSpacing);
		}
		if ((prefs_IconVerticalSpacing != -1) && (current_IconVerticalSpacing != prefs_IconVerticalSpacing))
		{
			D(bug("[IconWindowIconList] IconWindowIconList__HookFunc_ProcessIconListPrefsFunc: Icon Vertical Spacing changed - updating ..\n"));
			options_changed = TRUE;
			SET(self, MUIA_IconList_Icon_VerticalSpacing, prefs_IconVerticalSpacing);
		}
		if ((prefs_IconImageSpacing != -1) && (current_IconImageSpacing != prefs_IconImageSpacing))
		{
			D(bug("[IconWindowIconList] IconWindowIconList__HookFunc_ProcessIconListPrefsFunc: Icon Label Image Spacing changed - updating ..\n"));
			options_changed = TRUE;
			SET(self, MUIA_IconList_Icon_ImageSpacing, prefs_IconImageSpacing);
		}
		if ((prefs_LabelTextHorizontalPadding != -1) && (current_LabelTextHorizontalPadding != prefs_LabelTextHorizontalPadding))
		{
			D(bug("[IconWindowIconList] IconWindowIconList__HookFunc_ProcessIconListPrefsFunc: Icon Label Horizontal Padding changed - updating ..\n"));
			options_changed = TRUE;
			SET(self, MUIA_IconList_LabelText_HorizontalPadding, prefs_LabelTextHorizontalPadding);
		}
		if ((prefs_LabelTextVerticalPadding != -1) &&(current_LabelTextVerticalPadding != prefs_LabelTextVerticalPadding))
		{
			D(bug("[IconWindowIconList] IconWindowIconList__HookFunc_ProcessIconListPrefsFunc: Icon Label Vertical Padding changed - updating ..\n"));
			options_changed = TRUE;
			SET(self, MUIA_IconList_LabelText_VerticalPadding, prefs_LabelTextVerticalPadding);
		}
		if ((prefs_LabelTextBorderWidth != -1) && (current_LabelTextBorderWidth != prefs_LabelTextBorderWidth))
		{
			D(bug("[IconWindowIconList] IconWindowIconList__HookFunc_ProcessIconListPrefsFunc: Icon Label Border Width changed - updating ..\n"));
			options_changed = TRUE;
			SET(self, MUIA_IconList_LabelText_BorderWidth, prefs_LabelTextBorderWidth);
		}
		if ((prefs_LabelTextBorderHeight != -1) && (current_LabelTextBorderHeight != prefs_LabelTextBorderHeight))
		{
			D(bug("[IconWindowIconList] IconWindowIconList__HookFunc_ProcessIconListPrefsFunc: Icon Label Border Height changed - updating ..\n"));
			options_changed = TRUE;
			SET(self, MUIA_IconList_LabelText_BorderHeight, prefs_LabelTextBorderHeight);
		}

		if ((options_changed) && !(prefs_Processing))
		{
			D(bug("[IconWindowIconList] IconWindowIconList__HookFunc_ProcessIconListPrefsFunc: IconList Options have changed, causing an update ..\n"));
			DoMethod(self, MUIM_IconList_Update);
		}
		else if (data->iwcd_IconWindow)
		{
			SET(data->iwcd_IconWindow, MUIA_IconWindow_Changed, TRUE);
		}
	}
	AROS_USERFUNC_EXIT
}

AROS_UFH3(
	void, IconWindowIconList__HookFunc_UpdateNetworkPrefsFunc,
	AROS_UFHA(struct Hook *,    hook,   A0),
	AROS_UFHA(APTR *,           obj,    A2),
	AROS_UFHA(APTR,             param,  A1)
)
{
	AROS_USERFUNC_INIT

	/* Get our private data */
	Object *self = ( Object *)obj;
	Object *prefs = NULL;
	Class *CLASS = *( Class **)param;

	SETUP_INST_DATA;

	D(bug("[IconWindowIconList] IconWindowIconList__HookFunc_UpdateNetworkPrefsFunc()\n"));

	GET(_app(self), MUIA_Wanderer_Prefs, &prefs);

	if (prefs)
	{
		BOOL    options_changed = FALSE;
		IPTR	prefs_Processing = 0;

		GET(prefs, MUIA_WandererPrefs_Processing, &prefs_Processing);

		if ((BOOL)XGET(_win(self), MUIA_IconWindow_IsRoot))
		{
			D(bug("[IconWindowIconList] IconWindowIconList__HookFunc_UpdateNetworkPrefsFunc: Setting ROOT view Network options ..\n"));
			ULONG   current_ShowNetwork = 0;

			GET(self, MUIA_IconWindowExt_NetworkBrowser_Show, &current_ShowNetwork);

			D(bug("[IconWindowIconList] IconWindowIconList__HookFunc_UpdateNetworkPrefsFunc: Current = %d\n", current_ShowNetwork));

			ULONG   prefs_ShowNetwork = 0;

			GET(prefs, MUIA_IconWindowExt_NetworkBrowser_Show, &prefs_ShowNetwork);
		
			D(bug("[IconWindowIconList] IconWindowIconList__HookFunc_UpdateNetworkPrefsFunc: Prefs = %d\n", prefs_ShowNetwork));

			if ((BOOL)current_ShowNetwork != (BOOL)prefs_ShowNetwork)
			{
				D(bug("[IconWindowIconList] IconWindowIconList__HookFunc_UpdateNetworkPrefsFunc: ROOT view Network prefs changed - updating ..\n"));
				options_changed = TRUE;
				((struct IconWindowIconVolumeList_DATA *)data)->iwvcd_ShowNetworkBrowser = prefs_ShowNetwork;
			}
		}
		if ((options_changed) && !(prefs_Processing))
		{
			D(bug("[IconWindowIconList] IconWindowIconList__HookFunc_UpdateNetworkPrefsFunc: Network prefs changed, causing an update ..\n"));
			DoMethod(self, MUIM_IconList_Update);
		}
		else if (data->iwcd_IconWindow)
		{
			SET(data->iwcd_IconWindow, MUIA_IconWindow_Changed, TRUE);
		}
	}
	AROS_USERFUNC_EXIT
}

/*** Methods ****************************************************************/

Object *IconWindowIconList__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
	D(bug("[IconWindowIconList] IconWindowIconList__OM_NEW()\n"));
	
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
		data->iwcd_ProcessIconListPrefs_hook.h_Entry = ( HOOKFUNC )IconWindowIconList__HookFunc_ProcessIconListPrefsFunc;
	}
		
	return self;
}

IPTR IconWindowIconList__OM_SET(Class *CLASS, Object *self, struct opSet *message)
{
	SETUP_INST_DATA;

	struct TagItem *tstate = message->ops_AttrList, *tag;

	while ((tag = NextTagItem((const struct TagItem**)&tstate)) != NULL)
	{
		switch (tag->ti_Tag)
		{
			case MUIA_Background:
			{
				D(bug("[IconWindowIconList] IconWindowIconList__OM_SET: MUIA_Background\n"));
				break;
			}
			case MUIA_IconWindow_Window:
			{
				data->iwcd_IconWindow = tag->ti_Data;
				break;
			}
		}
	}
	return DoSuperMethodA(CLASS, self, (Msg) message);
}

IPTR IconWindowIconList__OM_GET(Class *CLASS, Object *self, struct opGet *message)
{
	SETUP_INST_DATA;
	IPTR *store = message->opg_Storage;
	IPTR  rv    = TRUE;

	switch (message->opg_AttrID)
	{
		default:
			rv = DoSuperMethodA(CLASS, self, (Msg) message);
	}
	
	return rv;
}

IPTR IconWindowIconList__MUIM_Setup
(
	Class *CLASS, Object *self, Msg message
)
{
	SETUP_INST_DATA;

	if (!DoSuperMethodA(CLASS, self, message)) return FALSE;

	Object *prefs = NULL;

	GET(_app(self), MUIA_Wanderer_Prefs, &prefs);

	if (prefs)
	{
		GET(_win(self), MUIA_IconWindow_BackgroundAttrib, &data->iwcd_ViewPrefs_ID);
D(bug("[IconWindowIconList] IconWindowIconList__MUIM_Setup: Window Background = '%s'\n", data->iwcd_ViewPrefs_ID));
		data->iwcd_ViewPrefs_NotificationObject = DoMethod(prefs,
																MUIM_WandererPrefs_ViewSettings_GetNotifyObject,
																data->iwcd_ViewPrefs_ID);

D(bug("[IconWindowIconList] IconWindowIconList__MUIM_Setup: Background Notification Obj @ 0x%p\n", data->iwcd_ViewPrefs_NotificationObject));

		/* Set our initial options */
		IPTR    prefs_ListMode = 0,
				prefs_TextMode = 0,
				prefs_TextMaxLen = 0,
				prefs_MultiLine = 0,
				prefs_MultiLineOnFocus = 0,
				prefs_IconHorizontalSpacing = 0,
				prefs_IconVerticalSpacing = 0,
				prefs_IconImageSpacing = 0,
				prefs_LabelTextHorizontalPadding = 0,
				prefs_LabelTextVerticalPadding = 0,
				prefs_LabelTextBorderWidth = 0,
				prefs_LabelTextBorderHeight = 0,
				prefs_Processing = 0;

		prefs_ListMode = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_IconListMode);
		prefs_TextMode = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_LabelText_Mode);
		prefs_TextMaxLen = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_LabelText_MaxLineLen);

		prefs_MultiLine = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_LabelText_MultiLine);
		prefs_MultiLineOnFocus = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_LabelText_MultiLineOnFocus);
		prefs_IconHorizontalSpacing = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_Icon_HorizontalSpacing);
		prefs_IconVerticalSpacing = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_Icon_VerticalSpacing);
		prefs_IconImageSpacing = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_Icon_ImageSpacing);
		prefs_LabelTextHorizontalPadding = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_LabelText_HorizontalPadding);
		prefs_LabelTextVerticalPadding = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_LabelText_VerticalPadding);
		prefs_LabelTextBorderWidth = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_LabelText_BorderWidth);
		prefs_LabelTextBorderHeight = DoMethod(prefs, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwcd_ViewPrefs_ID, MUIA_IconList_LabelText_BorderHeight);

		if (prefs_ListMode != (IPTR)-1) SET(self, MUIA_IconList_IconListMode, prefs_ListMode);
		if (prefs_TextMode != (IPTR)-1) SET(self, MUIA_IconList_LabelText_Mode, prefs_TextMode);
		if (prefs_TextMaxLen != (IPTR)-1) SET(self, MUIA_IconList_LabelText_MaxLineLen, prefs_TextMaxLen);
		if (prefs_MultiLine != (IPTR)-1) SET(self, MUIA_IconList_LabelText_MultiLine, prefs_MultiLine);
		if (prefs_MultiLineOnFocus != (IPTR)-1) SET(self, MUIA_IconList_LabelText_MultiLineOnFocus, prefs_MultiLineOnFocus);
		if (prefs_IconHorizontalSpacing != (IPTR)-1) SET(self, MUIA_IconList_Icon_HorizontalSpacing, prefs_IconHorizontalSpacing);
		if (prefs_IconVerticalSpacing != (IPTR)-1) SET(self, MUIA_IconList_Icon_VerticalSpacing, prefs_IconVerticalSpacing);
		if (prefs_IconImageSpacing != (IPTR)-1) SET(self, MUIA_IconList_Icon_ImageSpacing, prefs_IconImageSpacing);
		if (prefs_LabelTextHorizontalPadding != (IPTR)-1) SET(self, MUIA_IconList_LabelText_HorizontalPadding, prefs_LabelTextHorizontalPadding);
		if (prefs_LabelTextVerticalPadding != (IPTR)-1) SET(self, MUIA_IconList_LabelText_VerticalPadding, prefs_LabelTextVerticalPadding);
		if (prefs_LabelTextBorderWidth != (IPTR)-1) SET(self, MUIA_IconList_LabelText_BorderWidth, prefs_LabelTextBorderWidth);
		if (prefs_LabelTextBorderHeight != (IPTR)-1) SET(self, MUIA_IconList_LabelText_BorderHeight, prefs_LabelTextBorderHeight);

		/* Configure notifications incase they get updated =) */
		DoMethod
		(
			data->iwcd_ViewPrefs_NotificationObject, MUIM_Notify, MUIA_IconList_IconListMode, MUIV_EveryTime,
			(IPTR) self, 3, 
			MUIM_CallHook, &data->iwcd_ProcessIconListPrefs_hook, (IPTR)CLASS
		);

		DoMethod
		(
			data->iwcd_ViewPrefs_NotificationObject, MUIM_Notify, MUIA_IconList_LabelText_Mode, MUIV_EveryTime,
			(IPTR) self, 3, 
			MUIM_CallHook, &data->iwcd_ProcessIconListPrefs_hook, (IPTR)CLASS
		);

		DoMethod
		(
			data->iwcd_ViewPrefs_NotificationObject, MUIM_Notify, MUIA_IconList_LabelText_MaxLineLen, MUIV_EveryTime,
			(IPTR) self, 3, 
			MUIM_CallHook, &data->iwcd_ProcessIconListPrefs_hook, (IPTR)CLASS
		);

		DoMethod
		(
			data->iwcd_ViewPrefs_NotificationObject, MUIM_Notify, MUIA_IconList_LabelText_MultiLine, MUIV_EveryTime,
			(IPTR) self, 3, 
			MUIM_CallHook, &data->iwcd_ProcessIconListPrefs_hook, (IPTR)CLASS
		);

		DoMethod
		(
			data->iwcd_ViewPrefs_NotificationObject, MUIM_Notify, MUIA_IconList_LabelText_MultiLineOnFocus, MUIV_EveryTime,
			(IPTR) self, 3, 
			MUIM_CallHook, &data->iwcd_ProcessIconListPrefs_hook, (IPTR)CLASS
		);

		DoMethod
		(
			data->iwcd_ViewPrefs_NotificationObject, MUIM_Notify, MUIA_IconList_Icon_HorizontalSpacing, MUIV_EveryTime,
			(IPTR) self, 3, 
			MUIM_CallHook, &data->iwcd_ProcessIconListPrefs_hook, (IPTR)CLASS
		);

		DoMethod
		(
			data->iwcd_ViewPrefs_NotificationObject, MUIM_Notify, MUIA_IconList_Icon_VerticalSpacing, MUIV_EveryTime,
			(IPTR) self, 3, 
			MUIM_CallHook, &data->iwcd_ProcessIconListPrefs_hook, (IPTR)CLASS
		);

		DoMethod
		(
			data->iwcd_ViewPrefs_NotificationObject, MUIM_Notify, MUIA_IconList_Icon_ImageSpacing, MUIV_EveryTime,
			(IPTR) self, 3, 
			MUIM_CallHook, &data->iwcd_ProcessIconListPrefs_hook, (IPTR)CLASS
		);

		DoMethod
		(
			data->iwcd_ViewPrefs_NotificationObject, MUIM_Notify, MUIA_IconList_LabelText_HorizontalPadding, MUIV_EveryTime,
			(IPTR) self, 3, 
			MUIM_CallHook, &data->iwcd_ProcessIconListPrefs_hook, (IPTR)CLASS
		);

		DoMethod
		(
			data->iwcd_ViewPrefs_NotificationObject, MUIM_Notify, MUIA_IconList_LabelText_VerticalPadding, MUIV_EveryTime,
			(IPTR) self, 3, 
			MUIM_CallHook, &data->iwcd_ProcessIconListPrefs_hook, (IPTR)CLASS
		);

		DoMethod
		(
			data->iwcd_ViewPrefs_NotificationObject, MUIM_Notify, MUIA_IconList_LabelText_BorderWidth, MUIV_EveryTime,
			(IPTR) self, 3, 
			MUIM_CallHook, &data->iwcd_ProcessIconListPrefs_hook, (IPTR)CLASS
		);

		DoMethod
		(
			data->iwcd_ViewPrefs_NotificationObject, MUIM_Notify, MUIA_IconList_LabelText_BorderHeight, MUIV_EveryTime,
			(IPTR) self, 3, 
			MUIM_CallHook, &data->iwcd_ProcessIconListPrefs_hook, (IPTR)CLASS
		);
	}
	
	if ((BOOL)XGET(_win(self), MUIA_IconWindow_IsRoot))
	{
		if (prefs)
		{
			((struct IconWindowIconVolumeList_DATA *)data)->iwvcd_UpdateNetworkPrefs_hook.h_Entry = ( HOOKFUNC )IconWindowIconList__HookFunc_UpdateNetworkPrefsFunc;
			
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

	D(bug("[IconWindowIconList] IconWindowIconList__MUIM_Window_Setup: Setup complete!\n"));
	
	return TRUE;
}

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

	return DoSuperMethodA(CLASS, self, message);
}

IPTR IconWindowIconList__MUIM_HandleEvent
(
	Class *CLASS, Object *self, struct MUIP_HandleEvent *message
)
{
	SETUP_INST_DATA;

	D(bug("[IconWindowIconList] IconWindowIconList__MUIM_HandleEvent()\n"));
	struct IntuiMessage *imsg = message->imsg;

	if(imsg->Class == IDCMP_DISKINSERTED) 
	{
		D(bug("[IconWindowIconList] IconWindowIconList__MUIM_HandleEvent: IDCMP_DISKINSERTED\n"));
		DoMethod(self, MUIM_IconList_Update);
		return(MUI_EventHandlerRC_Eat);
	}
	else if (imsg->Class == IDCMP_DISKREMOVED) 
	{
		D(bug("[IconWindowIconList] IconWindowIconList__MUIM_HandleEvent: IDCMP_DISKREMOVED\n"));
		DoMethod(self, MUIM_IconList_Update);
		return(MUI_EventHandlerRC_Eat);
	}
	return 0;
}

IPTR IconWindowIconList__MUIM_DrawBackground
(
	Class *CLASS, Object *self, struct MUIP_DrawBackground *message
)
{
	SETUP_INST_DATA;

	IPTR 				retVal = (IPTR)TRUE;
	IPTR                clip = NULL, adjust_left = 0, adjust_top = 0;

	D(bug("[IconWindow] IconWindowIconList__MUIM_DrawBackground()\n"));

	if ((iconwindow_BackFill_Active == NULL) ||
		(data->iwcd_IconWindow == NULL))
	{
		D(bug("[IconWindow] IconWindowIconList__MUIM_DrawBackground: No Backfill support/Window not set .. causing parent class to render\n"));
		goto iwc_ParentBackground;
	}

	struct RastPort     		  *DrawBackGround_RastPort = _rp(self);
	struct IconWindowBackFillMsg  DrawBackGround_BackFillMsg;

	DrawBackGround_BackFillMsg.Layer = DrawBackGround_RastPort->Layer;

	DrawBackGround_BackFillMsg.AreaBounds.MinX = _mleft(self);
	DrawBackGround_BackFillMsg.AreaBounds.MinY = _mtop(self);
	DrawBackGround_BackFillMsg.AreaBounds.MaxX = (_mleft(self) + _mwidth(self));
	DrawBackGround_BackFillMsg.AreaBounds.MaxY = (_mtop(self) + _mheight(self));

	DrawBackGround_BackFillMsg.DrawBounds.MinX = message->left;
	DrawBackGround_BackFillMsg.DrawBounds.MinY = message->top;
	DrawBackGround_BackFillMsg.DrawBounds.MaxX = (message->left + message->width);
	DrawBackGround_BackFillMsg.DrawBounds.MaxY = (message->top + message->height);

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

	clip = MUI_AddClipping(muiRenderInfo(self), message->left, message->top, message->width, message->height);

	message->width = _mwidth(self);
	message->height = _mheight(self);
	message->left = _mleft(self);
	message->top = _mtop(self);

	retVal = DoSuperMethodA(CLASS, self, (Msg) message);

	MUI_RemoveClipping(muiRenderInfo(self),clip);

	return retVal;
}

IPTR IconWindowIconList__MUIM_IconList_Update
(
	Class *CLASS, Object *self, struct MUIP_IconList_Update *message
)
{
	SETUP_INST_DATA;

	IPTR 				retVal = (IPTR)TRUE;

	if ((BOOL)XGET(_win(self), MUIA_IconWindow_IsRoot))
	{
		D(bug("[IconWindowIconList] IconWindowIconList__MUIM_IconList_Update: (ROOT WINDOW) Causing parent to update\n"));
		retVal = DoSuperMethodA(CLASS, self, (Msg) message);

		D(bug("[IconWindowIconList] IconWindowIconList__MUIM_IconList_Update: Check if we should show NetworkBrowser Icon ..\n"));

		Object *prefs = NULL;
		BOOL    sort_list = FALSE;

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
					if (this_entry = DoMethod(self, MUIM_IconList_CreateEntry, (IPTR)"?wanderer.networkbrowse?", (IPTR)"Network Access..", (IPTR)NULL, (IPTR)_nb_dob))
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
				if (GetVar("SYS/UserFilesLocation", __icwc_intern_TxtBuff, TXTBUFF_LEN, GVF_GLOBAL_ONLY) != -1)
				{
					char * userfiles_path = NULL;

					D(bug("[IconWindowIconList] IconWindowIconList__MUIM_IconList_Update: SYS/UserFilesLocation = '%s'\n", __icwc_intern_TxtBuff));

					if ((userfiles_path = AllocVec(strlen(__icwc_intern_TxtBuff) + 1, MEMF_CLEAR|MEMF_PUBLIC)) != NULL)
					{
						struct DiskObject    *_nb_dob = NULL;

						volumel_data->iwvcd_UserFolderPath = userfiles_path;

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
							if (this_entry = DoMethod(self, MUIM_IconList_CreateEntry, userfiles_path, (IPTR)"User Files..", (IPTR)NULL, (IPTR)_nb_dob))
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

/*** Setup ******************************************************************/
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
