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
#include <intuition/icclass.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/diskimage.h>
#include <clib/alib_protos.h>

#define dbug(x) //Printf x

extern struct ClassLibrary *TBIBase;

static CONST CONST_STRPTR image_names[IID_MAX] = {
	"tapeinsert",
	"tapeeject",
	"protectdrive",
	"prefs",
	"refresh",
	"list_crypt",
	"list_checkmark",
	"list_plugin",
	"list_disk",
	"list_cd"
};

#ifdef FALLBACK_IMAGES
#include "images/insert.c"
#include "images/eject.c"
#include "images/writeprotect.c"
#include "images/prefs.c"
#include "images/refresh.c"
#include "images/list_writeprotected.c"
#include "images/list_checkmark.c"
#include "images/list_plugin.c"
#include "images/list_disk.c"
#include "images/list_cd.c"

static CONST APTR fallback_images[IID_MAX] = {
	&insert,
	&eject,
	&writeprotect,
	&prefs,
	&refresh,
	&list_writeprotected,
	&list_checkmark,
	&list_plugin,
	&list_disk,
	&list_cd
};
#endif

struct GUIElements Gui;

static BOOL translate_gui = TRUE;

static struct NewMenu main_newmenu[] = {
	{ NM_TITLE, STR_ID(-1),                         NULL, 0, 0, MENU_ID(0)                 },
	{ NM_ITEM,  STR_ID(MSG_PROJECT_ABOUT),          "?",  0, 0, MENU_ID(MID_ABOUT)         },
	{ NM_ITEM,  NM_BARLABEL,                        NULL, 0, 0, MENU_ID(0)                 },
	{ NM_ITEM,  STR_ID(MSG_PROJECT_HIDE),           "H",  0, 0, MENU_ID(MID_HIDE)          },
	{ NM_ITEM,  STR_ID(MSG_PROJECT_ICONIFY),        "I",  0, 0, MENU_ID(MID_ICONIFY)       },
	{ NM_ITEM,  NM_BARLABEL,                        NULL, 0, 0, MENU_ID(0)                 },
	{ NM_ITEM,  STR_ID(MSG_PROJECT_QUIT),           "Q",  0, 0, MENU_ID(MID_QUIT)          },
	{ NM_TITLE, STR_ID(MSG_SETTINGS_MENU),          NULL, 0, 0, MENU_ID(0)                 },
	{ NM_ITEM,  STR_ID(MSG_SETTINGS_CHANGETEMPDIR), NULL, 0, 0, MENU_ID(MID_CHANGETEMPDIR) },
	{ NM_ITEM,  STR_ID(MSG_SETTINGS_PLUGINS),       "P",  0, 0, MENU_ID(MID_PLUGINS)       },
	{ NM_ITEM,  NM_BARLABEL,                        NULL, 0, 0, MENU_ID(0)                 },
	{ NM_ITEM,  STR_ID(MSG_SETTINGS_SAVE),          NULL, 0, 0, MENU_ID(MID_SAVESETTINGS)  },
	{ NM_END,   STR_ID(-1),                         NULL, 0, 0, MENU_ID(0)                 }
};

