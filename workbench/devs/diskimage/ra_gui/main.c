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
#include <dos/filehandler.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/icon.h>
#include <proto/commodities.h>
#include <proto/intuition.h>
#include <proto/asl.h>
#include <proto/wb.h>
#include <proto/diskimage.h>
#include <proto/expat.h>
#include <proto/screennotify.h>
#include <clib/alib_protos.h>
#include <SDI_compiler.h>
#include <string.h>
#include "rev/DiskImageGUI_rev.h"

#define dbug(x) //Printf x

CONST TEXT USED verstag[] = VERSTAG;
struct Library *ButtonBase;
struct LocaleInfo LocaleInfo;
struct DiskObject *Icon;
struct Library *ScreenNotifyBase;
struct ClassLibrary *TBIBase;
struct ClassLibrary *PNGBase;
struct MsgPort *NotifyPort;
APTR NotifyHandle;
struct FileRequester *FileReq;
PrefsObject *ApplicationPrefs;

struct SignalTaskHook {
	struct Hook hook;
	struct Task *task;
	ULONG sigmask;
};

static ULONG SignalTaskFunc (REG(a0, struct SignalTaskHook *hook),
	REG(a2, APTR unused1), REG(a1, APTR unused2));

int main (int argc, STRPTR *argv) {
	int rc = RETURN_FAIL;
	BYTE DiskChangeSignal = -1;
	struct SignalTaskHook *DiskChangeHook = NULL;
	BYTE ReloadPluginsSignal = -1;
	struct SignalTaskHook *ReloadPluginsHook = NULL;
	struct MsgPort *WBMenuItemPort = NULL;
	struct AppMenuItem *WBMenuItem = NULL;
	ULONG sigs;

	ButtonBase = OpenLibrary("gadgets/button.gadget", 0);
	if (!ButtonBase) {
		goto error;
	}
	
	InitLocaleInfo(SysBase, &LocaleInfo, PROGNAME".catalog");

	Icon = GetProgramIcon();
	if (!Icon) {
		goto error;
	}

	dbug(("RegisterCxBroker\n"));
	if (!RegisterCxBroker()) {
		goto error;
	}

	ScreenNotifyBase = OpenLibrary(SCREENNOTIFY_NAME, SCREENNOTIFY_VERSION);
	TBIBase = (struct ClassLibrary *)OpenLibrary("images/titlebar.image", 0);
	PNGBase = (struct ClassLibrary *)OpenLibrary("images/png.image", 0);

	if (ScreenNotifyBase) {
		NotifyPort = CreateMsgPort();
		if (!NotifyPort) {
			goto error;
		}
		dbug(("NotifyPort: 0x%08lX\n", NotifyPort));
	}

	dbug(("BeginScreenNotify\n"));
	BeginScreenNotify();

	dbug(("AllocAslRequestTags\n"));
	FileReq = AllocAslRequestTags(ASL_FileRequest,
		ASLFR_InitialDrawer,	TTString(Icon, "FILEDIR", ""),
		ASLFR_DoSaveMode,		FALSE,
		ASLFR_DoPatterns,		TRUE,
		ASLFR_InitialPattern,	TTString(Icon, "PATTERN", "#?"),
		TAG_END);
	if (!FileReq) {
		goto error;
	}

	ApplicationPrefs = AllocPrefsDictionary();
	if (!ApplicationPrefs) {
		goto error;
	}
	if (!ReadPrefs(ApplicationPrefs, "ENV:"PROGNAME".a500.org.xml")) {
		ReadPrefs(ApplicationPrefs, "ENVARC:"PROGNAME".a500.org.xml");
	}

	dbug(("InitExtWindowClass\n"));
	if (!InitExtWindowClass()) {
		goto error;
	}
	dbug(("InitExtScrollerClass\n"));
	if (!InitExtScrollerClass()) {
		goto error;
	}
	
	dbug(("OpenDiskImageDevice\n"));
	if (!OpenDiskImageDevice(~0)) {
		goto error;
	}

	DiskChangeSignal = AllocSignal(-1);
	if (DiskChangeSignal == -1) {
		goto error;
	}
	DiskChangeHook = CreateExtHook(sizeof(struct SignalTaskHook), SignalTaskFunc, NULL);
	if (!DiskChangeHook) {
		goto error;
	}
	DiskChangeHook->task = FindTask(NULL);
	DiskChangeHook->sigmask = 1UL << DiskChangeSignal;
	dbug(("AddDiskChangeHook\n"));
	AddDiskChangeHook(&DiskChangeHook->hook, TRUE);

	if (CheckLib(DiskImageBase, 52, 22)) {
		ReloadPluginsSignal = AllocSignal(-1);
		if (ReloadPluginsSignal == -1) {
			goto error;
		}
		ReloadPluginsHook = CreateExtHook(sizeof(struct SignalTaskHook), SignalTaskFunc, NULL);
		if (!DiskChangeHook) {
			goto error;
		}
		ReloadPluginsHook->task = FindTask(NULL);
		ReloadPluginsHook->sigmask = 1UL << ReloadPluginsSignal;
		dbug(("AddReloadPluginsHook\n"));
		AddReloadPluginsHook(&ReloadPluginsHook->hook, TRUE);
	}

	if (TTBoolean(Icon, "WBMENUITEM")) {
		WBMenuItemPort = CreateMsgPort();
		if (!WBMenuItemPort) {
			goto error;
		}
		dbug(("AddAppMenuItemA\n"));
		WBMenuItem = AddAppMenuItemA(0, 0, PROGNAME, WBMenuItemPort, NULL);
		if (!WBMenuItem) {
			goto error;
		}
	}

	if (TTBoolean(Icon, "CX_POPUP")) {
		dbug(("ShowWindow(WID_MAIN)\n"));
		if (!ShowWindow(WID_MAIN)) {
			goto error;
		}
	}

	rc = RETURN_OK;

	dbug(("Entering main loop\n"));
	while (TRUE) {
		sigs = GetGUISignals();
		if (NotifyPort) sigs |= (1UL << NotifyPort->mp_SigBit);
		sigs |= (1UL << DiskChangeSignal);
		sigs |= (1UL << ReloadPluginsSignal);
		sigs |= (1UL << BrokerPort->mp_SigBit);
		if (WBMenuItemPort) sigs |= (1UL << WBMenuItemPort->mp_SigBit);
		sigs = Wait(sigs|SIGBREAKF_CTRL_C);

		if (sigs & SIGBREAKF_CTRL_C) {
			break;
		}

		if (NotifyPort && (sigs & (1UL << NotifyPort->mp_SigBit))) {
			struct ScreenNotifyMessage *msg;
			while (msg = (struct ScreenNotifyMessage *)GetMsg(NotifyPort)) {
				dbug(("snm_Type: 0x%08lX\n", msg->snm_Type));
				dbug(("snm_Value: 0x%08lX\n", msg->snm_Value));
				if (msg->snm_Type == SCREENNOTIFY_TYPE_WORKBENCH) {
					if (msg->snm_Value == FALSE) {
						CleanupGUI();
					}
				}
				ReplyMsg((struct Message *)msg);
			}
		}

		if (sigs & (1UL << DiskChangeSignal)) {
			ScanUnits();
		}

		if (sigs & (1UL << ReloadPluginsSignal)) {
			ScanPlugins();
		}

		if (sigs & (1UL << BrokerPort->mp_SigBit)) {
			CxMsg *msg;
			BOOL quit = FALSE;
			while (msg = (CxMsg *)GetMsg(BrokerPort)) {
				switch (CxMsgType(msg)) {
					case CXM_IEVENT:
						switch (CxMsgID(msg)) {
							case EVT_POPKEY:
								ShowWindow(WID_MAIN);
								break;
						}
						break;
					case CXM_COMMAND:
						switch (CxMsgID(msg)) {
							case CXCMD_DISABLE:
								ActivateCxObj(Broker, FALSE);
								break;
							case CXCMD_ENABLE:
								ActivateCxObj(Broker, TRUE);
								break;
							case CXCMD_APPEAR:
							case CXCMD_UNIQUE:
								ShowWindow(WID_MAIN);
								break;
							case CXCMD_DISAPPEAR:
								HideWindow(WID_MAIN);
								break;
							case CXCMD_KILL:
								quit = TRUE;
								break;
						}
						break;
				}
				ReplyMsg((struct Message *)msg);
			}
			if (quit) {
				break;
			}
		}

		if (WBMenuItemPort && (sigs & (1UL << WBMenuItemPort->mp_SigBit))) {
			struct AppMessage *msg;
			while (msg = (struct AppMessage *)GetMsg(WBMenuItemPort)) {
				if (msg->am_Type == AMTYPE_APPMENUITEM && msg->am_Class == 0) {
					ShowWindow(WID_MAIN);
				}
				ReplyMsg((struct Message *)msg);
			}
		}

		if (Gui.windows[WID_MAIN]) {
			ULONG res, event;
			UWORD code;
			struct Menu *menu;
			struct MenuItem *item;
			ULONG mid;
			BOOL quit = FALSE;
			while ((res = RA_HandleInput(Gui.windows[WID_MAIN], &code)) != WMHI_LASTMSG) {
				switch (res & WMHI_CLASSMASK) {
					case WMHI_CLOSEWINDOW:
						HideWindow(WID_MAIN);
						break;
					case WMHI_ICONIFY:
						IconifyWindow(WID_MAIN);
						break;
					case WMHI_UNICONIFY:
						ShowWindow(WID_MAIN);
						break;
					case WMHI_SNAPSHOT:
						SaveWindowSize(Gui.windows[WID_MAIN], "main_window", TRUE);
						break;
					case WMHI_NEWSIZE:
						UpdateLBVertScroller(WID_MAIN, GID_DRIVELIST, GID_DRIVELISTVPROP);
						break;
					case WMHI_GADGETUP:
						switch (res & WMHI_GADGETMASK) {
							case GID_SPEEDBAR:
								switch (code) {
									case SBID_INSERT:
										InsertDisk();
										break;
									case SBID_EJECT:
										EjectDisk();
										break;
									case SBID_WRITEPROTECT:
										ToggleWriteProtect();
										break;
									case SBID_SETDEVICETYPE:
										SetDeviceType();
										break;
									case SBID_REFRESH:
										ScanUnits();
										break;
								}
								break;
							case GID_DRIVELIST:
								GetAttr(LISTBROWSER_RelEvent, Gui.gadgets[GID_DRIVELIST], &event);
								switch (event) {
									case LBRE_NORMAL:
										UpdateSpeedBar();
										break;
									case LBRE_DOUBLECLICK:
										InsertOrEjectDisk();
										break;
								}
								break;
						}
						break;
					case WMHI_MENUPICK:
						menu = Gui.menustrip;
						while (menu && (item = ItemAddress(menu, code))) {
							mid = (ULONG)GTMENUITEM_USERDATA(item);
							code = item->NextSelect;
							switch (mid) {
								case MID_QUIT:
									quit = TRUE;
									break;
								case MID_HIDE:
									HideWindow(WID_MAIN);
									menu = NULL;
									break;
								case MID_ICONIFY:
									IconifyWindow(WID_MAIN);
									menu = NULL;
									break;
								case MID_SNAPSHOT:
									SaveWindowSize(Gui.windows[WID_MAIN], "main_window", TRUE);
									break;
								case MID_ABOUT:
									AboutRequester();
									break;
								case MID_CHANGETEMPDIR:
									ChangeTempDir();
									break;
								case MID_PLUGINS:
									ShowWindow(WID_PLUGINS);
									break;
								case MID_SAVESETTINGS:
									SaveSettings();
									break;
							}
						}
						break;
				}
			}
			if (quit) {
				break;
			}
		}

		if (Gui.windows[WID_PLUGINS]) {
			ULONG res, event;
			UWORD code;
			while ((res = RA_HandleInput(Gui.windows[WID_PLUGINS], &code)) != WMHI_LASTMSG) {
				switch (res & WMHI_CLASSMASK) {
					case WMHI_CLOSEWINDOW:
						HideWindow(WID_PLUGINS);
						break;
					case WMHI_ICONIFY:
						IconifyWindow(WID_PLUGINS);
						break;
					case WMHI_UNICONIFY:
						ShowWindow(WID_PLUGINS);
						break;
					case WMHI_SNAPSHOT:
						SaveWindowSize(Gui.windows[WID_PLUGINS], "plugins_window", TRUE);
						break;
					case WMHI_NEWSIZE:
						UpdateLBVertScroller(WID_PLUGINS, GID_PLUGINLIST, GID_PLUGINLISTVPROP);
						break;
				}
			}
		}
	}

error:
	dbug(("Exiting\n"));
	CleanupGUI();
	if (WBMenuItemPort) {
		if (WBMenuItem) {
			struct Message *msg;
			RemoveAppMenuItem(WBMenuItem);
			while (msg = GetMsg(WBMenuItemPort)) ReplyMsg(msg);
		}
		DeleteMsgPort(WBMenuItemPort);
	}
	if (ReloadPluginsHook) {
		AddReloadPluginsHook(&ReloadPluginsHook->hook, FALSE);
		FreeExtHook(ReloadPluginsHook);
	}
	FreeSignal(ReloadPluginsSignal);
	if (DiskChangeHook) {
		AddDiskChangeHook(&DiskChangeHook->hook, FALSE);
		FreeExtHook(DiskChangeHook);
	}
	FreeSignal(DiskChangeSignal);
	CloseDiskImageDevice();
	FreeClass(ExtScrollerClass);
	FreeClass(ExtWindowClass);
	FreePrefsObject(ApplicationPrefs);
	FreeAslRequest(FileReq);
	StopScreenNotify();
	DeleteMsgPort(NotifyPort);
	if (PNGBase) CloseLibrary((struct Library *)PNGBase);
	if (TBIBase) CloseLibrary((struct Library *)TBIBase);
	if (ScreenNotifyBase) CloseLibrary(ScreenNotifyBase);
	UnregisterCxBroker();
	FreeDiskObject(Icon);
	FreeLocaleInfo(SysBase, &LocaleInfo);
	if (ButtonBase) CloseLibrary(ButtonBase);

	return rc;
}

