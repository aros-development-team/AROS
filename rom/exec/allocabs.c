/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Allocate memory at address
    Lang: english
*/

/* Needed for mungwall macros to work */
#define MDEBUG 1

#include <aros/debug.h>
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/memheaderext.h>
#include <proto/exec.h>

#include "exec_intern.h"
#include "memory.h"

/*****************************************************************************

    NAME */

	AROS_LH2(APTR, AllocAbs,

/*  SYNOPSIS */
	AROS_LHA(ULONG, byteSize, D0),
	AROS_LHA(APTR,  location, D1),

/*  LOCATION */
	struct ExecBase *, SysBase, 34, Exec)

/*  FUNCTION
	Allocate some memory from the system memory pool at a given address.

    INPUTS
	byteSize - Number of bytes you want to get
	location - Where you want to get the memory

    RESULT
	A pointer to some memory including the requested bytes or NULL if
	the memory couldn't be allocated

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	FreeMem()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct MemHeader *mh;
    IPTR origSize = byteSize;
    APTR ret = NULL;
    
    /* Zero bytes requested? May return everything ;-). */
    if(!byteSize)
	return NULL;

    /* Make room for mungwall if needed */
    if (PrivExecBase(SysBase)->IntFlags & EXECF_MungWall)
    {
    	location -= MUNGWALL_SIZE + MUNGWALLHEADER_SIZE;
        byteSize += MUNGWALL_SIZE * 2 + MUNGWALLHEADER_SIZE;
    }

    /* Protect the memory list from access by other tasks. */
    Forbid();

    /* Loop over MemHeader structures */
    ForeachNode(&SysBase->MemList, mh)
    {
        if (mh->mh_Lower <= location && mh->mh_Upper > location)
            break;
    }
    
    /* If no header was found which matched the requirements, just give up. */
    if (!mh->mh_Node.ln_Succ)
    {
        Permit();
        return NULL;
    }

    /* If the header is managed, let the manager handle the request.  */    
    if (mh->mh_Attributes & MEMF_MANAGED)
    {
        struct MemHeaderExt *mhe = (struct MemHeaderExt *)mh;

        if (mhe->mhe_AllocAbs)
	    ret = mhe->mhe_AllocAbs(mhe, byteSize, location);
    }
    else
    	ret = AllocateExt(mh, location, byteSize, 0);

    Permit();

    return MungWall_Build(ret, origSize + location - ret, 0, SysBase);
    
    AROS_LIBFUNC_EXIT
} /* AllocAbs */

