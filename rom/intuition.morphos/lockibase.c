/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
 
    Locks the access to all public fields of the IntuitionBase.
*/

#include <proto/exec.h>
#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH1(ULONG, LockIBase,

         /*  SYNOPSIS */
         AROS_LHA(ULONG, What, D0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 69, Intuition)

/*  FUNCTION
    Locks Intuition. While you hold this lock, no fields of Intuition
    will change. Please release this as soon as possible.
 
    INPUTS
    What - Which fields of Intuition should be locked. The only allowed
        value for this is currently 0 which means to lock everything.
 
    RESULT
    The result of this function must be passed to UnlockIBase().
 
    NOTES
    You *must not* call this function if you have any locks on other
    system resources like layers and LayerInfo locks.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    UnLockIBase()
 
    INTERNALS
 
    HISTORY
    29-10-95    digulla automatically created from
                intuition_lib.fd and clib/intuition_protos.h
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    ObtainSemaphore (GetPrivIBase(IntuitionBase)->IBaseLock);

    return What;
    AROS_LIBFUNC_EXIT
} /* LockIBase */
