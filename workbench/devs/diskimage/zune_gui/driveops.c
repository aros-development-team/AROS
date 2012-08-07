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
#include <proto/diskimage.h>
#include <clib/alib_protos.h>
#include <SDI_hook.h>

HOOKPROTO(InsertFunc, IPTR, Object *app, IPTR *params);
MakeHook(InsertHook, InsertFunc);
HOOKPROTO(EjectFunc, IPTR, Object *app, IPTR *params);
MakeHook(EjectHook, EjectFunc);
HOOKPROTO(WriteProtectFunc, IPTR, Object *app, IPTR *params);
MakeHook(WriteProtectHook, WriteProtectFunc);
HOOKPROTO(SetDeviceTypeFunc, IPTR, Object *app, IPTR *params);
MakeHook(SetDeviceTypeHook, SetDeviceTypeFunc);
HOOKPROTO(DoSetDeviceTypeFunc, IPTR, Object *app, IPTR *params);
MakeHook(DoSetDeviceTypeHook, DoSetDeviceTypeFunc);


HOOKPROTO(InsertFunc, IPTR, Object *app, IPTR *params) {
	struct DriveEntry *e;
	DoMethod(Gui.gad[GID_DRIVELIST], MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &e);
	if (e) {
		SetWindowBusy(~0, TRUE);
		if (MUI_AslRequest(FileReq, NULL)) {
			STRPTR filename;
			filename = CombinePaths(FileReq->fr_Drawer, FileReq->fr_File);
			if (filename) {
				LONG error = NO_ERROR;
				TEXT error_buffer[256];
				LONG msge;
				FreeVec(e->diskimage);
				e->diskimage = NULL;
				error_buffer[0] = 0;
				msge = UnitControl(e->unit_num,
					DITAG_Error,				(IPTR)&error,
					DITAG_ErrorString,			(IPTR)error_buffer,
					DITAG_ErrorStringLength,	sizeof(error_buffer),
					DITAG_CurrentDir,			(IPTR)GetCurrentDir(),
					DITAG_Filename,				(IPTR)filename,
					DITAG_GetImageName,			(IPTR)&e->diskimage,
					TAG_END);
				FreeVec(filename);
				if (error_buffer[0]) {
					ErrorStringRequester(error_buffer);
				} else {
					IoErrRequester(error ? error : msge);
				}
				DoMethod(Gui.gad[GID_DRIVELIST], MUIM_List_Redraw, e->list_pos);
			}
		}
		SetWindowBusy(~0, FALSE);
	}
	return 0;
}

HOOKPROTO(EjectFunc, IPTR, Object *app, IPTR *params) {
	struct DriveEntry *e;
	DoMethod(Gui.gad[GID_DRIVELIST], MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &e);
	if (e) {
		FreeVec(e->diskimage);
		e->diskimage = NULL;
		UnitControl(e->unit_num,
			DITAG_Filename,		(IPTR)NULL,
			DITAG_GetImageName,	(IPTR)&e->diskimage,
			TAG_END);
		DoMethod(Gui.gad[GID_DRIVELIST], MUIM_List_Redraw, e->list_pos);
	}
	return 0;
}

HOOKPROTO(WriteProtectFunc, IPTR, Object *app, IPTR *params) {
	struct DriveEntry *e;
	DoMethod(Gui.gad[GID_DRIVELIST], MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &e);
	if (e) {
		UnitControl(e->unit_num,
			DITAG_WriteProtect,		!e->writeprotect,
			DITAG_GetWriteProtect,	(IPTR)&e->writeprotect,
			TAG_END);
		DoMethod(Gui.gad[GID_DRIVELIST], MUIM_List_Redraw, e->list_pos);
	}
	return 0;
}

HOOKPROTO(SetDeviceTypeFunc, IPTR, Object *app, IPTR *params) {
	struct DriveEntry *e;
	DoMethod(Gui.gad[GID_DRIVELIST], MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &e);
	if (e) {
		LONG active;
		ULONG sigs;
		SetWindowBusy(~0, TRUE);
		switch (e->device_type) {
			case DG_CDROM:
				active = 1;
				break;
			default:
				active = 0;
				break;
		}
		DoSetDeviceTypeHook.h_Data = e;
		set(Gui.gad[GID_DEVICETYPE], MUIA_Cycle_Active, active);
		set(Gui.wnd[WID_SETDEVICETYPE], MUIA_Window_Open, TRUE);
		while (XGET(Gui.wnd[WID_SETDEVICETYPE], MUIA_Window_Open)) {
			DoMethod(Gui.app, MUIM_Application_NewInput, (IPTR)&sigs);
			if (sigs) {
				sigs |= (1UL << DiskChangeSignal);
				sigs = Wait(sigs|SIGBREAKF_CTRL_C);
				if (sigs & SIGBREAKF_CTRL_C) {
					break;
				}
				if (sigs & (1UL << DiskChangeSignal)) {
					ScanUnits();
				}
			}
		}
		SetWindowBusy(~0, FALSE);
	}
	return 0;
}

HOOKPROTO(DoSetDeviceTypeFunc, IPTR, Object *app, IPTR *params) {
	struct DriveEntry *e = hook->h_Data;
	LONG active = XGET(Gui.gad[GID_DEVICETYPE], MUIA_Cycle_Active);
	switch (active) {
		case 1:
			e->device_type = DG_CDROM;
			break;
		default:
			e->device_type = DG_DIRECT_ACCESS;
			break;
	}
	UnitControl(e->unit_num,
		DITAG_SetDeviceType,	e->device_type,
		DITAG_GetDeviceType,	(IPTR)&e->device_type,
		TAG_END);
	DoMethod(Gui.gad[GID_DRIVELIST], MUIM_List_Redraw, e->list_pos);
	set(Gui.wnd[WID_SETDEVICETYPE], MUIA_Window_Open, FALSE);
	return 0;
}