static ULONG SignalTaskFunc (REG(a0, struct SignalTaskHook *hook),
	REG(a2, APTR unused1), REG(a1, APTR unused2))
{
	Signal(hook->task, hook->sigmask);
	return 0;
}

static ULONG screen_notify_stop_count = 1;

struct DiskObject *GetProgramIcon (void) {
	struct DiskObject *icon;
	dbug(("GetProgramIcon\n"));
	icon = GetDiskObjectNew("PROGDIR:"PROGNAME);
	if (icon) {
		icon->do_CurrentX = NO_ICON_POSITION;
		icon->do_CurrentY = NO_ICON_POSITION;
	}
	dbug(("Icon: 0x%08lX\n", icon));
	return icon;
}

void BeginScreenNotify (void) {
	if (ScreenNotifyBase) {
		if (NotifyHandle) {
			return;
		}
		if (screen_notify_stop_count > 0) {
			screen_notify_stop_count--;
		}
		if (screen_notify_stop_count == 0) {
			NotifyHandle = AddWorkbenchClient(NotifyPort, 0);
			dbug(("NotifyHandle: 0x%08lX\n", NotifyHandle));
		}
	}
}

void StopScreenNotify (void) {
	if (ScreenNotifyBase) {
		if (NotifyHandle) {
			while (!RemWorkbenchClient(NotifyHandle)) Delay(10);
			NotifyHandle = NULL;
		}
		screen_notify_stop_count++;
	}
}

