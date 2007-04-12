/*
    Copyright  2004-2006, The AROS Development Team. All rights reserved.
    $Id: iconwindowiconlist.c 25432 2007-03-14 18:05:52Z NicJA $
*/

#define MUIMASTER_YES_INLINE_STDARG

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
};

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct IconWindowIconList_DATA *data = INST_DATA(CLASS, self)

#define IconWindowIconDrawerList_DATA IconWindowIconList_DATA
#define IconWindowIconVolumeList_DATA IconWindowIconList_DATA

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

		ULONG   current_ListMode = 0,
                current_TextMode = 0,
                current_TextMaxLen = 0;

		GET(self, MUIA_IconList_ListMode, &current_ListMode);
		GET(self, MUIA_IconList_TextMode, &current_TextMode);
		GET(self, MUIA_IconList_TextMaxLen, &current_TextMaxLen);

		ULONG   prefs_ListMode = 0,
                prefs_TextMode = 0,
                prefs_TextMaxLen = 0;

		GET(prefs, MUIA_WandererPrefs_Icon_ListMode, &prefs_ListMode);
		GET(prefs, MUIA_WandererPrefs_Icon_TextMode, &prefs_TextMode);
		GET(prefs, MUIA_WandererPrefs_Icon_TextMaxLen, &prefs_TextMaxLen);		

		if (current_ListMode != prefs_ListMode)
		{
D(bug("[IconWindowIconList] IconWindowIconList__HookFunc_ProcessIconListPrefsFunc: IconList ListMode changed - updating ..\n"));
			options_changed = TRUE;
			SET(self, MUIA_IconList_ListMode, prefs_ListMode);
		}
		if (current_TextMode != prefs_TextMode)
		{
D(bug("[IconWindowIconList] IconWindowIconList__HookFunc_ProcessIconListPrefsFunc: IconList TextRenderMode changed - updating ..\n"));
			options_changed = TRUE;
			SET(self, MUIA_IconList_TextMode, prefs_TextMode);
		}
		if (current_TextMaxLen != prefs_TextMaxLen)
		{
D(bug("[IconWindowIconList] IconWindowIconList__HookFunc_ProcessIconListPrefsFunc: IconList Max Text Length changed - updating ..\n"));
			options_changed = TRUE;
			SET(self, MUIA_IconList_TextMaxLen, prefs_TextMaxLen);
		}

		if (options_changed)
		{
D(bug("[IconWindowIconList] IconWindowIconList__HookFunc_ProcessIconListPrefsFunc: IconList Options have changed, causing an update ..\n"));
			DoMethod(self, MUIM_IconList_Update);
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
        TAG_MORE, (IPTR) message->ops_AttrList
    );
    
    if (self != NULL)
    {
        SETUP_INST_DATA;
D(bug("[IconWindowIconList] IconWindowIconList__OM_NEW: SELF = %x\n", self));
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
		/* Set our initial options */
		SET(self, MUIA_IconList_ListMode, XGET(prefs, MUIA_WandererPrefs_Icon_ListMode));
		SET(self, MUIA_IconList_TextMode, XGET(prefs, MUIA_WandererPrefs_Icon_TextMode));
		SET(self, MUIA_IconList_TextMaxLen, XGET(prefs, MUIA_WandererPrefs_Icon_TextMaxLen));

		/* Configure notifications incase they get updated =) */
		DoMethod
		(
			prefs, MUIM_Notify, MUIA_WandererPrefs_Icon_ListMode, MUIV_EveryTime,
			(IPTR) self, 3, 
			MUIM_CallHook, &data->iwcd_ProcessIconListPrefs_hook, (IPTR)CLASS
		);

		DoMethod
		(
			prefs, MUIM_Notify, MUIA_WandererPrefs_Icon_TextMode, MUIV_EveryTime,
			(IPTR) self, 3, 
			MUIM_CallHook, &data->iwcd_ProcessIconListPrefs_hook, (IPTR)CLASS
		);

		DoMethod
		(
			prefs, MUIM_Notify, MUIA_WandererPrefs_Icon_TextMaxLen, MUIV_EveryTime,
			(IPTR) self, 3, 
			MUIM_CallHook, &data->iwcd_ProcessIconListPrefs_hook, (IPTR)CLASS
		);

	}
	
	if ((BOOL)XGET(_win(self), MUIA_IconWindow_IsRoot))
	{
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

D(bug("[IconWindowIconList] IconWindowIconList__MUIM_Cleanup()\n"));

	if ((BOOL)XGET(_win(self), MUIA_IconWindow_IsRoot))
	{
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

D(bug("[IconWindow] IconWindowIconList__MUIM_DrawBackground: RastPort @ %x\n", DrawBackGround_RastPort));
	
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
	MUIM_HandleEvent,              Msg
);
