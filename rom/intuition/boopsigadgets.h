#ifndef BOOPSIGADGETS_H
#define BOOPSIGADGETS_H
/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Routines for BOOPSI Gadgets
    Lang: english
*/
#ifndef INTUITION_INTUITION_H
#   include <intuition/intuition.h>
#endif
#ifndef INTUITION_INTUITIONBASE_H
#   include <intuition/intuitionbase.h>
#endif

extern void RefreshBoopsiGadget (struct Gadget * gadget,
	struct Window * window, struct IntuitionBase * IntuitionBase);

extern VOID DoGMLayout(struct Gadget *, struct Window *,struct Requester *,
		UWORD, BOOL, struct IntuitionBase *);

#endif /* BOOPSIGADGETS_H */