static struct NewMenu main_newmenu_notbiclass[] = {
	{ NM_TITLE, STR_ID(-1),                         NULL, 0, 0, MENU_ID(0)                 },
	{ NM_ITEM,  STR_ID(MSG_PROJECT_ABOUT),          "?",  0, 0, MENU_ID(MID_ABOUT)         },
	{ NM_ITEM,  NM_BARLABEL,                        NULL, 0, 0, MENU_ID(0)                 },
	{ NM_ITEM,  STR_ID(MSG_PROJECT_HIDE),           "H",  0, 0, MENU_ID(MID_HIDE)          },
	{ NM_ITEM,  STR_ID(MSG_PROJECT_ICONIFY),        "I",  0, 0, MENU_ID(MID_ICONIFY)       },
	{ NM_ITEM,  STR_ID(MSG_PROJECT_SNAPSHOT),       NULL, 0, 0, MENU_ID(MID_SNAPSHOT)      },
	{ NM_ITEM,  NM_BARLABEL,                        NULL, 0, 0, MENU_ID(0)                 },
	{ NM_ITEM,  STR_ID(MSG_PROJECT_QUIT),           "Q",  0, 0, MENU_ID(MID_QUIT)          },
	{ NM_TITLE, STR_ID(MSG_SETTINGS_MENU),          NULL, 0, 0, MENU_ID(0)                 },
	{ NM_ITEM,  STR_ID(MSG_SETTINGS_CHANGETEMPDIR), NULL, 0, 0, MENU_ID(MID_CHANGETEMPDIR) },
	{ NM_ITEM,  STR_ID(MSG_SETTINGS_PLUGINS),       "P",  0, 0, MENU_ID(MID_PLUGINS)       },
	{ NM_ITEM,  NM_BARLABEL,                        NULL, 0, 0, MENU_ID(0)                 },
	{ NM_ITEM,  STR_ID(MSG_SETTINGS_SAVE),          NULL, 0, 0, MENU_ID(MID_SAVESETTINGS)  },
	{ NM_END,   STR_ID(-1),                         NULL, 0, 0, MENU_ID(0)                 }
};

static struct HintInfo main_hintinfo[] = {
	{ GID_SPEEDBAR, SBID_INSERT,        STR_ID(MSG_INSERT_GAD),        0 },
	{ GID_SPEEDBAR, SBID_EJECT,         STR_ID(MSG_EJECT_GAD),         0 },
	{ GID_SPEEDBAR, SBID_WRITEPROTECT,  STR_ID(MSG_WRITEPROTECT_GAD),  0 },
	{ GID_SPEEDBAR, SBID_SETDEVICETYPE, STR_ID(MSG_SETDEVICETYPE_GAD), 0 },
	{ GID_SPEEDBAR, SBID_REFRESH,       STR_ID(MSG_REFRESH_GAD),       0 },
	{ -1,           -1,                 NULL,                          0 }
};

CONST_STRPTR device_types_array[] = {
	STR_ID(MSG_DEVICETYPE_DIRECT_ACCESS),
	STR_ID(MSG_DEVICETYPE_CDROM),
	NULL
};

struct ColumnInfo drivelist_columns[DRIVE_COL_MAX+1];
struct ColumnInfo pluginlist_columns[PLUG_COL_MAX+1];

