/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/layers.h>
#include <proto/graphics.h>
#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH2(void, EndRefresh,

         /*  SYNOPSIS */
         AROS_LHA(struct Window *, window, A0),
         AROS_LHA(BOOL           , complete, D0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 61, Intuition)

/*  FUNCTION
 
    INPUTS
 
    RESULT
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    INTERNALS
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    DEBUG_REFRESH(dprintf("EndRefresh: Window 0x%lx Complete %d\n", window, complete));

    SANITY_CHECK(window)

    /* Check whether the BeginRefresh was aborted due to a FALSE=BeginUpdate()*/
    if (window->Flags & WFLG_WINDOWREFRESH)
        EndUpdate(WLAYER(window), complete);

    /* reset all bits indicating a necessary or ongoing refresh */
    AROS_ATOMIC_AND(window->Flags, ~WFLG_WINDOWREFRESH);

    /* I reset this one only if Complete is TRUE!?! */
    if (TRUE == complete)
        WLAYER(window)->Flags &= ~LAYERREFRESH;

    /* Unlock the layers. */
    UnlockLayer(WLAYER(window));

    if (BLAYER(window))
        UnlockLayer(BLAYER(window));

    UnlockLayerInfo(&window->WScreen->LayerInfo);

    AROS_LIBFUNC_EXIT
} /* EndRefresh */
