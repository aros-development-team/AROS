/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Include for AROS GTYP_STRGADGET.
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



VOID RefreshStrGadget(struct Gadget *, struct Window *, struct IntuitionBase *);
VOID UpdateStrGadget(struct Gadget *, struct Window *, struct IntuitionBase *);
ULONG HandleStrInput(struct Gadget *, struct GadgetInfo	*,
		struct InputEvent *, UWORD *, struct IntuitionBase *);
