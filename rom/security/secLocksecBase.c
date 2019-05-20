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
	AROS_LH0(struct secPointers *, secLocksecBase,

/*  SYNOPSIS */
	/* void */

/*  LOCATION */
	struct SecurityBase *, secBase, 37, Security)

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

    struct secPointers *ptr;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    if (secgetuid() == secROOT_UID)
    {
        ObtainSemaphore(&secBase->SuperSem);
        ObtainSemaphore(&secBase->TaskOwnerSem);
        ObtainSemaphore(&secBase->SegOwnerSem);
        ObtainSemaphore(&secBase->MonitorSem);
        ObtainSemaphore(&secBase->VolumesSem);
        if ( (ptr = MAllocV(sizeof(struct secPointers))) )
        {
            ptr->Monitors = &secBase->MonitorList;
            ptr->Segments = &secBase->SegOwnerList;
            ptr->Sessions = &secBase->SessionsList;
            ptr->Tasks    = secBase->TaskOwnerList;
            ptr->Volumes  = secBase->Volumes;
            return ptr;
        }
    }
    return NULL;

    AROS_LIBFUNC_EXIT

} /* secLocksecBase */