LONG GetUnitNumber (struct Node *node) {
	LONG *unit_num = NULL;
	GetListBrowserNodeAttrs(node,
		LBNA_Column,				DRIVE_COL_UNIT,
		LBNCA_Integer,				&unit_num,
		TAG_END);
	if (unit_num) {
		return *unit_num;
	}
	return -1;
}

LONG GetSelectedUnit (void) {
	struct Node *node = NULL;
	GetAttr(LISTBROWSER_SelectedNode, Gui.gadgets[GID_DRIVELIST], (Tag *)&node);
	return GetUnitNumber(node);
}

CONST_STRPTR GetSelectedPlugin (void) {
	LONG i;
	struct Node *node;
	CONST_STRPTR name;

	i = -1;
	GetAttr(CHOOSER_Selected, Gui.gadgets[GID_PLUGINCHOOSER], (Tag *)&i);
	if (i <= 0) return NULL;

	node = GetHead(Gui.lists[LID_PLUGINCHOOSER]);
	while (node && i--) node = GetSucc(node);

	name = NULL;
	if (node) {
		GetChooserNodeAttrs(node, CNA_Text, &name, TAG_END);
	}
	return name;
}

static TEXT drive_buffer[1024];
static STRPTR drive_buffer_ptr;

void AddDriveNode (struct List *list, struct Node *n1) {
	struct Node *n2;
	LONG u1, u2;
	u1 = GetUnitNumber(n1);
	n2 = list->lh_TailPred;
	while (n2->ln_Pred) {
		u2 = GetUnitNumber(n2);
		if (u1 >= u2) {
			Insert(list, n1, n2);
			return;
		}
		n2 = n2->ln_Pred;
	}
	AddHead(list, n1);
}

