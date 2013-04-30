/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
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
	Finishes refreshing which was initialized with BeginRefresh().
	The argument |complete| is usually TRUE. It can be useful to
	set it to FALSE when refreshing is split into several tasks.

    INPUTS
	window   - the window to be refreshed
	complete - BOOL which states if all refreshing is done

    RESULT
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
	BeginRefresh()

    INTERNALS
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct LayersBase *LayersBase = GetPrivIBase(IntuitionBase)->LayersBase;
    DEBUG_REFRESH(dprintf("EndRefresh: Window 0x%lx Complete %d\n", window, complete));

    SANITY_CHECK(window)

    /* Check whether the BeginRefresh was aborted due to a FALSE=BeginUpdate()*/
    if (window->Flags & WFLG_WINDOWREFRESH)
        EndUpdate(WLAYER(window), complete);

    /* reset all bits indicating a necessary or ongoing refresh */
    AROS_ATOMIC_AND(window->Flags, ~WFLG_WINDOWREFRESH);

    /* I reset this one only if Complete is TRUE!?! */
    if (complete)
        WLAYER(window)->Flags &= ~LAYERREFRESH;

    /* Unlock the layers. */
    UnlockLayer(WLAYER(window));

    if (BLAYER(window))
        UnlockLayer(BLAYER(window));

    UnlockLayerInfo(&window->WScreen->LayerInfo);

    AROS_LIBFUNC_EXIT
} /* EndRefresh */
