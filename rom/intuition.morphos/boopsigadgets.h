#ifndef BOOPSIGADGETS_H
#define BOOPSIGADGETS_H
/*
    (C) 1995-96 AROS - The Amiga Research OS
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

extern void RefreshBoopsiGadget (struct Gadget * gadget, struct Window * window,
	                                 struct Requester *requester, struct IntuitionBase * IntuitionBase);

extern VOID DoGMLayout(struct Gadget *, struct Window *,struct Requester *,
	                       UWORD, BOOL, struct IntuitionBase *);

#endif /* BOOPSIGADGETS_H */

