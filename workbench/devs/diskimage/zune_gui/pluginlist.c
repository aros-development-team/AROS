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
#include <clib/alib_protos.h>
#include <SDI_hook.h>

#define PLUG_COL_FMT "MINWIDTH=16 BAR,P=\33r BAR,BAR,"
enum {
	PLUG_COL_ICON,
	PLUG_COL_PRI,
	PLUG_COL_WRITE,
	PLUG_COL_NAME,
	PLUG_COL_MAX
};

enum {
	PLUG_ICO_PLUGIN,
	PLUG_ICO_CHECKMARK,
	PLUG_ICO_MAX
};

struct PluginListData {
	Object *image[PLUG_ICO_MAX];
	IPTR handle[PLUG_ICO_MAX];
};

static CONST CONST_STRPTR image_name[PLUG_ICO_MAX] = {
	"list_plugin",
	"list_checkmark"
};
static TEXT image_str[PLUG_ICO_MAX][16];

DISPATCHERPROTO(PluginList_Dispatch);

struct MUI_CustomClass *PluginList_CreateClass (void) {
	return MUI_CreateCustomClass(NULL, MUIC_List, NULL, sizeof(struct PluginListData),
		ENTRY(PluginList_Dispatch));
}

void PluginList_FreeClass (struct MUI_CustomClass *cl) {
	MUI_DeleteCustomClass(cl);
}

static IPTR PluginList_New(Class *cl, Object *o, struct opSet *ops);
static IPTR PluginList_Setup(Class *cl, Object *o, Msg msg);
static IPTR PluginList_Cleanup(Class *cl, Object *o, Msg msg);

DISPATCHER(PluginList_Dispatch) {
	switch (msg->MethodID) {
		case OM_NEW: return PluginList_New(cl, obj, (struct opSet *)msg);
		case MUIM_Setup: return PluginList_Setup(cl, obj, msg);
		case MUIM_Cleanup: return PluginList_Cleanup(cl, obj, msg);
	}
	return DoSuperMethodA(cl, obj, msg);
}

HOOKPROTO(PluginList_ConstructFunc, IPTR, APTR pool, struct PluginEntry *e);
MakeStaticHook(PluginList_ConstructHook, PluginList_ConstructFunc);
HOOKPROTO(PluginList_DestructFunc, IPTR, APTR pool, struct PluginEntry *e);
MakeStaticHook(PluginList_DestructHook, PluginList_DestructFunc);
HOOKPROTO(PluginList_CompareFunc, IPTR, const struct PluginEntry *e2, const struct PluginEntry *e1);
MakeStaticHook(PluginList_CompareHook, PluginList_CompareFunc);
HOOKPROTO(PluginList_DisplayFunc, IPTR, CONST_STRPTR *array, struct PluginEntry *e);
MakeStaticHook(PluginList_DisplayHook, PluginList_DisplayFunc);

static IPTR PluginList_New(Class *cl, Object *o, struct opSet *ops) {
	Object *res;
	struct TagItem tags[] = {
		{ MUIA_List_Title,         TRUE                            },
		{ MUIA_List_Format,        (IPTR)PLUG_COL_FMT              },
		{ MUIA_List_ConstructHook, (IPTR)&PluginList_ConstructHook },
		{ MUIA_List_DestructHook,  (IPTR)&PluginList_DestructHook  },
		{ MUIA_List_CompareHook,   (IPTR)&PluginList_CompareHook   },
		{ MUIA_List_DisplayHook,   (IPTR)&PluginList_DisplayHook   },
		{ TAG_MORE,                (IPTR)ops->ops_AttrList         }
	};
	ops->ops_AttrList = tags;
	res = (Object *)DoSuperMethodA(cl, o, (Msg)ops);
	ops->ops_AttrList = (struct TagItem *)tags[6].ti_Data;
	return (IPTR)res;
}

static IPTR PluginList_Setup(Class *cl, Object *o, Msg msg) {
	struct PluginListData *data = INST_DATA(cl, o);
	int i;
	if (!DoSuperMethodA(cl, o, msg)) {
		return FALSE;
	}
	for (i = 0; i < PLUG_ICO_MAX; i++) {
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

static IPTR PluginList_Cleanup(Class *cl, Object *o, Msg msg) {
	struct PluginListData *data = INST_DATA(cl, o);
	int i;
	for (i = 0; i < PLUG_ICO_MAX; i++) {
		DoMethod(o, MUIM_List_DeleteImage, data->handle[i]);
		MUI_DisposeObject(data->image[i]);
		image_str[i][0] = 0;
		data->handle[i] = 0;
		data->image[i] = NULL;
	}
	return DoSuperMethodA(cl, o, msg);
}

HOOKPROTO(PluginList_ConstructFunc, IPTR, APTR pool, struct PluginEntry *e) {
	if (e) {
		e->priority = ASPrintfPooled(pool, "%ld", (LONG)e->pri_num);
		e->name = ASPrintfPooled(pool, e->is_builtin ? "%s (builtin)" : "%s", e->name);
	}
	return (IPTR)e;
}

HOOKPROTO(PluginList_DestructFunc, IPTR, APTR pool, struct PluginEntry *e) {
	if (e) {
		FreeVecPooled(pool, e->priority);
		FreeVecPooled(pool, e->name);
		FreeVecPooled(pool, e);
	}
	return 0;
}

HOOKPROTO(PluginList_CompareFunc, IPTR, const struct PluginEntry *e2, const struct PluginEntry *e1) {
	return ((LONG)e2->pri_num - (LONG)e1->pri_num);
}

HOOKPROTO(PluginList_DisplayFunc, IPTR, CONST_STRPTR *array, struct PluginEntry *e) {
	if (e) {
		e->list_pos = (ULONG)array[-1];
		array[PLUG_COL_ICON] = image_str[PLUG_ICO_PLUGIN];
		array[PLUG_COL_PRI] = e->priority;
		array[PLUG_COL_WRITE] = e->has_write ? image_str[PLUG_ICO_CHECKMARK] : (STRPTR)"";
		array[PLUG_COL_NAME] = e->name;
	} else {
		array[PLUG_COL_ICON] = "";
		array[PLUG_COL_PRI] = GetString(&LocaleInfo, MSG_PRIORITY_LBL);
		array[PLUG_COL_WRITE] = GetString(&LocaleInfo, MSG_WRITESUPPORT_LBL);
		array[PLUG_COL_NAME] = GetString(&LocaleInfo, MSG_PLUGIN_LBL);
	}
	return 0;
}
