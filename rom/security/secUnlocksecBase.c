/*
    Copyright © 2002-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <proto/security.h>

#include <stdio.h>

#include "security_intern.h"
#include "security_memory.h"

/*****************************************************************************

    NAME */
	AROS_LH1(void, secUnlocksecBase,

/*  SYNOPSIS */
	/* (muP) */
	AROS_LHA(struct secPointers *, secP, D0),

/*  LOCATION */
	struct SecurityBase *, secBase, 38, Security)

/*  FUNCTION

    INPUTS


    RESULT


    NOTES


    EXAMPLE

    BUGS

    SEE ALSO


    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    if (secgetuid() == secROOT_UID)
    {
        ReleaseSemaphore(&secBase->TaskOwnerSem);
        ReleaseSemaphore(&secBase->SegOwnerSem);
        ReleaseSemaphore(&secBase->MonitorSem);
        ReleaseSemaphore(&secBase->VolumesSem);
        ReleaseSemaphore(&secBase->SuperSem);
        FreeV(secP);
    }
    AROS_LIBFUNC_EXIT

} /* secUnlocksecBase */

