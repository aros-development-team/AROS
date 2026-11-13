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

#include <gadgets/extscroller.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <clib/alib_protos.h>
#include <SDI_compiler.h>

#define GA(o) ((struct Gadget *)(o))

struct ExtScroller {
	LONG total;
	LONG visible;
};

static ULONG EXTSCROLLER_Dispatch (REG(a0, Class *cl), REG(a2, Object *o), REG(a1, Msg msg));

Class *ExtScrollerClass;

Class *InitExtScrollerClass (void) {
	ExtScrollerClass = MakeClass("extscroller.gadget", "propgclass", NULL,
		sizeof(struct ExtScroller), 0);
	if (ExtScrollerClass) {
		ExtScrollerClass->cl_Dispatcher.h_Entry = EXTSCROLLER_Dispatch;
	}
	return ExtScrollerClass;
}

static ULONG EXTSCROLLER_Get (Class *cl, Object *o, struct opGet *opg);
static ULONG EXTSCROLLER_Set (Class *cl, Object *o, struct opSet *ops);
static void DoRender (Object *o, struct GadgetInfo *ginfo, ULONG flags);

static ULONG EXTSCROLLER_Dispatch (REG(a0, Class *cl), REG(a2, Object *o), REG(a1, Msg msg)) {
	switch (msg->MethodID) {
		case OM_GET: return EXTSCROLLER_Get(cl, o, (struct opGet *)msg);
		case OM_SET:
		case OM_UPDATE: return EXTSCROLLER_Set(cl, o, (struct opSet *)msg);
	}
	return DoSuperMethodA(cl, o, msg);
}

static ULONG EXTSCROLLER_Get (Class *cl, Object *o, struct opGet *opg) {
	ULONG attrid = opg->opg_AttrID;
	switch (attrid) {
		case SCROLLER_Total:
			attrid = PGA_Total;
			break;
		case SCROLLER_Visible:
			attrid = PGA_Visible;
			break;
		case SCROLLER_Top:
			attrid = PGA_Top;
			break;
		default:
			return DoSuperMethodA(cl, o, (Msg)opg);
	}
	return DoSuperMethod(cl, o, OM_GET, attrid, opg->opg_Storage);
}

static ULONG EXTSCROLLER_Set (Class *cl, Object *o, struct opSet *ops) {
	struct ExtScroller *extscr = INST_DATA(cl, o);
	struct TagItem *tstate = ops->ops_AttrList;
	struct TagItem *tag;
	while ((tag = NextTagItem(&tstate))) {
		switch (tag->ti_Tag) {
			case SCROLLER_Total:
				tag->ti_Tag = PGA_Total;
			case PGA_Total:
				extscr->total = tag->ti_Data;
				break;
			case SCROLLER_Visible:
				tag->ti_Tag = PGA_Visible;
			case PGA_Visible:
				extscr->visible = tag->ti_Data;
				break;
			case SCROLLER_Top:
				tag->ti_Tag = PGA_Top;
				break;
			case EXTSCROLLER_GoUp:
			case EXTSCROLLER_GoDown:
				if (tag->ti_Data == 1) {
					LONG top;
					struct TagItem tags[2];
					DoSuperMethod(cl, o, OM_GET, PGA_Top, &top);
					if (tag->ti_Tag == EXTSCROLLER_GoUp) {
						const LONG min = 0;
						if (top == min) break;
						top -= 5;
						if (top < min) top = min;
					} else {
						const LONG max = extscr->total - extscr->visible;
						if (top == max) break;
						top += 5;
						if (top > max) top = max;
					}
					tags[0].ti_Tag = PGA_Top;
					tags[0].ti_Data = top;
					tags[1].ti_Tag = TAG_END;
					DoSuperMethod(cl, o, OM_SET, tags, ops->ops_GInfo, 0);
					DoSuperMethod(cl, o, OM_NOTIFY, tags, ops->ops_GInfo, 0);
					DoRender(o, ops->ops_GInfo, GREDRAW_UPDATE);
				}
				break;
		}
	}
	return DoSuperMethodA(cl, o, (Msg)ops);
}

static void DoRender (Object *o, struct GadgetInfo *ginfo, ULONG flags) {
	struct RastPort *rp;
	rp = ObtainGIRPort(ginfo);
	if (rp) {
		DoMethod(o, GM_RENDER, ginfo, rp, flags);
		ReleaseGIRPort(rp);
	}
}
