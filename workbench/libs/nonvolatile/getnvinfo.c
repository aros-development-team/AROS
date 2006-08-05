/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$
*/
#include <dos/dosextens.h>

/*****************************************************************************

    NAME */

#include <libraries/nonvolatile.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/nvdisk.h>


AROS_LH1(struct NVInfo *, GetNVInfo,

/*  SYNOPSIS */

	AROS_LHA(BOOL, killRequesters, D1),

/*  LOCATION */

	struct Library *, nvBase, 9, Nonvolatile)

/*  FUNCTION

    Report information on the user's preferred nonvolatile storage device.

    INPUTS

    killRequesters  --  if TRUE no system requesters will be displayed during
                        the operation of this function

    RESULT

    Pointer to an NVInfo structure containing the information on the nonvolatile
    storage device currently in use. Returns NULL in case of a failure.
    
    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    FreeNVData(), StoreNV(), <libraries/nonvolatile.h>

    INTERNALS

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    struct Process *me = (struct Process *)FindTask(NULL);
    APTR oldReq = me->pr_WindowPtr;
    struct NVInfo *info = AllocVec(sizeof(struct NVInfo), MEMF_ANY);

    if(info == NULL)
	return NULL;

    if(killRequesters)
	me->pr_WindowPtr = (APTR)-1;

    /* Try to get the information from the HIDD */
    if(MemInfo(info))
    {
	/* Round down to nearest 10 bytes */
	info->nvi_MaxStorage  = (info->nvi_MaxStorage/10)*10;
	info->nvi_FreeStorage = (info->nvi_FreeStorage/10)*10;
    }
    else
    {
	FreeVec(info);
	info = NULL;
    }
    
    if(killRequesters)
	me->pr_WindowPtr = oldReq;

    return info;

    AROS_LIBFUNC_EXIT
} /* GetNVInfo */

