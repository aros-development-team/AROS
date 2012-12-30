/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Allocate some memory
    Lang: english
*/

#include <exec/alerts.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/rt.h>
#include <aros/macros.h>
#include <aros/arossupportbase.h>
#include <exec/memory.h>
#include <exec/memheaderext.h>
#include <exec/nodes.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>

#include "exec_debug.h"

#ifndef DEBUG_AllocMem
#   define DEBUG_AllocMem 0
#endif
#undef DEBUG
#if DEBUG_AllocMem
#   define DEBUG 1
#endif

#include "exec_intern.h"
#include "exec_util.h"
#include "memory.h"
#include "mungwall.h"

/*****************************************************************************

    NAME */

	AROS_LH2(APTR, AllocMem,

/*  SYNOPSIS */
	AROS_LHA(IPTR,  byteSize,     D0),
	AROS_LHA(ULONG, requirements, D1),

/* LOCATION */
	struct ExecBase *, SysBase, 33, Exec)

/*  FUNCTION
	Allocate some memory from the sytem memory pool with the given
	requirements.

    INPUTS
	byteSize     - Number of bytes you want to get
	requirements - Type of memory

    RESULT
	A pointer to the number of bytes you wanted or NULL if the memory
	couldn't be allocated

    NOTES
	The memory is aligned to sizeof(struct MemChunk). All requests
	are rounded up to a multiple of that size.

    EXAMPLE
	mytask=(struct Task *)AllocMem(sizeof(struct Task),MEMF_PUBLIC|MEMF_CLEAR);

    BUGS

    SEE ALSO
	FreeMem()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    APTR res = NULL;
    struct checkMemHandlersState cmhs;
    IPTR origSize = byteSize;
    struct TraceLocation loc = CURRENT_LOCATION("AllocMem");

    D(if (SysBase->DebugAROSBase))
    D(bug("Call AllocMem (%d, %08x)\n", byteSize, requirements));

    /* Zero bytes requested? May return everything ;-). */
    if(!byteSize)
	return NULL;

    /* Make room for safety walls around allocated block and an some more extra space
       for other interesting things, actually --> the size.

       This all will look like this:

       [MEMCHUNK_FOR_EXTRA_STUFF][BEFORE-MUNGWALL][<alloced-memory-for-user>][AFTER_MUNGWALL]

       MEMCHUNK_FOR_EXTRA_STUFF is used (amongst other things) to save the original alloc
       size (byteSize) param. So it is possible in FreeMem to check, if freemem size
       matches allocmem size or not.
    */
    if (PrivExecBase(SysBase)->IntFlags & EXECF_MungWall)
        byteSize += MUNGWALL_TOTAL_SIZE;

    cmhs.cmhs_CurNode                = (struct Node *)SysBase->ex_MemHandlers.mlh_Head;
    cmhs.cmhs_Data.memh_RequestSize  = byteSize;
    cmhs.cmhs_Data.memh_RequestFlags = requirements;
    cmhs.cmhs_Data.memh_Flags        = 0;

    do
    {
	res = nommu_AllocMem(byteSize, requirements, &loc, SysBase);
    } while (res == NULL && checkMemHandlers(&cmhs, SysBase) == MEM_TRY_AGAIN);

#if ENABLE_RT
    RT_Add (RTT_MEMORY, res, origSize);
#endif  

    res = MungWall_Build(res, NULL, origSize, requirements, &loc, SysBase);

    /* Set DOS error if called from a process */
    if (res == NULL)
    {
        struct Process *process = (struct Process *)FindTask(NULL);
        if (process->pr_Task.tc_Node.ln_Type == NT_PROCESS)
            process->pr_Result2 = ERROR_NO_FREE_STORE;
    }

#if DEBUG
    if (SysBase->DebugAROSBase)
	bug("AllocMem result: 0x%p\n", res);
#endif
    return res;
    
    AROS_LIBFUNC_EXIT
    
} /* AllocMem */
