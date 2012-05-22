/* Copyright 2007-2012 Fredrik Wikstrom. All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
** POSSIBILITY OF SUCH DAMAGE.
*/

#include "diskimagegui.h"
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/commodities.h>
#include <proto/diskimage.h>
#include <clib/alib_protos.h>
#include "rev/DiskImageGUI_rev.h"

struct GUIElements Gui;

BOOL translate_gui = TRUE;

static struct NewMenu main_newmenu[] = {
	{ NM_TITLE, STR_ID(-1),                         NULL, 0, 0, MENU_ID(0)                 },
	{ NM_ITEM,  STR_ID(MSG_PROJECT_ABOUT),          "?",  0, 0, MENU_ID(MID_ABOUT)         },
	{ NM_ITEM,  NM_BARLABEL,                        NULL, 0, 0, MENU_ID(0)                 },
	{ NM_ITEM,  STR_ID(MSG_PROJECT_HIDE),           "H",  0, 0, MENU_ID(MID_HIDE)          },
	{ NM_ITEM,  STR_ID(MSG_PROJECT_ICONIFY),        "I",  0, 0, MENU_ID(MID_ICONIFY)       },
//	{ NM_ITEM,  STR_ID(MSG_PROJECT_SNAPSHOT),       NULL, 0, 0, MENU_ID(MID_SNAPSHOT)      },
	{ NM_ITEM,  NM_BARLABEL,                        NULL, 0, 0, MENU_ID(0)                 },
	{ NM_ITEM,  STR_ID(MSG_PROJECT_QUIT),           "Q",  0, 0, MENU_ID(MID_QUIT)          },
	{ NM_TITLE, STR_ID(MSG_SETTINGS_MENU),          NULL, 0, 0, MENU_ID(0)                 },
	{ NM_ITEM,  STR_ID(MSG_SETTINGS_CHANGETEMPDIR), NULL, 0, 0, MENU_ID(MID_CHANGETEMPDIR) },
	{ NM_ITEM,  STR_ID(MSG_SETTINGS_PLUGINS),       "P",  0, 0, MENU_ID(MID_PLUGINS)       },
	{ NM_ITEM,  NM_BARLABEL,                        NULL, 0, 0, MENU_ID(0)                 },
	{ NM_ITEM,  STR_ID(MSG_SETTINGS_SAVE),          NULL, 0, 0, MENU_ID(MID_SAVESETTINGS)  },
	{ NM_END,   STR_ID(-1),                         NULL, 0, 0, MENU_ID(0)                 }
};

CONST_STRPTR device_types_array[] = {
	STR_ID(MSG_DEVICETYPE_DIRECT_ACCESS),
	STR_ID(MSG_DEVICETYPE_CDROM),
	NULL
};

