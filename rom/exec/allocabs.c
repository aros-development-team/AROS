/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
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
#include "exec_util.h"
#include "memory.h"
#include "mungwall.h"

/*****************************************************************************

    NAME */

	AROS_LH2(APTR, AllocAbs,

/*  SYNOPSIS */
	AROS_LHA(IPTR,  byteSize, D0),
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
       	This function may trash memory right after the allocation if it builds
    	a new MemChunk there. Additionally it will trash additional area before
    	and after the allocated space if mungwall is turned on. The additional
    	space will be used for mungwall service data.

	We can't just disable mungwalling for this function because in this case
	FreeMem() on AllocAbs()ed region will crash (FreeMem() will expect mungwall
	header attached to the chunk and there's no way to tell which function was
	used to allocate it.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    IPTR origSize = byteSize;
    APTR ret = NULL;
    struct TraceLocation tp = CURRENT_LOCATION("AllocAbs");

    /* Zero bytes requested? May return everything ;-). */
    if(!byteSize)
	return NULL;

    /* Make room for mungwall if needed */
    if (PrivExecBase(SysBase)->IntFlags & EXECF_MungWall)
    {
    	location -= MUNGWALL_BLOCK_SHIFT;
        byteSize += MUNGWALL_TOTAL_SIZE;
    }

    ret = InternalAllocAbs(location, byteSize, SysBase);

    D(bug("[AllocAbs] Location: requested 0x%p, actual 0x%p\n", location, ret));
    D(bug("[AllocAbs] Length: requested %u, actual %u\n", origSize, origSize + location - ret));
    /*
     * Starting address may have been adjusted, in this case 'ret' will
     * differ from 'location', and allocation length was in fact increased
     * by this difference.
     * We also specify MEMF_CLEAR because we do not want to destroy original
     * memory contents.
     */
    return MungWall_Build(ret, NULL, origSize + location - ret, MEMF_CLEAR, &tp, SysBase);

    AROS_LIBFUNC_EXIT
} /* AllocAbs */
