/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include "intuition_intern.h"
#include <proto/graphics.h>
#include <proto/layers.h>

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

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    /* Check whether the BeginRefresh was aborted due to a FALSE=BeginUpdate()*/
    if (window->Flags & WFLG_WINDOWREFRESH)
        EndUpdate(window->WLayer, complete);
    
    /* reset all bits indicating a necessary or ongoing refresh */
    window->Flags &= ~WFLG_WINDOWREFRESH;

    /* I reset this one only if Complete is TRUE!?! */
    if (TRUE == complete)
        window->WLayer->Flags &= ~LAYERREFRESH;

    /* Unlock the layers. */
    if (IS_GZZWINDOW(window))
        UnlockLayerRom(window->BorderRPort->Layer);

    UnlockLayerRom(window->WLayer);

#if USE_LOCKLAYERINFO_AS_REFRESHLOCK
    UnlockLayerInfo(&window->WScreen->LayerInfo);
#else
    #warning EndRefresh: Remove this ReleaseSem, if UNLOCK_REFRESH macro in inputhandler_actions.c is
    #warning             changed to do ReleaseSem(GadgetLock) + UnLockLayers()

    ReleaseSemaphore(&GetPrivScreen(window->WScreen)->RefreshLock);
#endif

    AROS_LIBFUNC_EXIT
} /* EndRefresh */
