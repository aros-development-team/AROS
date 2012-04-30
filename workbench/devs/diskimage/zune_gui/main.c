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
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/icon.h>
#include <proto/commodities.h>
#include <proto/diskimage.h>
#include <clib/alib_protos.h>
#include <string.h>
#include "support.h"
#include <SDI_hook.h>
#include "rev/DiskImageGUI_rev.h"

CONST TEXT USED verstag[] = VERSTAG;

struct LocaleInfo LocaleInfo;
BYTE DiskChangeSignal = -1;
BYTE ReloadPluginsSignal = -1;
struct DiskObject *Icon;
struct FileRequester *FileReq;
struct MUI_CustomClass *DriveListClass;
struct MUI_CustomClass *PluginListClass;

HOOKPROTO(BrokerFunc, IPTR, Object *app, CxMsg *msg);
MakeHook(BrokerHook, BrokerFunc);
HOOKPROTO(MenuFunc, IPTR, Object *app, IPTR *params);
MakeHook(MenuHook, MenuFunc);
HOOKPROTO(SignalFunc, IPTR, APTR unused, IPTR *params);
MakeHook(SignalHook, SignalFunc);

int main (void) {
	IPTR DiskChangeParams[2];
	struct Hook *DiskChangeHook = NULL;
	IPTR ReloadPluginsParams[2];
	struct Hook *ReloadPluginsHook = NULL;
	ULONG sigs = 0;
	
	InitLocaleInfo((struct Library *)SysBase, &LocaleInfo, PROGNAME".catalog");

	DiskChangeSignal = AllocSignal(-1);
	if (DiskChangeSignal == -1) {
		goto error;
	}

	ReloadPluginsSignal = AllocSignal(-1);
	if (ReloadPluginsSignal == -1) {
		goto error;
	}
	
	Icon = GetDiskObjectNew("PROGDIR:"PROGNAME);
	if (!Icon) {
		goto error;
	}
	
	FileReq = MUI_AllocAslRequestTags(ASL_FileRequest,
		ASLFR_SleepWindow,		TRUE,
		ASLFR_InitialDrawer,	TTString(Icon, "FILEDIR", ""),
		ASLFR_DoSaveMode,		FALSE,
		ASLFR_DoPatterns,		TRUE,
		ASLFR_InitialPattern,	TTString(Icon, "PATTERN", "#?"),
		TAG_END);
	if (!FileReq) {
		goto error;
	}
	
	DriveListClass = DriveList_CreateClass();
	if (!DriveListClass) {
		goto error;
	}

	PluginListClass = PluginList_CreateClass();
	if (!PluginListClass) {
		goto error;
	}
		
	if (!OpenDiskImageDevice(~0)) {
		goto error;
	}

	if (!CreateGUI()) {
		goto error;
	}
	
	DiskChangeParams[0] = (IPTR)FindTask(NULL);
	DiskChangeParams[1] = (1UL << DiskChangeSignal);
	DiskChangeHook = CreateHook((HOOKFUNC)SignalFunc, DiskChangeParams);
	if (!DiskChangeHook) {
		goto error;
	}
	AddDiskChangeHook(DiskChangeHook, TRUE);
	
	ReloadPluginsParams[0] = (IPTR)FindTask(NULL);
	ReloadPluginsParams[1] = (1UL << ReloadPluginsSignal);
	ReloadPluginsHook = CreateHook((HOOKFUNC)SignalFunc, ReloadPluginsParams);
	if (!ReloadPluginsHook) {
		goto error;
	}
	AddReloadPluginsHook(ReloadPluginsHook, TRUE);
	
	ScanUnits();
	ScanPlugins();

	if (TTBoolean(Icon, "CX_POPUP")) {
		set(Gui.wnd[WID_MAIN], MUIA_Window_Open, TRUE);
		if (!XGET(Gui.wnd[WID_MAIN], MUIA_Window_Open)) {
			goto error;
		}
		DoMethod(Gui.gad[GID_DRIVELIST], MUIM_List_Redraw, MUIV_List_Redraw_All);
	}
	
	while ((LONG)DoMethod(Gui.app, MUIM_Application_NewInput, (IPTR)&sigs) !=
		MUIV_Application_ReturnID_Quit)
	{
		if (sigs) {
			sigs |= (1UL << DiskChangeSignal);
			sigs |= (1UL << ReloadPluginsSignal);
			sigs = Wait(sigs|SIGBREAKF_CTRL_C);
			
			if (sigs & SIGBREAKF_CTRL_C) {
				break;
			}
			
			if (sigs & (1UL << DiskChangeSignal)) {
				ScanUnits();
			}
			
			if (sigs & (1UL << ReloadPluginsSignal)) {
				ScanPlugins();
			}
		}
	}
	
error:
	CleanupGUI();
	PluginList_FreeClass(PluginListClass);
	DriveList_FreeClass(DriveListClass);
	MUI_FreeAslRequest(FileReq);
	if (ReloadPluginsHook) {
		AddReloadPluginsHook(ReloadPluginsHook, FALSE);
		FreeHook(ReloadPluginsHook);
	}
	if (DiskChangeHook) {
		AddDiskChangeHook(DiskChangeHook, FALSE);
		FreeHook(DiskChangeHook);
	}
	CloseDiskImageDevice();
	FreeDiskObject(Icon);
	FreeSignal(DiskChangeSignal);
	FreeLocaleInfo((struct Library *)SysBase, &LocaleInfo);
	
	return 0;
}

