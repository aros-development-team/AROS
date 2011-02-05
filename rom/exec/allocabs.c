/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Allocate memory at address
    Lang: english
*/

#include <aros/debug.h>
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/memheaderext.h>
#include <proto/exec.h>

#include "exec_intern.h"
#include "memory.h"
#include "mungwall.h"

/*****************************************************************************

    NAME */

	AROS_LH2(APTR, AllocAbs,

/*  SYNOPSIS */
	AROS_LHA(ULONG, byteSize, D0),
	AROS_LHA(APTR,  location, A1),

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

    IPTR origSize = byteSize;
    APTR ret = NULL;

    /* Zero bytes requested? May return everything ;-). */
    if(!byteSize)
	return NULL;

    /* Make room for mungwall if needed */
    if (PrivExecBase(SysBase)->IntFlags & EXECF_MungWall)
    {
    	location -= MUNGWALL_BLOCK_SHIFT;
        byteSize += MUNGWALL_TOTAL_SIZE;
    }

    ret = nommu_AllocAbs(location, byteSize, SysBase);

    D(bug("[AllocAbs] Location: requested 0x%p, actual 0x%p\n", location, ret));
    D(bug("[AllocAbs] Length: requested %u, actual %u\n", origSize, origSize + location - ret));
    /*
     * Starting address may have been adjusted, in this case 'ret' will
     * differ from 'location', and allocation length was in fact increased
     * by this difference.
     */
    return MungWall_Build(ret, NULL, origSize + location - ret, 0, SysBase);

    AROS_LIBFUNC_EXIT
} /* AllocAbs */