BOOL SetupGUI (void) {
	ULONG i;
	struct Node *node;
	STRPTR popkey;
	CONST_STRPTR main_window_title;
	CONST_STRPTR *devtype;
	static Tag vertproptag = EXTWINDOW_VertProp;
	static Tag vertobjecttag = EXTWINDOW_VertObject;

	if (Gui.initialised) {
		return TRUE;
	}
	
	Gui.initialised = TRUE;

	if (WindowBase->lib_Version >= 50) {
		vertproptag = WINDOW_VertProp;
		vertobjecttag = WINDOW_VertObject;
	}

	Gui.pool = CreatePool(MEMF_ANY, 4096, 1024);
	if (!Gui.pool) {
		goto error;
	}

	if (translate_gui) {
		translate_gui = FALSE;
		TranslateMenus(&LocaleInfo, main_newmenu);
		main_newmenu[0].nm_Label = PROGNAME;
		TranslateMenus(&LocaleInfo, main_newmenu_notbiclass);
		main_newmenu_notbiclass[0].nm_Label = PROGNAME;
		TranslateHints(&LocaleInfo, main_hintinfo);
		TranslateArray(&LocaleInfo, device_types_array);
	}

	Gui.userport = CreateMsgPort();
	Gui.appport = CreateMsgPort();
	if (!Gui.userport || !Gui.appport) {
		goto error;
	}

	Gui.screen = LockPubScreen(WORKBENCHNAME);
	if (!Gui.screen) {
		goto error;
	}

	ClearMem(&drivelist_columns, sizeof(drivelist_columns));
	drivelist_columns[DRIVE_COL_ICON].ci_Title = "";
	drivelist_columns[DRIVE_COL_UNIT].ci_Title = (STRPTR)GetString(&LocaleInfo, MSG_UNIT_LBL);
	drivelist_columns[DRIVE_COL_DEVICE].ci_Title = (STRPTR)GetString(&LocaleInfo, MSG_DEVICE_LBL);
	drivelist_columns[DRIVE_COL_WP].ci_Title = (STRPTR)GetString(&LocaleInfo, MSG_WRITEPROTECT_LBL);
	drivelist_columns[DRIVE_COL_DISKIMAGE].ci_Title = (STRPTR)GetString(&LocaleInfo, MSG_FILENAME_LBL);
	drivelist_columns[DRIVE_COL_MAX].ci_Width = -1;
	ClearMem(&pluginlist_columns, sizeof(pluginlist_columns));
	pluginlist_columns[PLUG_COL_ICON].ci_Title = "";
	pluginlist_columns[PLUG_COL_PRI].ci_Title = (STRPTR)GetString(&LocaleInfo, MSG_PRIORITY_LBL);
	pluginlist_columns[PLUG_COL_WRITE].ci_Title = (STRPTR)GetString(&LocaleInfo, MSG_WRITESUPPORT_LBL);
	pluginlist_columns[PLUG_COL_NAME].ci_Title = (STRPTR)GetString(&LocaleInfo, MSG_PLUGIN_LBL);
	pluginlist_columns[PLUG_COL_MAX].ci_Width = -1;

	for (i = 0; i < LID_MAX; i++) {
		Gui.lists[i] = CreateList(FALSE);
		if (!Gui.lists[i]) {
			goto error;
		}
	}

	Gui.images[IID_INSERT] = LoadImage(Gui.screen, image_names[IID_INSERT], TRUE, TRUE);
	Gui.images[IID_EJECT] = LoadImage(Gui.screen, image_names[IID_EJECT], TRUE, TRUE);
	Gui.images[IID_WRITEPROTECT] = LoadImage(Gui.screen, image_names[IID_WRITEPROTECT], TRUE, TRUE);
	Gui.images[IID_PREFS] = LoadImage(Gui.screen, image_names[IID_PREFS], TRUE, TRUE);
	Gui.images[IID_REFRESH] = LoadImage(Gui.screen, image_names[IID_REFRESH], TRUE, TRUE);
	Gui.images[IID_LIST_WRITEPROTECTED] = LoadImage(Gui.screen, image_names[IID_LIST_WRITEPROTECTED], FALSE, FALSE);
	Gui.images[IID_LIST_CHECKMARK] = LoadImage(Gui.screen, image_names[IID_LIST_CHECKMARK], FALSE, FALSE);
	Gui.images[IID_LIST_PLUGIN] = LoadImage(Gui.screen, image_names[IID_LIST_PLUGIN], FALSE, FALSE);
	Gui.images[IID_LIST_DISK] = LoadImage(Gui.screen, image_names[IID_LIST_DISK], FALSE, FALSE);
	Gui.images[IID_LIST_CDROM] = LoadImage(Gui.screen, image_names[IID_LIST_CDROM], FALSE, FALSE);
	for (i = 0; i < IID_MAX; i++) {
		if (!Gui.images[i]) {
#ifdef FALLBACK_IMAGES
			Gui.images[i] = fallback_images[i];
			Gui.fallback_image[i] = TRUE;
#else
			ImageNotFoundRequester(image_names[i]);
			goto error;
#endif
		}
	}

	node = AllocSpeedButtonNode(SBID_INSERT,
		SBNA_Disabled,		TRUE,
		SBNA_Image,			Gui.images[IID_INSERT],
		SBNA_Highlight,		SBH_IMAGE,
		TAG_END);
	if (!node) {
		goto error;
	}
	AddTail(Gui.lists[LID_SPEEDBAR], node);
	node = AllocSpeedButtonNode(SBID_EJECT,
		SBNA_Disabled,		TRUE,
		SBNA_Image,			Gui.images[IID_EJECT],
		SBNA_Highlight,		SBH_IMAGE,
		TAG_END);
	if (!node) {
		goto error;
	}
	AddTail(Gui.lists[LID_SPEEDBAR], node);
	node = AllocSpeedButtonNode(SBID_WRITEPROTECT,
		SBNA_Disabled,		TRUE,
		SBNA_Image,			Gui.images[IID_WRITEPROTECT],
		SBNA_Highlight,		SBH_IMAGE,
		TAG_END);
	if (!node) {
		goto error;
	}
	AddTail(Gui.lists[LID_SPEEDBAR], node);
	if (CheckLib(DiskImageBase, 52, 23)) {
		node = AllocSpeedButtonNode(SBID_SETDEVICETYPE,
			SBNA_Disabled,		TRUE,
			SBNA_Image,			Gui.images[IID_PREFS],
			SBNA_Highlight,		SBH_IMAGE,
			TAG_END);
		if (!node) {
			goto error;
		}
		AddTail(Gui.lists[LID_SPEEDBAR], node);
	}
	node = AllocSpeedButtonNode(SBID_REFRESH,
		SBNA_Disabled,		FALSE,
		SBNA_Image,			Gui.images[IID_REFRESH],
		SBNA_Highlight,		SBH_IMAGE,
		TAG_END);
	if (!node) {
		goto error;
	}
	AddTail(Gui.lists[LID_SPEEDBAR], node);

	node = AllocChooserNode(
		CNA_Text,			GetString(&LocaleInfo, MSG_AUTO_LBL),
		TAG_END);
	if (!node) {
		goto error;
	}
	AddHead(Gui.lists[LID_PLUGINCHOOSER], node);
	
	devtype = device_types_array;
	while (*devtype) {
		node = AllocChooserNode(
			CNA_Text,		*devtype++,
			TAG_END);
		if (!node) {
			goto error;
		}
		AddTail(Gui.lists[LID_DEVICETYPECHOOSER], node);
	}

	popkey = TTString(Icon, "CX_POPKEY", NULL);
	if (!popkey || !TrimStr(popkey)[0]) {
		popkey = "none";
	}

	main_window_title = ASPrintfPooled(Gui.pool, GetString(&LocaleInfo, MSG_MAIN_WND), PROGNAME, popkey);
	if (!main_window_title) {
		goto error;
	}

	Gui.visualinfo = GetVisualInfoA(Gui.screen, NULL);
	if (!Gui.visualinfo) {
		goto error;
	}
	if (TBIBase) {
		Gui.menustrip = CreateMenus(main_newmenu, GTMN_FullMenu, TRUE, TAG_END);
	} else {
		Gui.menustrip = CreateMenus(main_newmenu_notbiclass, GTMN_FullMenu, TRUE, TAG_END);
	}
	if (!Gui.menustrip || !LayoutMenus(Gui.menustrip, Gui.visualinfo, GTMN_NewLookMenus, TRUE, TAG_END)) {
		goto error;
	}
	
	Gui.windows[WID_MAIN] = ExtWindowObject,
		WA_Title,				main_window_title,
		WA_Flags,				WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_SIZEGADGET
								|WFLG_NEWLOOKMENUS|WFLG_NOCAREREFRESH|WFLG_ACTIVATE|WFLG_SIZEBRIGHT,
		WA_IDCMP,				IDCMP_CLOSEWINDOW|IDCMP_GADGETUP|IDCMP_MENUPICK|IDCMP_NEWSIZE,
		WA_PubScreen,			Gui.screen,
		WINDOW_Position,		WPOS_CENTERMOUSE,
		WINDOW_SharedPort,		Gui.userport,
		WINDOW_AppPort,			Gui.appport,
		WINDOW_IconifyGadget,	TRUE,
		WINDOW_Icon,			GetProgramIcon(),
		WINDOW_IconTitle,		PROGNAME,
		WINDOW_HintInfo,		main_hintinfo,
		WINDOW_GadgetHelp,		TRUE,
		WINDOW_MenuStrip,		Gui.menustrip,
		vertproptag,			TRUE,
		WINDOW_Layout,			VLayoutObject,
			LAYOUT_SpaceOuter,				TRUE,
			LAYOUT_AddChild,				Gui.gadgets[GID_SPEEDBAR] = SpeedBarObject,
				GA_ID,						GID_SPEEDBAR,
				GA_RelVerify,				TRUE,
				SPEEDBAR_Buttons,			Gui.lists[LID_SPEEDBAR],
			End,
			LAYOUT_AddChild,				Gui.gadgets[GID_DRIVELIST] = ListBrowserObject,
				GA_ID,						GID_DRIVELIST,
				GA_RelVerify,				TRUE,
				LISTBROWSER_ColumnTitles,	TRUE,
				LISTBROWSER_TitleClickable,	TRUE,
				LISTBROWSER_ColumnInfo,		drivelist_columns,
				LISTBROWSER_Labels,			Gui.lists[LID_DRIVELIST],
				LISTBROWSER_ShowSelected,	TRUE,
				LISTBROWSER_AutoFit,		TRUE,
				LISTBROWSER_VerticalProp,	FALSE,
			End,
			LAYOUT_AddChild,				Gui.gadgets[GID_PLUGINCHOOSER] = ChooserObject,
				GA_ID,						GID_PLUGINCHOOSER,
				CHOOSER_Labels,				Gui.lists[LID_PLUGINCHOOSER],
			End,
			CHILD_Label,					LabelObject,
				LABEL_Text,					GetString(&LocaleInfo, MSG_PLUGIN_GAD),
			End,
		End,
		EXTWINDOW_SnapshotGadget,			TRUE,
	End;
	Gui.windows[WID_PLUGINS] = ExtWindowObject,
		WA_Title,				GetString(&LocaleInfo, MSG_PLUGINS_WND),
		WA_Flags,				WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_SIZEGADGET
								|WFLG_NOCAREREFRESH|WFLG_ACTIVATE|WFLG_SIZEBRIGHT,
		WA_IDCMP,				IDCMP_CLOSEWINDOW|IDCMP_GADGETUP|IDCMP_NEWSIZE,
		WA_PubScreen,			Gui.screen,
		WINDOW_Position,		WPOS_CENTERMOUSE,
		WINDOW_SharedPort,		Gui.userport,
		WINDOW_AppPort,			Gui.appport,
		WINDOW_IconifyGadget,	TRUE,
		WINDOW_Icon,			GetProgramIcon(),
		vertproptag,			TRUE,
		WINDOW_Layout,			LayoutObject,
			LAYOUT_SpaceOuter,				TRUE,
			LAYOUT_AddChild,				Gui.gadgets[GID_PLUGINLIST] = ListBrowserObject,
				GA_ID,						GID_PLUGINLIST,
				GA_RelVerify,				TRUE,
				LISTBROWSER_ColumnTitles,	TRUE,
				LISTBROWSER_TitleClickable,	TRUE,
				LISTBROWSER_ColumnInfo,		pluginlist_columns,
				LISTBROWSER_Labels,			Gui.lists[LID_PLUGINLIST],
				LISTBROWSER_ShowSelected,	TRUE,
				LISTBROWSER_AutoFit,		TRUE,
				LISTBROWSER_VerticalProp,	FALSE,
			End,
		End,
		EXTWINDOW_SnapshotGadget,			TRUE,
	End;
	Gui.windows[WID_SETDEVICETYPE] = WindowObject,
		WA_Flags,				WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_NOCAREREFRESH
								|WFLG_ACTIVATE,
		WA_IDCMP,				IDCMP_CLOSEWINDOW|IDCMP_GADGETUP,
		WA_PubScreen,			Gui.screen,
		WINDOW_Position,		WPOS_CENTERMOUSE,
		WINDOW_SharedPort,		Gui.userport,
		WINDOW_Layout,			VLayoutObject,
			LAYOUT_SpaceOuter,		TRUE,
			LAYOUT_AddChild,		Gui.gadgets[GID_DEVICETYPECHOOSER] = ChooserObject,
				GA_ID,				GID_DEVICETYPECHOOSER,
				CHOOSER_Labels,		Gui.lists[LID_DEVICETYPECHOOSER],
				CHOOSER_Selected,	0,
			End,
			CHILD_Label,			LabelObject,
				LABEL_Text,			GetString(&LocaleInfo, MSG_DEVICETYPE_GAD),
			End,
			LAYOUT_AddChild,		HLayoutObject,
				LAYOUT_BevelStyle,	BVS_SBAR_VERT,
				LAYOUT_AddChild,	Gui.gadgets[GID_SETDEVICETYPE_SAVE] = ButtonObject,
					GA_ID,			GID_SETDEVICETYPE_SAVE,
					GA_RelVerify,	TRUE,
					GA_Text,		GetString(&LocaleInfo, MSG_SAVE_GAD),
				End,
				LAYOUT_AddChild,	Gui.gadgets[GID_SETDEVICETYPE_CANCEL] = ButtonObject,
					GA_ID,			GID_SETDEVICETYPE_CANCEL,
					GA_RelVerify,	TRUE,
					GA_Text,		GetString(&LocaleInfo, MSG_CANCEL_GAD),
				End,
			End,
		End,
	End;
	for (i = 0; i < WID_MAX; i++) {
		if (!Gui.windows[i]) {
			goto error;
		}
	}

	{
		static const struct TagItem vprop2lb_map[] = {
			{ PGA_Top,                  LISTBROWSER_VPropTop     },
			{ SCROLLER_Top,             LISTBROWSER_VPropTop     },
			{ TAG_END,                  0                        }
		};
		static const struct TagItem lb2vprop_map[] = {
			{ LISTBROWSER_VPropTotal,   SCROLLER_Total           },
			{ LISTBROWSER_VPropVisible, SCROLLER_Visible         },
			{ LISTBROWSER_VPropTop,     SCROLLER_Top             },
			{ TAG_END,                  0                        }
		};
		GetAttr(vertobjecttag, Gui.windows[WID_MAIN], (Tag *)&Gui.gadgets[GID_DRIVELISTVPROP]);
		GetAttr(vertobjecttag, Gui.windows[WID_PLUGINS], (Tag *)&Gui.gadgets[GID_PLUGINLISTVPROP]);
		SetAttrs(Gui.gadgets[GID_DRIVELISTVPROP],
			ICA_MAP,	vprop2lb_map,
			ICA_TARGET,	Gui.gadgets[GID_DRIVELIST],
			TAG_END);
		SetAttrs(Gui.gadgets[GID_PLUGINLISTVPROP],
			ICA_MAP,	vprop2lb_map,
			ICA_TARGET,	Gui.gadgets[GID_PLUGINLIST],
			TAG_END);
		SetAttrs(Gui.gadgets[GID_DRIVELIST],
			ICA_MAP,	lb2vprop_map,
			ICA_TARGET,	Gui.gadgets[GID_DRIVELISTVPROP],
			TAG_END);
		SetAttrs(Gui.gadgets[GID_PLUGINLIST],
			ICA_MAP,	lb2vprop_map,
			ICA_TARGET,	Gui.gadgets[GID_PLUGINLISTVPROP],
			TAG_END);
	}

	RestoreWindowSize(Gui.windows[WID_MAIN], "main_window");
	RestoreWindowSize(Gui.windows[WID_PLUGINS], "plugins_window");

	ScanUnits();
	ScanPlugins();
	
	return TRUE;

error:
	CleanupGUI();

	return FALSE;
}