void ScanUnits (void) {
	Object *listview = Gui.gad[GID_DRIVELIST];
	const ULONG dosflags = LDF_DEVICES|LDF_READ;
	struct DosList *dl;
	struct FileSysStartupMsg *fssm;
	TEXT devicename[20];
	int i, num_entries;
	struct DriveEntry **entries, *e;
	ULONG selected_unit = ~0;
	struct DriveEntry *selected_entry = NULL;
	
	DoMethod(listview, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &e);
	if (e) selected_unit = e->unit_num;

	set(listview, MUIA_List_Quiet, TRUE);
		
	DoMethod(listview, MUIM_List_Clear);
	
	num_entries = 0;
	dl = LockDosList(dosflags);
	while ((dl = NextDosEntry(dl, dosflags))) {
		if (CheckBPTR(dl->dol_Name) && (fssm = CheckBPTR(dl->dol_misc.dol_handler.dol_Startup))
			&& CheckBPTR(fssm->fssm_Device))
		{
			const int ln = strlen("diskimage.device");
			if (AROS_BSTR_strlen(fssm->fssm_Device) == ln &&
				!memcmp(AROS_BSTR_ADDR(fssm->fssm_Device), "diskimage.device", ln))
			{
				num_entries++;
			}
		}
	}
	UnLockDosList(dosflags);
	
	entries = AllocVecPooled(Gui.pool, sizeof(APTR) * num_entries);
	if (!entries) {
		set(listview, MUIA_List_Quiet, FALSE);
		return;
	}
	for (i = 0; i < num_entries; i++) {
		entries[i] = AllocVecPooled(Gui.pool, sizeof(*e));
		if (!entries[i]) {
			num_entries = i;
			for (i = 0; i < num_entries; i++) {
				FreeVecPooled(Gui.pool, entries[i]);
			}
			FreeVecPooled(Gui.pool, entries);
			set(listview, MUIA_List_Quiet, FALSE);
			return;
		}
	}
	
	i = 0;
	dl = LockDosList(dosflags);
	while ((dl = NextDosEntry(dl, dosflags))) {
		if (CheckBPTR(dl->dol_Name) && (fssm = CheckBPTR(dl->dol_misc.dol_handler.dol_Startup))
			&& CheckBPTR(fssm->fssm_Device))
		{
			const int ln = strlen("diskimage.device");
			if (AROS_BSTR_strlen(fssm->fssm_Device) == ln &&
				!memcmp(AROS_BSTR_ADDR(fssm->fssm_Device), "diskimage.device", ln))
			{
				if (i < num_entries) {
					CopyStringBSTRToC(dl->dol_Name, devicename, sizeof(devicename));
					e = entries[i];
					e->unit_num = fssm->fssm_Unit;
					e->device_type = DG_DIRECT_ACCESS;
					e->writeprotect = FALSE;
					e->unit = ASPrintfPooled(Gui.pool, "%ld", e->unit_num);
					e->drive = ASPrintfPooled(Gui.pool, "%s", devicename);
					e->diskimage = NULL;
					if (selected_unit == e->unit_num) {
						selected_entry = e;
					}
					i++;
				}
			}
		}
	}
	UnLockDosList(dosflags);
	
	DoMethod(listview, MUIM_List_Insert, entries, i, MUIV_List_Insert_Sorted);
	set(listview, MUIA_List_Quiet, FALSE);
	if (selected_entry) {
		set(listview, MUIA_List_Active, selected_entry->list_pos);
	}
	
	for (; i < num_entries; i++) {
		FreeVecPooled(Gui.pool, entries[i]);
	}
	FreeVecPooled(Gui.pool, entries);
}

HOOKPROTO(ListviewPluginFunc, IPTR, struct DiskImagePlugin *plugin, APTR unused) {
	struct PluginEntry *e;
	e = AllocVecPooled(Gui.pool, sizeof(*e));
	if (e) {
		e->pri_num = plugin->Node.ln_Pri;
		e->is_builtin = (plugin->Flags & PLUGIN_FLAG_BUILTIN) ? TRUE : FALSE;
		e->has_write = plugin->plugin_Write ? TRUE : FALSE;
		e->priority = NULL;
		e->name = plugin->Node.ln_Name;
		DoMethod(Gui.gad[GID_PLUGINLIST], MUIM_List_InsertSingle, e, MUIV_List_Insert_Bottom);
	}
	return 0;
}
MakeStaticHook(ListviewPluginHook, ListviewPluginFunc);

