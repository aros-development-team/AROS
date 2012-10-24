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

#define DRIVE_COL_FMT "MINWIDTH=16 BAR,P=\33r BAR,BAR,MINWIDTH=16 BAR,"
enum {
	DRIVE_COL_ICON,
	DRIVE_COL_UNIT,
	DRIVE_COL_DEVICE,
	DRIVE_COL_WP,
	DRIVE_COL_DISKIMAGE,
	DRIVE_COL_MAX
};

enum {
	DRIVE_ICO_DISK,
	DRIVE_ICO_CD,
	DRIVE_ICO_WP,
	DRIVE_ICO_MAX
};

struct DriveListData {
	Object *image[DRIVE_ICO_MAX];
	IPTR handle[DRIVE_ICO_MAX];
};

static CONST CONST_STRPTR image_name[DRIVE_ICO_MAX] = {
	"Disk",
	"CD",
	"Crypt"
};
static TEXT image_str[DRIVE_ICO_MAX][16];

DISPATCHERPROTO(DriveList_Dispatch);

struct MUI_CustomClass *DriveList_CreateClass (void) {
	return MUI_CreateCustomClass(NULL, MUIC_List, NULL, sizeof(struct DriveListData),
		ENTRY(DriveList_Dispatch));
}

void DriveList_FreeClass (struct MUI_CustomClass *cl) {
	MUI_DeleteCustomClass(cl);
}

static IPTR DriveList_New(Class *cl, Object *o, struct opSet *ops);
static IPTR DriveList_Setup(Class *cl, Object *o, Msg msg);
static IPTR DriveList_Cleanup(Class *cl, Object *o, Msg msg);

DISPATCHER(DriveList_Dispatch) {
	switch (msg->MethodID) {
		case OM_NEW: return DriveList_New(cl, obj, (struct opSet *)msg);
		case MUIM_Setup: return DriveList_Setup(cl, obj, msg);
		case MUIM_Cleanup: return DriveList_Cleanup(cl, obj, msg);
	}
	return DoSuperMethodA(cl, obj, msg);
}

HOOKPROTO(DriveList_ConstructFunc, IPTR, APTR pool, struct DriveEntry *e);
MakeStaticHook(DriveList_ConstructHook, DriveList_ConstructFunc);
HOOKPROTO(DriveList_DestructFunc, IPTR, APTR pool, struct DriveEntry *e);
MakeStaticHook(DriveList_DestructHook, DriveList_DestructFunc);
HOOKPROTO(DriveList_CompareFunc, IPTR, const struct DriveEntry *e2, const struct DriveEntry *e1);
MakeStaticHook(DriveList_CompareHook, DriveList_CompareFunc);
HOOKPROTO(DriveList_DisplayFunc, IPTR, CONST_STRPTR *array, struct DriveEntry *e);
MakeStaticHook(DriveList_DisplayHook, DriveList_DisplayFunc);

static IPTR DriveList_New(Class *cl, Object *o, struct opSet *ops) {
	Object *res;
	struct TagItem tags[] = {
		{ MUIA_List_Title,         TRUE                           },
		{ MUIA_List_Format,        (IPTR)DRIVE_COL_FMT            },
		{ MUIA_List_ConstructHook, (IPTR)&DriveList_ConstructHook },
		{ MUIA_List_DestructHook,  (IPTR)&DriveList_DestructHook  },
		{ MUIA_List_CompareHook,   (IPTR)&DriveList_CompareHook   },
		{ MUIA_List_DisplayHook,   (IPTR)&DriveList_DisplayHook   },
		{ TAG_MORE,                (IPTR)ops->ops_AttrList        }
	};
	ops->ops_AttrList = tags;
	res = (Object *)DoSuperMethodA(cl, o, (Msg)ops);
	ops->ops_AttrList = (struct TagItem *)tags[6].ti_Data;
	return (IPTR)res;
}

static IPTR DriveList_Setup(Class *cl, Object *o, Msg msg) {
	struct DriveListData *data = INST_DATA(cl, o);
	int i;
	if (!DoSuperMethodA(cl, o, msg)) {
		return FALSE;
	}
	for (i = 0; i < DRIVE_ICO_MAX; i++) {
		data->image[i] = LoadImage(image_name[i], NULL);
		if (data->image[i]) {
			data->handle[i] = DoMethod(o, MUIM_List_CreateImage, data->image[i], 0);
		} else {
			data->handle[i] = (IPTR)NULL;
		}
		SNPrintf(image_str[i], sizeof(image_str[i]), "\33O[%08lx]", data->handle[i]);
	}
	return TRUE;
}