void CleanupGUI (void) {
	if (Gui.initialised) {
		ULONG i;
		struct Node *node, *succ;

		Gui.initialised = FALSE;

		for (i = 0; i < GID_MAX; i++) {
			Gui.gadgets[i] = NULL;
		}

		HideWindow(WID_MAIN);
		HideWindow(WID_PLUGINS);

		for (i = 0; i < WID_MAX; i++) {
			DisposeObject(Gui.windows[i]);
			Gui.windows[i] = NULL;
		}
		
		FreeMenus(Gui.menustrip);
		FreeVisualInfo(Gui.visualinfo);

		while (node = RemHead(Gui.lists[LID_DEVICETYPECHOOSER])) {
			FreeChooserNode(node);
		}
		while (node = RemHead(Gui.lists[LID_PLUGINCHOOSER])) {
			FreeChooserNode(node);
		}
		FreeListBrowserList(Gui.lists[LID_PLUGINLIST]);
		FreeListBrowserList(Gui.lists[LID_DRIVELIST]);

		node = GetHead(Gui.lists[LID_SPEEDBAR]);
		while (node) {
			succ = GetSucc(node);
			FreeSpeedButtonNode(node);
			node = succ;
		}

		for (i = 0; i < IID_MAX; i++) {
#ifdef FALLBACK_IMAGES
			if (Gui.fallback_image[i]) {
				Gui.fallback_image[i] = FALSE;
				Gui.images[i] = NULL;
				continue;
			}
#endif
			DisposeObject(Gui.images[i]);
			Gui.images[i] = NULL;
		}

		for (i = 0; i < LID_MAX; i++) {
			DeleteList(Gui.lists[i]);
			Gui.lists[i] = NULL;
		}

		UnlockPubScreen(NULL, Gui.screen);
		Gui.screen = NULL;

		DeleteMsgPort(Gui.appport);
		DeleteMsgPort(Gui.userport);
		Gui.appport = NULL;
		Gui.userport = NULL;

		DeletePool(Gui.pool);
		Gui.pool = NULL;
	}
}

