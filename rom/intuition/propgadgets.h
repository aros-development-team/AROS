#ifndef _PROPGADGETS_H_
#define _PROPGADGETS_H_
/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Header for PROP Gadgets
    Lang: english
*/
#ifndef INTUITION_INTUITION_H
#   include <intuition/intuition.h>
#endif
#ifndef INTUITION_INTUITIONBASE_H
#   include <intuition/intuitionbase.h>
#endif
#ifndef _GADGETS_H_
#   include "gadgets.h"
#endif


int CalcKnobSize (struct Gadget * propGadget, struct BBox * knobbox);
void RefreshPropGadget (struct Gadget * gadget, struct Window * window,
	struct IntuitionBase * IntuitionBase);
void RefreshPropGadgetKnob (struct Gadget * gadget, struct BBox * clear,
	struct BBox * knob, struct Window * window,
	struct IntuitionBase * IntuitionBase);

VOID HandlePropSelectDown (struct Gadget *gadget, struct Window	*win,
     struct Requester	*req, UWORD mouse_x, UWORD mouse_y,
     struct IntuitionBase *IntuitionBase);

VOID HandlePropSelectUp (struct Gadget *gadget, struct Window *w,
    struct Requester *req, struct IntuitionBase *IntuitionBase);	

VOID HandlePropMouseMove (struct Gadget	*gadget, struct Window *w,
    struct Requester *req, LONG dx, LONG dy,
    struct IntuitionBase *IntuitionBase);
#endif /* _PROPGADGETS_H_ */