void ScanUnits (void) {
	struct Window *window;
	Object *listbrowser;
	struct List *list;
	struct Node *selected_node;
	LONG selected_unit;
	ULONG sort_column;
	const ULONG dosflags = LDF_DEVICES|LDF_READ;
	struct DosList *dl;
	TEXT devicename[64];
	STRPTR device;
	struct FileSysStartupMsg *fssm;
	BOOL free_fssm = FALSE;
	struct Node *node;
	LONG unit_num;

	if (!Gui.initialised) {
		return;
	}

	window = NULL;
	GetAttr(WINDOW_Window, Gui.windows[WID_MAIN], (Tag *)&window);
	listbrowser = Gui.gadgets[GID_DRIVELIST];
	list = Gui.lists[LID_DRIVELIST];
	
	selected_node = NULL;
	GetAttrs(listbrowser,
		LISTBROWSER_SelectedNode,	&selected_node,
		TAG_END);
	selected_unit = GetUnitNumber(selected_node);

	SetAttrs(listbrowser,
		LISTBROWSER_Labels,			~0,
		TAG_END);
	FreeListBrowserList(list);

	drive_buffer_ptr = drive_buffer;
	selected_node = NULL;
	dl = LockDosList(dosflags);
	while (dl = NextDosEntry(dl, dosflags)) {
		if (!dl->dol_Task || !CheckBPTR(dl->dol_Name)) {
			continue;
		}
		fssm = (struct FileSysStartupMsg *)DoPkt0(dl->dol_Task, ACTION_GET_DISK_FSSM);
		if (fssm) {
			free_fssm = TRUE;
		} else {
			free_fssm = FALSE;
			if (IoErr() == ERROR_ACTION_NOT_KNOWN) {
				fssm = CheckBPTR(dl->dol_misc.dol_handler.dol_Startup);
			}
		}
		if (fssm && CheckBPTR(fssm->fssm_Device)) {
			CopyStringBSTRToC(fssm->fssm_Device, devicename, sizeof(devicename));
			if (!strcmp(FilePart(devicename), "diskimage.device")) {
				LONG *unit_ptr;
				unit_ptr = (LONG *)drive_buffer_ptr;
				*unit_ptr = fssm->fssm_Unit;
				drive_buffer_ptr += sizeof(*unit_ptr);
				CopyStringBSTRToC(dl->dol_Name, devicename, sizeof(devicename));
				node = AllocListBrowserNode(DRIVE_COL_MAX,
					LBNA_Column,		DRIVE_COL_ICON,
					LBNCA_Image,		Gui.images[IID_LIST_DISK],
					LBNA_Column,		DRIVE_COL_UNIT,
					LBNCA_HorizJustify,	LCJ_RIGHT,
					LBNCA_Integer,		unit_ptr,
					LBNA_Column,		DRIVE_COL_DEVICE,
					LBNCA_CopyText,		TRUE,
					LBNCA_Text,			devicename,
					LBNA_Column,		DRIVE_COL_DISKIMAGE,
					LBNCA_Text,			GetString(&LocaleInfo, MSG_NO_DISK),
					TAG_END);
				if (node) {
					AddDriveNode(list, node);
					if (fssm->fssm_Unit == selected_unit) {
						selected_node = node;
					}
				}
			}
		}
		if (free_fssm && fssm) {
			DoPkt1(dl->dol_Task, ACTION_FREE_DISK_FSSM, (LONG)fssm);
		}
	}
	UnLockDosList(dosflags);

	node = GetHead(list);
	while (node) {
		unit_num = GetUnitNumber(node);
		if (unit_num != -1) {
			UBYTE device_type = DG_DIRECT_ACCESS;
			ULONG device_icon = IID_LIST_DISK;
			STRPTR filename = NULL;
			BOOL writeprotect = FALSE;

			UnitControl(unit_num,
				DITAG_GetDeviceType,	&device_type,
				DITAG_GetWriteProtect,	&writeprotect,
				DITAG_GetImageName,		&filename,
				TAG_END);

			if (device_type == DG_CDROM) {
				device_icon = IID_LIST_CDROM;
			}

			SetListBrowserNodeAttrs(node,
				LBNA_Column,		DRIVE_COL_ICON,
				LBNCA_Image,		Gui.images[device_icon],
				LBNA_Column,		DRIVE_COL_WP,
				LBNCA_Image,		writeprotect ? Gui.images[IID_LIST_WRITEPROTECTED] : NULL,
				TAG_END);
			if (filename) {
				SetListBrowserNodeAttrs(node,
					LBNA_Column,		DRIVE_COL_DISKIMAGE,
					LBNCA_CopyText,		TRUE,
					LBNCA_Text,			filename,
					TAG_END);
			}
			FreeVec(filename);
		}
		node = GetSucc(node);
	}

	SetGadgetAttrs(GA(listbrowser), window, NULL,
		LISTBROWSER_Labels,			list,
		LISTBROWSER_SelectedNode,	selected_node,
		TAG_END);

	UpdateSpeedBar();
}

