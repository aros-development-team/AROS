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

#include <intuition/icclass.h>
#include <classes/extwindow.h>
#include <gadgets/extscroller.h>
#include <images/titlebar.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/window.h>
#include <clib/alib_protos.h>
#include <SDI_compiler.h>

#define GA(o) ((struct Gadget *)(o))

struct ExtWindow {
	struct Window *window;
	ULONG flags;
	APTR drawinfo;
	Object *snapshotimg;
	Object *snapshotgad;
	Object *vertpropgad;
	Object *uparrowimg;
	Object *uparrowgad;
	Object *downarrowimg;
	Object *downarrowgad;
};

#define GID_SNAPSHOT  0x7FFE
#define GID_VERTPROP  0x7FFD
#define GID_HORIZPROP 0x7FFC
#define EWFLG_SNAPSHOTGADGET 0x00000001
#define EWFLG_VERTPROPGADGET 0x00000002

static ULONG EXTWINDOW_Dispatch (REG(a0, Class *cl), REG(a2, Object *o), REG(a1, Msg msg));

Class *ExtWindowClass;

Class *InitExtWindowClass (void) {
	ExtWindowClass = MakeClass("extwindow.class", NULL, WINDOW_GetClass(),
		sizeof(struct ExtWindow), 0);
	if (ExtWindowClass) {
		ExtWindowClass->cl_Dispatcher.h_Entry = EXTWINDOW_Dispatch;
	}
	return ExtWindowClass;
}

static ULONG EXTWINDOW_New (Class *cl, Object *o, struct opSet *ops);
static ULONG EXTWINDOW_Get (Class *cl, Object *o, struct opGet *opg);
static ULONG EXTWINDOW_Open (Class *cl, Object *o, Msg msg);
static ULONG EXTWINDOW_Close (Class *cl, Object *o, Msg msg);
static ULONG EXTWINDOW_Dispose (Class *cl, Object *o, Msg msg);
static ULONG EXTWINDOW_HandleInput (Class *cl, Object *o, Msg msg);
static void ParseTagList (const struct TagItem *src_tags,
	struct TagItem *win_tags, struct TagItem *extwin_tags);
static struct Gadget *FindSysGadget (struct Window *window, ULONG type);
static BOOL AddSnapshotGadget (struct ExtWindow *extwin);
static void RemSnapshotGadget (struct ExtWindow *extwin);
static BOOL AddVertPropGadget (struct ExtWindow *extwin);
static void RemVertPropGadget (struct ExtWindow *extwin);

static ULONG EXTWINDOW_Dispatch (REG(a0, Class *cl), REG(a2, Object *o), REG(a1, Msg msg)) {
	switch (msg->MethodID) {
		case OM_NEW: return EXTWINDOW_New(cl, o, (struct opSet *)msg);
		case OM_GET: return EXTWINDOW_Get(cl, o, (struct opGet *)msg);
		case WM_OPEN: return EXTWINDOW_Open(cl, o, msg);
		case WM_CLOSE:
		case WM_ICONIFY: return EXTWINDOW_Close(cl, o, msg);
		case OM_DISPOSE: return EXTWINDOW_Dispose(cl, o, msg);
		case WM_HANDLEINPUT: return EXTWINDOW_HandleInput(cl, o, msg);
	}
	return DoSuperMethodA(cl, o, msg);
}

static const struct TagItem up2vprop_map[] = {
	{ GA_ID,   EXTSCROLLER_GoUp   },
	{ TAG_END, 0                  },
};
static const struct TagItem down2vprop_map[] = {
	{ GA_ID,   EXTSCROLLER_GoDown },
	{ TAG_END, 0                  },
};

