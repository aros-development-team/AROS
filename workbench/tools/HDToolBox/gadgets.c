/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <proto/utility.h>
#include <exec/memory.h>
#include <intuition/gadgetclass.h>
#include <intuition/screens.h>

#include "gadgets.h"
#include "hdtoolbox_support.h"
#include "platform.h"
#include "ptclass.h"

#include "debug.h"

struct TextAttr tattr = {"topaz.font",8,0,0};

struct Gadget *createGadgets(struct creategadget *gadgets, ULONG start, ULONG end, APTR visual) {
struct Gadget *glist=NULL;
struct Gadget *gad;
ULONG i;

	gad = CreateContext(&glist);
	if (!gad)
		return 0;
	for (i=start; i<end; i++)
	{
		gadgets[i-start].newgadget.ng_VisualInfo = visual;
		gadgets[i-start].newgadget.ng_TextAttr = &tattr;
		gad = CreateGadgetA(gadgets[i-start].kind, gad, &gadgets[i-start].newgadget, gadgets[i-start].tags);
		if (!gad)
		{
			FreeGadgets(glist);
			return 0;
		}
		gadgets[i-start].gadget = gad;
	}
	return glist;
}

void freeGadgets(struct Gadget *glist) {

	if (glist)
		FreeGadgets(glist);
}

#if 0
void clearGadgets(struct ExtGadget *gadget, struct Window *win, ULONG num) {

	while (gadget)
	{
		if (!num)
			return;
		EraseRect(
			win->RPort,
			gadget->LeftEdge,
			gadget->TopEdge,
			gadget->LeftEdge+gadget->Width,
			gadget->TopEdge+gadget->Height
		);
		if (gadget->Flags & GFLG_EXTENDED)
		{
			if (gadget->MoreFlags & GMORE_BOUNDS)
				EraseRect(
					win->RPort,
					gadget->BoundsLeftEdge,
					gadget->BoundsTopEdge,
					gadget->BoundsLeftEdge+gadget->BoundsWidth,
					gadget->BoundsTopEdge+gadget->BoundsHeight
				);
		}
		gadget = gadget->NextGadget;
		num --;
	}
}
#else
void clearWindow(struct Window *win) {

	EraseRect
	(
		win->RPort,
		win->BorderLeft,
		win->BorderTop,
		win->Width-win->BorderRight-1,
		win->Height-win->BorderBottom-1
	);
}
#endif

#define TAG_Left     0
#define TAG_Top      1
#define TAG_Width    2
#define TAG_Height   3
#define TAG_IText    4
#define TAG_Previous 5
#define TAG_ID       6
#define TAG_DrawInfo 7
#define TAG_UserData 8
#define TAG_Num      9

struct Gadget *ptgad=0;
struct DrawInfo *dri=0;

struct Gadget *makePTGadget(struct DrawInfo *dri, struct TagItem *stdgadtags) {
struct Gadget *obj;
Class *cl;
struct TagItem tags[]=
{
	{GA_Disabled, FALSE},
	{GA_Immediate, FALSE},
	{GA_RelVerify, TRUE},
	{TAG_MORE, (IPTR)NULL}
};

	cl = makePTClass();
	if (!cl)
		return 0;
	tags[0].ti_Data = FALSE; //GetTagData(GA_Disabled, FALSE, taglist);
	tags[1].ti_Data = TRUE; //GetTagData(GA_Immediate, FALSE, taglist);
	tags[3].ti_Data = (IPTR) stdgadtags;
	obj = (struct Gadget *) NewObjectA(cl, NULL, tags);
	return obj;
}

struct Gadget *createPTGadget(struct DrawInfo *dri) {
struct TagItem stdgadtags[] =
{
   {GA_Left	, 0L			},
	{GA_Top		, 0L			},
	{GA_Width	, 0L			},
	{GA_Height	, 0L			},
	{GA_IntuiText	, (IPTR)NULL		},
	{GA_Previous	, (IPTR)NULL		},
	{GA_ID		, 0L			},
	{GA_DrawInfo	, (IPTR)NULL		},
	{GA_UserData	, (IPTR)NULL		},
	{TAG_DONE				}
};

	stdgadtags[TAG_Left].ti_Data = 20;
	stdgadtags[TAG_Top].ti_Data = 30;
	stdgadtags[TAG_Width].ti_Data = 600;
	stdgadtags[TAG_Height].ti_Data = 30;
	stdgadtags[TAG_IText].ti_Tag = TAG_IGNORE;
	stdgadtags[TAG_ID].ti_Data = ID_PCP_PARTITION_GUI;
	stdgadtags[TAG_DrawInfo].ti_Data = (IPTR)dri;
	stdgadtags[TAG_UserData].ti_Data = 0;

	return makePTGadget(dri, &stdgadtags);
}

void allocPTGadget(struct Screen *scr, struct Gadget *glist) {

	dri = GetScreenDrawInfo(scr);
	if (dri)
	{
		ptgad = createPTGadget(dri);
		if (ptgad)
		{
			while (glist->NextGadget)
				glist = glist->NextGadget;
			glist->NextGadget = ptgad;
		}
	}
}

void freePTGadget(struct Screen *scr) {

	if (ptgad)
		DisposeObject(ptgad);
	if (dri)
		FreeScreenDrawInfo(scr, dri);
}


