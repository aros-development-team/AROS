/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/layers.h>
#include <proto/graphics.h>
#include "intuition_intern.h"
#include "inputhandler_actions.h"
#include "inputhandler_support.h"

#ifdef SKINS
#   include "renderwindowframe.h"
#endif

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

AROS_LH1(void, BeginRefresh,

         /*  SYNOPSIS */
         AROS_LHA(struct Window *, window, A0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 59, Intuition)

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

    ULONG mode = NO_DOUBLEBUFFER;

#ifdef BEGINUPDATEGADGETREFRESH
    BOOL gadgetrefresh = FALSE;
#endif

    DEBUG_REFRESH(dprintf("BeginRefresh: Window 0x%lx\n", window));

    SANITY_CHECK(window)

    //jDc: makes sure we've got 1 refresing window at a time
    LockLayerInfo(&window->WScreen->LayerInfo);

    /* Find out whether it's a GimmeZeroZero window with an extra layer to lock */
    if (BLAYER(window))
        LockLayer(0, BLAYER(window));

    /* jDc: in actual implementation border layer is created as the 1st one. this means it's added to
    ** screens layer semaphore list at the end (layers use AddTail), so here we also need to lock it
    ** as 1st one, otherwise we run into a deadlock with LockLayers() !!!
    */

    LockLayer(0, WLAYER(window));

    /* jDc: in current opaque implementation the damaged regions are added to
    ** window's internal damage list and matched against actual damage here
    */

    if (IW(window)->specialflags & SPFLAG_WANTBUFFER) mode = DOUBLEBUFFER;

    /* I don't think I ever have to update the BorderRPort's layer */
    if (FALSE == BeginUpdate(WLAYER(window)))
    {
        EndUpdate(WLAYER(window), FALSE);

        //dprintf("%s :BeginUpdate returned FALSE!->Aborting BeginUpdate()\n",__FUNCTION__);
        return;
    }

    /* jDc: because with opaque move window borders/gadgets are not refreshed to
    ** speed things up we do this here - this reduces cpu time spent in inputhandler
    */

    /* let the user know that we're currently doing a refresh */
    AROS_ATOMIC_OR(window->Flags, WFLG_WINDOWREFRESH);

    AROS_LIBFUNC_EXIT
} /* BeginRefresh */
