/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#ifndef INTUITION_INTUITION_H
#   include <intuition/intuition.h>
#endif
#ifndef INTUITION_INTUITIONBASE_H
#   include <intuition/intuitionbase.h>
#endif
#ifndef INTUITION_SGHOOKS_H
#   include <intuition/sghooks.h>
#endif
#ifndef _GADGETS_H_
#   include "gadgets.h"
#endif


AROS_UFP3(ULONG, GlobalEditFunc,
          AROS_UFPA(struct Hook *,      hook,       A0),
          AROS_UFPA(struct SGWork *,        sgw,        A2),
          AROS_UFPA(ULONG *,            command,    A1)
         );


VOID RefreshStrGadget(struct Gadget *, struct Window *, struct Requester *,
                      struct IntuitionBase *);
VOID UpdateStrGadget(struct Gadget *, struct Window *, struct Requester *,
                     struct IntuitionBase *);
ULONG HandleStrInput(struct Gadget *, struct GadgetInfo *,
                     struct InputEvent *, UWORD *, struct IntuitionBase *);