static ULONG EXTWINDOW_New (Class *cl, Object *o, struct opSet *ops) {
	Object *res;
	res = (Object *)DoSuperMethodA(cl, o, (Msg)ops);
	if (res) {
		struct TagItem *tags = ops->ops_AttrList;
		struct ExtWindow *extwin = INST_DATA(cl, res);
		LONG error_count = 0;
		if (GetTagData(EXTWINDOW_VertProp, FALSE, tags)) {
			extwin->flags |= EWFLG_VERTPROPGADGET;
			extwin->vertpropgad = ExtScrollerObject,
				GA_ID,			GID_VERTPROP,
				PGA_Borderless,	TRUE,
				PGA_NewLook,	TRUE,
			End;
			if (!extwin->vertpropgad) error_count++;
			extwin->uparrowgad = NewObject(NULL, "buttongclass",
				GA_ID,			1,
				GA_RelVerify,	FALSE,
				ICA_MAP,		up2vprop_map,
				ICA_TARGET,		extwin->vertpropgad,
				TAG_END);
			if (!extwin->uparrowgad) error_count++;
			extwin->downarrowgad = NewObject(NULL, "buttongclass",
				GA_ID,			1,
				GA_RelVerify,	FALSE,
				ICA_MAP,		down2vprop_map,
				ICA_TARGET,		extwin->vertpropgad,
				TAG_END);
			if (!extwin->downarrowgad) error_count++;
		}
		if (GetTagData(EXTWINDOW_SnapshotGadget, FALSE, tags)) {
			extwin->flags |= EWFLG_SNAPSHOTGADGET;
			extwin->snapshotgad = NewObject(NULL, "buttongclass",
				GA_ID,			GID_SNAPSHOT,
				GA_RelVerify,	TRUE,
				TAG_END);
			if (!extwin->snapshotgad) error_count++;
		}
		if (error_count) {
			CoerceMethod(cl, res, OM_DISPOSE);
			return (ULONG)NULL;
		}
	}
	return (ULONG)res;
}

static ULONG EXTWINDOW_Get (Class *cl, Object *o, struct opGet *opg) {
	struct ExtWindow *extwin = INST_DATA(cl, o);
	switch (opg->opg_AttrID) {
		case EXTWINDOW_VertObject:
			*opg->opg_Storage = (ULONG)extwin->vertpropgad;
			break;
		default:
			return DoSuperMethodA(cl, o, (Msg)opg);
	}
	return TRUE;
}

static ULONG EXTWINDOW_Open (Class *cl, Object *o, Msg msg) {
	struct ExtWindow *extwin = INST_DATA(cl, o);
	struct Window *res;
	res = (struct Window *)DoSuperMethodA(cl, o, msg);
	if (!extwin->window && res) {
		extwin->window = res;
		extwin->drawinfo = GetScreenDrawInfo(res->WScreen);
		if (extwin->flags & EWFLG_VERTPROPGADGET) {
			AddVertPropGadget(extwin);
		}
		if (extwin->flags & EWFLG_SNAPSHOTGADGET) {
			AddSnapshotGadget(extwin);
		}
	}
	return (ULONG)res;
}

static ULONG EXTWINDOW_Close (Class *cl, Object *o, Msg msg) {
	struct ExtWindow *extwin = INST_DATA(cl, o);
	if (extwin->window) {
		RemSnapshotGadget(extwin);
		RemVertPropGadget(extwin);
		FreeScreenDrawInfo(extwin->window->WScreen, extwin->drawinfo);
		extwin->drawinfo = NULL;
		extwin->window = NULL;
	}
	return DoSuperMethodA(cl, o, msg);
}

static ULONG EXTWINDOW_Dispose (Class *cl, Object *o, Msg msg) {
	struct ExtWindow *extwin = INST_DATA(cl, o);
	if (extwin->window) {
		RemSnapshotGadget(extwin);
		RemVertPropGadget(extwin);
		FreeScreenDrawInfo(extwin->window->WScreen, extwin->drawinfo);
		extwin->drawinfo = NULL;
		extwin->window = NULL;
	}
	DisposeObject(extwin->snapshotgad);
	extwin->snapshotgad = NULL;
	DisposeObject(extwin->downarrowgad);
	extwin->downarrowgad = NULL;
	DisposeObject(extwin->uparrowgad);
	extwin->uparrowgad = NULL;
	DisposeObject(extwin->vertpropgad);
	extwin->vertpropgad = NULL;
	return DoSuperMethodA(cl, o, msg);
}

static ULONG EXTWINDOW_HandleInput (Class *cl, Object *o, Msg msg) {
	ULONG res;
	res = DoSuperMethodA(cl, o, msg);
	if ((res & WMHI_CLASSMASK) == WMHI_GADGETUP) {
		switch (res & WMHI_GADGETMASK) {
			case GID_SNAPSHOT:
				res = WMHI_SNAPSHOT;
				break;
		}
	}
	return res;
}

static struct Gadget *FindSysGadget (struct Window *window, ULONG type) {
	if (window) {
		struct Gadget *gad;
		gad = window->FirstGadget;
		while (gad) {
			if ((gad->GadgetType & (GTYP_SYSGADGET|GTYP_SYSTYPEMASK)) == (GTYP_SYSGADGET|type)) {
				return gad;
			}
			gad = gad->NextGadget;
		}
	}
	return NULL;
}