void ScanPlugins (void) {
	Object *listview = Gui.gad[GID_PLUGINLIST];
	
	set(listview, MUIA_List_Quiet, TRUE);
	
	DoMethod(listview, MUIM_List_Clear);
	
	DoHookPlugins(&ListviewPluginHook);
	
	set(listview, MUIA_List_Quiet, FALSE);
}

void ChangeTempDir (void) {
	STRPTR tmpdir;
	struct FileRequester *freq;
	tmpdir = GetEnvVar(TEMPDIR_VAR);
	freq = MUI_AllocAslRequestTags(ASL_FileRequest,
		ASLFR_InitialDrawer,	tmpdir ? tmpdir : (STRPTR)"T:",
		ASLFR_DoSaveMode,		FALSE,
		ASLFR_DrawersOnly,		TRUE,
		TAG_END);
	if (freq) {
		SetWindowBusy(~0, TRUE);
		if (MUI_AslRequest(freq, NULL)) {
			SetEnvVar(TEMPDIR_VAR, freq->fr_Drawer, FALSE);
		}
		SetWindowBusy(~0, FALSE);
		MUI_FreeAslRequest(freq);
	}
	FreeVec(tmpdir);
}

void SaveSettings (void) {
	STRPTR tmpdir;
	tmpdir = GetEnvVar(TEMPDIR_VAR);
	SetEnvVar(TEMPDIR_VAR, tmpdir, TRUE);
	FreeVec(tmpdir);
}

HOOKPROTO(BrokerFunc, IPTR, Object *app, CxMsg *msg) {
	Object **wnd = Gui.wnd;
	switch (CxMsgType(msg)) {
		case CXM_IEVENT:
			switch (CxMsgID(msg)) {
				case EVT_POPKEY:
					set(wnd[WID_MAIN], MUIA_Window_Open, TRUE);
					set(app, MUIA_Application_Iconified, FALSE);
					DoMethod(Gui.gad[GID_DRIVELIST], MUIM_List_Redraw, MUIV_List_Redraw_All);
					break;
			}
			break;
		case CXM_COMMAND:
			switch (CxMsgID(msg)) {
				case CXCMD_DISABLE:
					ActivateCxObj((CxObj *)XGET(app, MUIA_Application_Broker), FALSE);
					break;
				case CXCMD_ENABLE:
					ActivateCxObj((CxObj *)XGET(app, MUIA_Application_Broker), TRUE);
					break;
				case CXCMD_UNIQUE:
				case CXCMD_APPEAR:
					set(wnd[WID_MAIN], MUIA_Window_Open, TRUE);
					set(app, MUIA_Application_Iconified, FALSE);
					DoMethod(Gui.gad[GID_DRIVELIST], MUIM_List_Redraw, MUIV_List_Redraw_All);
					break;
				case CXCMD_DISAPPEAR:
					set(wnd[WID_MAIN], MUIA_Window_Open, FALSE);
					break;
				case CXCMD_KILL:
					DoMethod(app, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
					break;
			}
			break;
	}
	return 0;
}

HOOKPROTO(MenuFunc, IPTR, Object *app, IPTR *params) {
	ULONG menu_id = (ULONG)params[0];
	Object **wnd = Gui.wnd;
	switch (menu_id) {
		case MID_ABOUT:
			set(wnd[WID_ABOUT], MUIA_Window_Open, TRUE);
			break;
		case MID_HIDE:
			set(wnd[WID_MAIN], MUIA_Window_Open, FALSE);
			break;
		case MID_ICONIFY:
			set(app, MUIA_Application_Iconified, TRUE);
			break;
		case MID_SNAPSHOT:
			DoMethod(wnd[WID_MAIN], MUIM_Window_Snapshot, TRUE);
			break;
		case MID_QUIT:
			DoMethod(app, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
			break;
		case MID_CHANGETEMPDIR:
			ChangeTempDir();
			break;
		case MID_PLUGINS:
			set(wnd[WID_PLUGINS], MUIA_Window_Open, TRUE);
			DoMethod(Gui.gad[GID_PLUGINLIST], MUIM_List_Redraw, MUIV_List_Redraw_All);
			break;
		case MID_SAVESETTINGS:
			SaveSettings();
			break;
	}
	return 0;
}

HOOKPROTO(SignalFunc, IPTR, APTR unused, IPTR *params) {
	struct Task *task = (struct Task *)params[0];
	ULONG sigmask = (ULONG)params[1];
	Signal(task, sigmask);
	return 0;
}
