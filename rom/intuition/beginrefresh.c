/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include "intuition_intern.h"
#include <proto/layers.h>
#include <proto/graphics.h>

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

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

#if USE_LOCKLAYERINFO_AS_REFRESHLOCK
    LockLayerInfo(&window->WScreen->LayerInfo);
#else
    #warning BeginRefresh: Remove this ObtainSem, if LOCK_REFRESH macro in inputhandler_actions.c is
    #warning               changed to do ObtainSem(GadgetLock) + LockLayers()
    
    ObtainSemaphore(&GetPrivScreen(window->WScreen)->RefreshLock);
#endif    

#if !USE_LOCKLAYERINFO_AS_REFRESHLOCK       
    /* lock all necessary layers. We have to use LockLayerInfo first because
       for a GZZ window 2 layers are locked. */
    LockLayerInfo(window->WLayer->LayerInfo);
#endif
       
    LockLayerRom(window->WLayer);

    /* Find out whether it's a GimmeZeroZero window with an extra layer to lock */
    if (IS_GZZWINDOW(window))
        LockLayerRom(window->BorderRPort->Layer);

#if !USE_LOCKLAYERINFO_AS_REFRESHLOCK       
    /* The layerinfo lock can be released when the layers are locked */    
    UnlockLayerInfo(window->WLayer->LayerInfo);
#endif
    
    /* I don't think I ever have to update the BorderRPort's layer */
    if (FALSE == BeginUpdate(window->WLayer))
    {
        EndUpdate(window->WLayer, FALSE);
        //kprintf("%s :BeginUpdate returned FALSE!->Aborting BeginUpdate()\n",__FUNCTION__);
        return;
    }

    /* let the user know that we're currently doing a refresh */
    window->Flags |= WFLG_WINDOWREFRESH;


    AROS_LIBFUNC_EXIT
} /* BeginRefresh */