BOOL CreateGUI (void) {
	STRPTR popkey;
	CONST_STRPTR window_title;
	CONST_STRPTR about_window_title;
    CONST_STRPTR device_window_title;
	CONST_STRPTR about_window_text;
	CxObj *broker;
	struct MsgPort *broker_mp;

	if (Gui.initialised) {
		return TRUE;
	}
	
	Gui.initialised = TRUE;
	
	Gui.pool = CreatePool(MEMF_ANY, 4096, 1024);
	if (!Gui.pool) {
		goto error;
	}
	
	if (translate_gui) {
		translate_gui = FALSE;
		TranslateMenus(&LocaleInfo, main_newmenu);
		main_newmenu[0].nm_Label = PROGNAME;
		TranslateArray(&LocaleInfo, device_types_array);
	}
	
	popkey = TTString(Icon, "CX_POPKEY", NULL);
	if (!popkey || !TrimStr(popkey)[0]) {
		popkey = "none";
	}
	window_title = ASPrintfPooled(Gui.pool, GetString(&LocaleInfo, MSG_MAIN_WND), PROGNAME, popkey);
	about_window_title = ASPrintfPooled(Gui.pool, GetString(&LocaleInfo, MSG_ABOUT_WND), PROGNAME);
    device_window_title = ASPrintfPooled(Gui.pool, GetString(&LocaleInfo, MSG_SETDEVICETYPE_WND), PROGNAME);
	about_window_text = ASPrintfPooled(Gui.pool, GetString(&LocaleInfo, MSG_ABOUT_REQ),
		DiskImageBase->lib_Node.ln_Name, (LONG)DiskImageBase->lib_Version,
		(LONG)DiskImageBase->lib_Revision, PROGNAME, (LONG)VERSION, (LONG)REVISION);
	if (!window_title || !about_window_title || !about_window_text) {
		goto error;
	}
	
	Gui.app = ApplicationObject,
		MUIA_Application_Title,					PROGNAME,
        MUIA_Application_Description,           GetString(&LocaleInfo, MSG_APPDESCRIPTION),
		MUIA_Application_Version,				&verstag[1],
		MUIA_Application_SingleTask,			TRUE,
		MUIA_Application_BrokerPri,				TTInteger(Icon, "CX_PRIORITY", 0),
		MUIA_Application_BrokerHook,			&BrokerHook,
		MUIA_Application_DiskObject,			Icon,
		SubWindow,								Gui.wnd[WID_MAIN] = WindowObject,
			MUIA_Window_ID,						MAKE_ID('M','A','I','N'),
			MUIA_Window_Title,					window_title,
			MUIA_Window_Width,					320,
			MUIA_Window_Height,					200,
			MUIA_Window_Menustrip,				MUI_MakeObject(MUIO_MenustripNM, main_newmenu, 0),
			WindowContents,						VGroup,
				Child,							HGroup,
					GroupFrame,
					Child,						Gui.gad[GID_INSERT] = MakeImageButton(
												"tapeinsert",
												GetString(&LocaleInfo, MSG_INSERT_GAD),
												TRUE),
					Child,						Gui.gad[GID_EJECT] = MakeImageButton(
												"tapeeject",
												GetString(&LocaleInfo, MSG_EJECT_GAD),
												TRUE),
					Child,						Gui.gad[GID_WRITEPROTECT] = MakeImageButton(
												"protectdrive",
												GetString(&LocaleInfo, MSG_WRITEPROTECT_GAD),
												TRUE),
					Child,						Gui.gad[GID_SETDEVICETYPE] = MakeImageButton(
												"prefs",
												GetString(&LocaleInfo, MSG_SETDEVICETYPE_GAD),
												TRUE),
					Child,						Gui.gad[GID_REFRESH] = MakeImageButton(
												"refresh",
												GetString(&LocaleInfo, MSG_REFRESH_GAD),
												FALSE),
					Child,						RectangleObject,
					End,
				End,
				Child,							Gui.gad[GID_DRIVELIST] = ListviewObject,
					MUIA_Listview_List,			NewObject(DriveListClass->mcc_Class, NULL,
						InputListFrame,
						MUIA_List_Pool,			Gui.pool,
					TAG_END),
				End,
			End,
		End,
		SubWindow,								Gui.wnd[WID_PLUGINS] = WindowObject,
			MUIA_Window_ID,						MAKE_ID('P','L','U','G'),
			MUIA_Window_Title,					GetString(&LocaleInfo, MSG_PLUGINS_WND),
			MUIA_Window_Width,					320,
			MUIA_Window_Height,					200,
			WindowContents,						VGroup,
				Child,							Gui.gad[GID_PLUGINLIST] = ListviewObject,
					MUIA_Listview_List,			NewObject(PluginListClass->mcc_Class, NULL,
						InputListFrame,
						MUIA_List_Pool,			Gui.pool,
					TAG_END),
				End,
			End,
		End,
		SubWindow,								Gui.wnd[WID_ABOUT] = WindowObject,
			MUIA_Window_Title,					about_window_title,
            MUIA_Window_ID,                     MAKE_ID('A','B','O','U'),
			WindowContents,						VGroup,
				Child,							TextObject,
					NoFrame,
					MUIA_Text_PreParse,			"\33c",
					MUIA_Text_Contents,			about_window_text,
				End,
				Child,							HGroup,
					Child,						RectangleObject,
					End,
					Child,						Gui.gad[GID_ABOUT_OK] = TextObject,
						ButtonFrame,
						MUIA_HorizWeight,		0,
						MUIA_InputMode,			MUIV_InputMode_RelVerify,
						MUIA_Text_PreParse,		"\33c",
						MUIA_Text_Contents,		GetString(&LocaleInfo, MSG_OK_GAD),
					End,
					Child,						RectangleObject,
					End,
				End,
			End,
		End,
		SubWindow,				Gui.wnd[WID_SETDEVICETYPE] = WindowObject,
            MUIA_Window_Title,  device_window_title,
            MUIA_Window_ID,     MAKE_ID('W','D','E','V'),
			WindowContents,		VGroup,
				Child,			HGroup,
					Child,		Label(GetString(&LocaleInfo, MSG_DEVICETYPE_GAD)),
					Child,		Gui.gad[GID_DEVICETYPE] =
								MUI_MakeObject(MUIO_Cycle, 0, device_types_array),
				End,
				Child,			HGroup,
					Child,		Gui.gad[GID_SETDEVICETYPE_SAVE] =
								SimpleButton(GetString(&LocaleInfo, MSG_SAVE_GAD)),
					Child,		RectangleObject,
					End,
					Child,		Gui.gad[GID_SETDEVICETYPE_CANCEL] =
								SimpleButton(GetString(&LocaleInfo, MSG_CANCEL_GAD)),
				End,
			End,
		End,
	End;
	if (!Gui.app) {
		goto error;
	}
	
	popkey = TTString(Icon, "CX_POPKEY", NULL);
	if (!popkey || !TrimStr(popkey)[0]) {
		popkey = NULL;
	}
	broker = (CxObj *)XGET(Gui.app, MUIA_Application_Broker);
	broker_mp = (struct MsgPort *)XGET(Gui.app, MUIA_Application_BrokerPort);
	if (broker && broker_mp && popkey) {
		CxObj *filter, *sender, *translate;
		filter = CxFilter(popkey);
		sender = CxSender(broker_mp, EVT_POPKEY);
		translate = CxTranslate(NULL);
		AttachCxObj(broker, filter);
		AttachCxObj(filter, sender);
		AttachCxObj(sender, translate);
		if (!broker || !filter || !sender) {
			goto error;
		}
	}
	
	DoMethod(Gui.wnd[WID_MAIN], MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
		Gui.wnd[WID_MAIN], 3, MUIM_Set, MUIA_Window_Open, FALSE);
	DoMethod(Gui.wnd[WID_MAIN], MUIM_Notify, MUIA_Window_MenuAction, MUIV_EveryTime,
		Gui.app, 3, MUIM_CallHook, &MenuHook, MUIV_TriggerValue);
	DoMethod(Gui.gad[GID_INSERT], MUIM_Notify, MUIA_Pressed, FALSE,
		Gui.app, 2, MUIM_CallHook, &InsertHook);
	DoMethod(Gui.gad[GID_EJECT], MUIM_Notify, MUIA_Pressed, FALSE,
		Gui.app, 2, MUIM_CallHook, &EjectHook);
	DoMethod(Gui.gad[GID_WRITEPROTECT], MUIM_Notify, MUIA_Pressed, FALSE,
		Gui.app, 2, MUIM_CallHook, &WriteProtectHook);
	DoMethod(Gui.gad[GID_SETDEVICETYPE], MUIM_Notify, MUIA_Pressed, FALSE,
		Gui.app, 2, MUIM_CallHook, &SetDeviceTypeHook);
	DoMethod(Gui.gad[GID_REFRESH], MUIM_Notify, MUIA_Pressed, FALSE,
		Gui.app, 4, MUIM_CallHook, &SignalHook, FindTask(NULL), (1UL << DiskChangeSignal));
	DoMethod(Gui.gad[GID_DRIVELIST], MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
		Gui.app, 2, MUIM_CallHook, &DriveList_ActiveHook);
	DoMethod(Gui.gad[GID_DRIVELIST], MUIM_Notify, MUIA_Listview_DoubleClick, TRUE,
		Gui.app, 2, MUIM_CallHook, &DriveList_DoubleClickHook);
	DoMethod(Gui.wnd[WID_PLUGINS], MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
		Gui.wnd[WID_PLUGINS], 3, MUIM_Set, MUIA_Window_Open, FALSE);
	DoMethod(Gui.wnd[WID_ABOUT], MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
		Gui.wnd[WID_ABOUT], 3, MUIM_Set, MUIA_Window_Open, FALSE);
	DoMethod(Gui.gad[GID_ABOUT_OK], MUIM_Notify, MUIA_Pressed, FALSE,
		Gui.wnd[WID_ABOUT], 3, MUIM_Set, MUIA_Window_Open, FALSE);
	DoMethod(Gui.wnd[WID_SETDEVICETYPE], MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
		Gui.wnd[WID_SETDEVICETYPE], 3, MUIM_Set, MUIA_Window_Open, FALSE);
	DoMethod(Gui.gad[GID_SETDEVICETYPE_SAVE], MUIM_Notify, MUIA_Pressed, FALSE,
		Gui.app, 2, MUIM_CallHook, &DoSetDeviceTypeHook);
	DoMethod(Gui.gad[GID_SETDEVICETYPE_CANCEL], MUIM_Notify, MUIA_Pressed, FALSE,
		Gui.wnd[WID_SETDEVICETYPE], 3, MUIM_Set, MUIA_Window_Open, FALSE);
    DoMethod(Gui.app, MUIM_Notify, MUIA_Application_DoubleStart, TRUE,
        Gui.app, 3, MUIM_Set, MUIA_Application_Iconified, FALSE);
    DoMethod(Gui.app, MUIM_Notify, MUIA_Application_DoubleStart, TRUE,
        Gui.wnd[WID_MAIN], 3, MUIM_Set, MUIA_Window_Open, TRUE);

	return TRUE;
error:
	CleanupGUI();
	
	return FALSE;
}

void CleanupGUI (void) {
	if (Gui.initialised) {
		if (Gui.app) {
			MUI_DisposeObject(Gui.app);
			Gui.app = NULL;
		}
		if (Gui.pool) {
			DeletePool(Gui.pool);
			Gui.pool = NULL;
		}
		Gui.initialised = FALSE;
	}
}

void SetWindowBusy (ULONG wnd_id, ULONG busy) {
	if (Gui.initialised) {
		if (wnd_id == -1UL) {
			SetWindowBusy(WID_MAIN, busy);
			SetWindowBusy(WID_PLUGINS, busy);
			SetWindowBusy(WID_ABOUT, busy);
		} else {
			set(Gui.wnd[wnd_id], MUIA_Window_Sleep, busy);
		}
	}
}