APTR ShowWindow (ULONG window_id) {
	struct Window *window;
	if (!Gui.initialised && !SetupGUI()) {
		return NULL;
	}
	dbug(("RA_OpenWindow\n"));
	window = RA_OpenWindow(Gui.windows[window_id]);
	if (window) {
		switch (window_id) {
			case WID_MAIN:
				UpdateLBVertScroller(WID_MAIN, GID_DRIVELIST, GID_DRIVELISTVPROP);
				break;
			case WID_PLUGINS:
				UpdateLBVertScroller(WID_PLUGINS, GID_PLUGINLIST, GID_PLUGINLISTVPROP);
				break;
		}
	}
	return window;
}

void HideWindow (ULONG window_id) {
	if (Gui.initialised) {
		switch (window_id) {
			case WID_MAIN:
				SaveWindowSize(Gui.windows[WID_MAIN], "main_window", FALSE);
				break;
			case WID_PLUGINS:
				SaveWindowSize(Gui.windows[WID_PLUGINS], "plugins_window", FALSE);
				break;
		}
		RA_CloseWindow(Gui.windows[window_id]);
	}
}

void IconifyWindow (ULONG window_id) {
	if (Gui.initialised) {
		switch (window_id) {
			case WID_MAIN:
				SaveWindowSize(Gui.windows[WID_MAIN], "main_window", FALSE);
				break;
			case WID_PLUGINS:
				SaveWindowSize(Gui.windows[WID_PLUGINS], "plugins_window", FALSE);
				break;
		}
		RA_Iconify(Gui.windows[window_id]);
	}
}

