/*
    Copyright © 2002-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <proto/exec.h>

#include "security_intern.h"
#include "security_enforce.h"

/*****************************************************************************

    NAME */
	AROS_LH6(LONG, secAccess_Control,

/*  SYNOPSIS */
	/* (fs, task, owner, prot) */
	AROS_LHA(ULONG, contextflags, D1),
	AROS_LHA(APTR, context, A1),
	AROS_LHA(struct secExtOwner *, task, A2),
	AROS_LHA(ULONG, objectowner, D2),
	AROS_LHA(LONG, objectprot, D3),
	AROS_LHA(LONG, access_type, D4),

/*  LOCATION */
	struct SecurityBase *, secBase, 33, Security)

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

    struct secVolume *vol = NULL;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    switch (contextflags)	{
        case secAC_FILESYSTEM_CONTEXT:
            {
                /* Context the a msgport of a filesystem */
                if (context != NULL)	{
                    /* Find the secVolume for the FileSystem */
                    ObtainSemaphore(&secBase->VolumesSem);
                    vol = secBase->Volumes;
                    while(vol)	{
                        if (vol->Process == (struct MsgPort*)context)
                            break;
                        vol = vol->Next;
                    }
                    ReleaseSemaphore(&secBase->VolumesSem);
                } 
            }
            break;
        case secAC_IGNORE_CONTEXT:
        default:
            /* Allow tasks other than filesystems to use this function */
            vol = NULL;
    }
    /* Pass it on to the workhorse */
    return IsAllowed(secBase, vol, task, objectowner, objectprot, access_type);

    AROS_LIBFUNC_EXIT

} /* secAccess_Control */