static IPTR DriveList_Cleanup(Class *cl, Object *o, Msg msg) {
	struct DriveListData *data = INST_DATA(cl, o);
	int i;
	for (i = 0; i < DRIVE_ICO_MAX; i++) {
		DoMethod(o, MUIM_List_DeleteImage, data->handle[i]);
		MUI_DisposeObject(data->image[i]);
		image_str[i][0] = 0;
		data->handle[i] = 0;
		data->image[i] = NULL;
	}
	return DoSuperMethodA(cl, o, msg);
}

HOOKPROTO(DriveList_ConstructFunc, IPTR, APTR pool, struct DriveEntry *e) {
	if (e) {
		UnitControl(e->unit_num,
			DITAG_GetDeviceType,	(IPTR)&e->device_type,
			DITAG_GetImageName,		(IPTR)&e->diskimage,
			DITAG_GetWriteProtect,	(IPTR)&e->writeprotect,
			TAG_END);
	}
	return (IPTR)e;
}

HOOKPROTO(DriveList_DestructFunc, IPTR, APTR pool, struct DriveEntry *e) {
	if (e) {
		FreeVecPooled(pool, e->unit);
		FreeVecPooled(pool, e->drive);
		FreeVec(e->diskimage);
		FreeVecPooled(pool, e);
	}
	return 0;
}

HOOKPROTO(DriveList_CompareFunc, IPTR, const struct DriveEntry *e2, const struct DriveEntry *e1) {
	return ((LONG)e1->unit_num - (LONG)e2->unit_num);
}

static inline LONG GetDeviceIcon (UBYTE devtype) {
	if (devtype == DG_CDROM) {
		return DRIVE_ICO_CD;
	}
	return DRIVE_ICO_DISK;
}

HOOKPROTO(DriveList_DisplayFunc, IPTR, CONST_STRPTR *array, struct DriveEntry *e) {
	if (e) {
		e->list_pos = (IPTR)array[-1];
		array[DRIVE_COL_ICON] = image_str[GetDeviceIcon(e->device_type)];
		array[DRIVE_COL_UNIT] = e->unit ? e->unit : (STRPTR)"";
		array[DRIVE_COL_DEVICE] = e->drive ? e->drive : (STRPTR)"";
		array[DRIVE_COL_WP] = e->writeprotect ? image_str[DRIVE_ICO_WP] : (STRPTR)"";
		array[DRIVE_COL_DISKIMAGE] = e->diskimage ? e->diskimage : GetString(&LocaleInfo, MSG_NO_DISK);
	} else {
		array[DRIVE_COL_ICON] = "";
		array[DRIVE_COL_UNIT] = GetString(&LocaleInfo, MSG_UNIT_LBL);
		array[DRIVE_COL_DEVICE] = GetString(&LocaleInfo, MSG_DEVICE_LBL);
		array[DRIVE_COL_WP] = GetString(&LocaleInfo, MSG_WRITEPROTECT_LBL);
		array[DRIVE_COL_DISKIMAGE] = GetString(&LocaleInfo, MSG_FILENAME_LBL);
	}
	return 0;
}

HOOKPROTO(DriveList_ActiveFunc, IPTR, Object *app, IPTR *params);
MakeHook(DriveList_ActiveHook, DriveList_ActiveFunc);
HOOKPROTO(DriveList_DoubleClickFunc, IPTR, Object *app, IPTR *params);
MakeHook(DriveList_DoubleClickHook, DriveList_DoubleClickFunc);

HOOKPROTO(DriveList_ActiveFunc, IPTR, Object *app, IPTR *params) {
	Object **gad = Gui.gad;
	struct DriveEntry *e;
	DoMethod(gad[GID_DRIVELIST], MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &e);
	set(gad[GID_INSERT], MUIA_Disabled, !e);
	set(gad[GID_EJECT], MUIA_Disabled, !e || !e->diskimage);
	set(gad[GID_WRITEPROTECT], MUIA_Disabled, !e);
	set(gad[GID_SETDEVICETYPE], MUIA_Disabled, !e);
	return 0;
}

HOOKPROTO(DriveList_DoubleClickFunc, IPTR, Object *app, IPTR *params) {
	Object **gad = Gui.gad;
	struct DriveEntry *e;
	DoMethod(gad[GID_DRIVELIST], MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &e);
	if (e) {
		if (e->diskimage) {
			DoMethod(app, MUIM_CallHook, &EjectHook);
		} else {
			DoMethod(app, MUIM_CallHook, &InsertHook);
		}
	}
	return 0;
}
