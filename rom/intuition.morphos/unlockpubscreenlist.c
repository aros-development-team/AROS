/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH0(VOID, UnlockPubScreenList,

         /*  SYNOPSIS */

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 88, Intuition)

/*  FUNCTION
 
    Release lock made by LockPubScreenList().
 
    INPUTS
 
    RESULT
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    LockPubScreenList()
 
    INTERNALS
 
    HISTORY
        21-06-98    SDuvan  Implemented
    29-10-95    digulla automatically created from
                intuition_lib.fd and clib/intuition_protos.h
 
*****************************************************************************/
#define GPB(x) GetPrivIBase(x)

{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    DEBUG_UNLOCKPUBSCREENLIST(dprintf("UnLockPubScreenList: <%s>\n",
                                      FindTask(NULL)->tc_Node.ln_Name));

    ReleaseSemaphore(&GPB(IntuitionBase)->PubScrListLock);

    DEBUG_UNLOCKPUBSCREENLIST(dprintf("UnLockPubScreenList: done\n"));

    AROS_LIBFUNC_EXIT
} /* UnlockPubScreenList */