static TEXT plugin_buffer[1024];
static STRPTR plugin_buffer_ptr;

static ULONG ChooserPluginHookFunc (REG(a0, struct Hook *hook),
	REG(a2, struct DiskImagePlugin *plugin),
	REG(a1, struct List *list))
{
	struct Node *node;
	STRPTR name;
	if (!(plugin->Flags & PLUGIN_FLAG_USERCHOICE)) {
		return 0;
	}
	strcpy(name = plugin_buffer_ptr, plugin->Node.ln_Name);
	plugin_buffer_ptr += strlen(name) + 1;
	node = AllocChooserNode(
		CNA_Text,	name,
		TAG_END);
	if (node) {
		AddTail(list, node);
	}
	return 0;
}

static ULONG ListBrowserPluginHookFunc (REG(a0, struct Hook *hook),
	REG(a2, struct DiskImagePlugin *plugin),
	REG(a1, struct List *list))
{
	TEXT buffer[40];
	CONST_STRPTR name;
	LONG *pri_ptr;
	struct Node *node;
	name = plugin->Node.ln_Name;
	pri_ptr = (LONG *)plugin_buffer_ptr;
	*pri_ptr = plugin->Node.ln_Pri;
	plugin_buffer_ptr += sizeof(*pri_ptr);
	if (plugin->Flags & PLUGIN_FLAG_BUILTIN) {
		SNPrintf(buffer, sizeof(buffer), "%s (internal)", name);
		name = buffer;
	}
	node = AllocListBrowserNode(PLUG_COL_MAX,
		LBNA_Column,		PLUG_COL_ICON,
		LBNCA_Image,		Gui.images[IID_LIST_PLUGIN],
		LBNA_Column,		PLUG_COL_PRI,
		LBNCA_HorizJustify,	LCJ_RIGHT,
		LBNCA_Integer,		pri_ptr,
		LBNA_Column,		PLUG_COL_WRITE,
		LBNCA_HorizJustify,	LCJ_CENTER,
		LBNCA_Image,		plugin->plugin_Write ? Gui.images[IID_LIST_CHECKMARK] : NULL,
		LBNA_Column,		PLUG_COL_NAME,
		LBNCA_CopyText,		TRUE,
		LBNCA_Text,			name,
		TAG_END);
	if (node) {
		AddTail(list, node);
	}
	return 0;
}