static BOOL AddSnapshotGadget (struct ExtWindow *extwin) {
	struct Window *window = extwin->window;
	struct Gadget *zoomgad;
	zoomgad = FindSysGadget(window, GTYP_WZOOM);
	if (!zoomgad) {
		return FALSE;
	}
	extwin->snapshotimg = NewObject(NULL, "tbiclass",
		SYSIA_DrawInfo,	extwin->drawinfo,
		SYSIA_Which,	SNAPSHOTIMAGE,
		TAG_END);
	if (!extwin->snapshotimg) {
		return FALSE;
	}
	SetAttrs(extwin->snapshotgad,
		GA_Image,		extwin->snapshotimg,
		GA_RelRight,	TBI_RELPOS(zoomgad, 3),
		GA_Top,			0,
		GA_Width,		zoomgad->Width,
		GA_Height,		zoomgad->Height,
		GA_TopBorder,	TRUE,
		TAG_END);
	AddGadget(window, GA(extwin->snapshotgad), 0);
	RefreshGList(GA(extwin->snapshotgad), window, NULL, 1);
	return TRUE;
}

static void RemSnapshotGadget (struct ExtWindow *extwin) {
	if (extwin->snapshotgad) {
		RemoveGadget(extwin->window, GA(extwin->snapshotgad));
		SetAttrs(extwin->snapshotgad,
			GA_Image,	NULL,
			TAG_END);
		DisposeObject(extwin->snapshotimg);
		extwin->snapshotimg = NULL;
	}
}

static BOOL AddVertPropGadget (struct ExtWindow *extwin) {
	struct Window *window = extwin->window;
	struct Gadget *sizegad;
	sizegad = FindSysGadget(window, GTYP_SIZING);
	if (!sizegad) {
		return FALSE;
	}
	extwin->uparrowimg = NewObject(NULL, "sysiclass",
		SYSIA_DrawInfo,	extwin->drawinfo,
		SYSIA_Which,	UPIMAGE,
		TAG_END);
	if (!extwin->uparrowimg) {
		return FALSE;
	}
	extwin->downarrowimg = NewObject(NULL, "sysiclass",
		SYSIA_DrawInfo,	extwin->drawinfo,
		SYSIA_Which,	DOWNIMAGE,
		TAG_END);
	if (!extwin->downarrowimg) {
		DisposeObject(extwin->uparrowimg);
		extwin->uparrowimg = NULL;
		return FALSE;
	}
	SetAttrs(extwin->vertpropgad,
		GA_RelRight,	-(window->BorderRight - 5),
		GA_Top,			window->BorderTop + 1,
		GA_Width,		window->BorderRight - 8,
		GA_RelHeight,	-(window->BorderTop + sizegad->Height + 4 + 2*sizegad->Height),
		GA_RightBorder,	TRUE,
		TAG_END);
	SetAttrs(extwin->uparrowgad,
		GA_Image,		extwin->uparrowimg,
		GA_RelRight,	-(window->BorderRight - 1),
		GA_RelBottom,	-(3*sizegad->Height + 1),
		GA_Width,		window->BorderRight,
		GA_Height,		sizegad->Height,
		TAG_END);
	SetAttrs(extwin->downarrowgad,
		GA_Image,		extwin->downarrowimg,
		GA_RelRight,	-(window->BorderRight - 1),
		GA_RelBottom,	-(2*sizegad->Height),
		GA_Width,		window->BorderRight,
		GA_Height,		sizegad->Height,
		TAG_END);
	AddGadget(window, GA(extwin->vertpropgad), 0);
	AddGadget(window, GA(extwin->uparrowgad), 0);
	AddGadget(window, GA(extwin->downarrowgad), 0);
	RefreshGList(GA(extwin->vertpropgad), window, NULL, 1);
	RefreshGList(GA(extwin->uparrowgad), window, NULL, 1);
	RefreshGList(GA(extwin->downarrowgad), window, NULL, 1);
	return TRUE;
}

static void RemVertPropGadget (struct ExtWindow *extwin) {
	if (extwin->vertpropgad) {
		RemoveGadget(extwin->window, GA(extwin->vertpropgad));
		RemoveGadget(extwin->window, GA(extwin->uparrowgad));
		RemoveGadget(extwin->window, GA(extwin->downarrowgad));
		SetAttrs(extwin->uparrowgad,
			GA_Image,	NULL,
			TAG_END);
		SetAttrs(extwin->downarrowgad,
			GA_Image,	NULL,
			TAG_END);
		DisposeObject(extwin->uparrowimg);
		extwin->uparrowimg = NULL;
		DisposeObject(extwin->downarrowimg);
		extwin->downarrowimg = NULL;
	}
}
