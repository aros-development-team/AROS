#ifndef _BOOLGADGETS_H_
#define _BOOLGADGETS_H_
/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
 
    Desc: Routines for BOOL Gadgets
    Lang: english
*/
#ifndef INTUITION_INTUITION_H
#   include <intuition/intuition.h>
#endif
#ifndef INTUITION_INTUITIONBASE_H
#   include <intuition/intuitionbase.h>
#endif

void RefreshBoolGadget (struct Gadget * gadget, struct Window * window,
                        struct Requester * requester, struct IntuitionBase * IntuitionBase);

void RefreshBoolGadgetState(struct Gadget * gadget, struct Window * window,
                            struct Requester *requester, struct IntuitionBase *IntuitionBase);

#endif /* _BOOLGADGETS_H_ */