void SetWindowBusy (ULONG window_id, ULONG busy) {
	if (Gui.initialised) {
		if (window_id == -1UL) {
			SetWindowBusy(WID_MAIN, busy);
			SetWindowBusy(WID_PLUGINS, busy);
		} else {
			if (Gui.windows[window_id]) {
				SetAttrs(Gui.windows[window_id],
					WA_BusyPointer, busy,
					TAG_END);
			}
		}
	}
}


ULONG GetGUISignals (void) {
	ULONG sigs = 0;
	if (Gui.initialised) {
		ULONG i;
		ULONG sigmask;
		for (i = 0; i < WID_MAX; i++) {
			sigmask = 0;
			GetAttr(WINDOW_SigMask, Gui.windows[i], &sigmask);
			sigs |= sigmask;
		}
	}
	return sigs;
}

void UpdateLBVertScroller (ULONG wnd_id, ULONG lb_id, ULONG sc_id) {
	if (Gui.initialised) {
		struct Window *window;
		ULONG top, visible, total;
		window = NULL;
		GetAttr(WINDOW_Window, Gui.windows[wnd_id], (Tag *)&window);
		total = top = visible = 0;
		GetAttrs(Gui.gadgets[lb_id],
			LISTBROWSER_VPropTotal,		&total,
			LISTBROWSER_VPropVisible,	&visible,
			LISTBROWSER_VPropTop,		&top,
			TAG_END);
		dbug(("total: %ld\nvisible: %ld\ntop: %ld\n", total, visible, top));
		SetGadgetAttrs(GA(Gui.gadgets[sc_id]), window, NULL,
			SCROLLER_Total,				total,
			SCROLLER_Visible,			visible,
			SCROLLER_Top,				top,
			TAG_END);
	}
}