void ScanPlugins (void) {
	struct Hook *hook;
	struct Window *main_window = NULL;
	struct Window *plugin_window = NULL;
	Object *chooser;
	Object *listbrowser;
	struct List *ch_list, *lb_list;
	struct Node *node, *succ;

	if (!Gui.initialised) {
		return;
	}

	hook = CreateHook(NULL, NULL);
	if (!hook) {
		return;
	}

	GetAttr(WINDOW_Window, Gui.windows[WID_MAIN], (Tag *)&main_window);
	GetAttr(WINDOW_Window, Gui.windows[WID_PLUGINS], (Tag *)&plugin_window);
	chooser = Gui.gadgets[GID_PLUGINCHOOSER];
	listbrowser = Gui.gadgets[GID_PLUGINLIST];
	ch_list = Gui.lists[LID_PLUGINCHOOSER];
	lb_list = Gui.lists[LID_PLUGINLIST];

	SetAttrs(chooser,
		CHOOSER_Labels,				~0,
		TAG_END);
	SetAttrs(listbrowser,
		LISTBROWSER_Labels,			~0,
		TAG_END);
	node = GetSucc(GetHead(ch_list));
	while (node) {
		succ = GetSucc(node);
		Remove(node);
		FreeChooserNode(node);
		node = succ;
	}
	FreeListBrowserList(lb_list);

	plugin_buffer_ptr = plugin_buffer;
	hook->h_Entry = ChooserPluginHookFunc;
	hook->h_Data = ch_list;
	DoHookPlugins(hook);
	hook->h_Entry = ListBrowserPluginHookFunc;
	hook->h_Data = lb_list;
	DoHookPlugins(hook);

	SetGadgetAttrs(GA(chooser), main_window, NULL,
		CHOOSER_Labels,				ch_list,
		CHOOSER_Selected,			0,
		TAG_END);
	SetGadgetAttrs(GA(listbrowser), plugin_window, NULL,
		LISTBROWSER_Labels,			lb_list,
		LISTBROWSER_SelectedNode,	NULL,
		TAG_END);

	FreeHook(hook);
}

