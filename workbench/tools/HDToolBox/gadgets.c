/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <aros/debug.h>
#include "gadgets.h"

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