void UpdateSpeedBar (void) {
	struct Window *window;
	LONG selected_unit;
	struct Node *node;
	ULONG disk_type;
	
	if (!Gui.initialised) {
		return;
	}

	window = NULL;
	GetAttr(WINDOW_Window, Gui.windows[WID_MAIN], (Tag *)&window);

	disk_type = DITYPE_NONE;
	selected_unit = GetSelectedUnit();
	if (selected_unit != -1) {
		if (UnitControl(selected_unit,
			DITAG_DiskImageType,	&disk_type,
			TAG_END))
		{
			selected_unit = -1;
		}
	}

	SetAttrs(Gui.gadgets[GID_SPEEDBAR],
		SPEEDBAR_Buttons,	~0,
		TAG_END);

	node = Gui.lists[LID_SPEEDBAR]->lh_Head;
	while (node->ln_Succ) {
		switch (node->ln_Pri) {
			case SBID_INSERT:
			case SBID_WRITEPROTECT:
			case SBID_SETDEVICETYPE:
				SetSpeedButtonNodeAttrs(node,
					SBNA_Disabled,	selected_unit == -1,
					TAG_END);
				break;
			case SBID_EJECT:
				SetSpeedButtonNodeAttrs(node,
					SBNA_Disabled,	disk_type == DITYPE_NONE,
					TAG_END);
				break;
		}
		node = node->ln_Succ;
	}

	SetGadgetAttrs(GA(Gui.gadgets[GID_SPEEDBAR]), window, NULL,
		SPEEDBAR_Buttons,	Gui.lists[LID_SPEEDBAR],
		TAG_END);
}

void ChangeTempDir (void) {
	struct Window *window;
	STRPTR tmpdir;
	struct FileRequester *freq;

	window = NULL;
	GetAttr(WINDOW_Window, Gui.windows[WID_MAIN], (Tag *)&window);

	tmpdir = GetEnvVar(TEMPDIR_VAR);
	freq = AllocAslRequestTags(ASL_FileRequest,
		ASLFR_InitialDrawer,	tmpdir ? tmpdir : "T:",
		ASLFR_DoSaveMode,		FALSE,
		ASLFR_DrawersOnly,		TRUE,
		TAG_END);
	if (freq) {
		StopScreenNotify();
		SetWindowBusy(~0, TRUE);

		if (AslRequestTags(freq,
			ASLFR_Screen,	Gui.screen,
			TAG_END))
		{
			SetEnvVar(TEMPDIR_VAR, freq->fr_Drawer, FALSE);
		}

		SetWindowBusy(~0, FALSE);
		BeginScreenNotify();

		FreeAslRequest(freq);
	}
	FreeVec(tmpdir);
}

void SaveSettings (void) {
	STRPTR tmpdir;
	tmpdir = GetEnvVar(TEMPDIR_VAR);
	SetEnvVar(TEMPDIR_VAR, tmpdir, TRUE);
	FreeVec(tmpdir);
}
