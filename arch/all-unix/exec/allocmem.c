/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
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
#include <aros/config.h>
#include <aros/arossupportbase.h>
#include <exec/memory.h>
#include <exec/memheaderext.h>
#include <exec/nodes.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>

#include <string.h>

#include "exec_debug.h"

#ifndef DEBUG_AllocMem
#   define DEBUG_AllocMem 0
#endif
#undef DEBUG
#if DEBUG_AllocMem
#   define DEBUG 1
#endif
#define MDEBUG 1
#   include <aros/debug.h>

#include "exec_intern.h"

/* See rom/exec/allocmem.c for documentation */

AROS_LH2(APTR, AllocMem,
    AROS_LHA(IPTR,  byteSize,     D0),
    AROS_LHA(ULONG, requirements, D1),
    struct ExecBase *, SysBase, 33, Exec)
{
    AROS_LIBFUNC_INIT

    APTR res;

    D(if (SysBase->DebugAROSBase))
    D(bug("Call AllocMem (%d, %08x)\n", byteSize, requirements));

    /* Zero bytes requested? May return everything ;-). */
    if(!byteSize)
	return NULL;

    byteSize += sizeof(APTR);

    if (!pool)
    if (!PrivExecBase(SysBase)->defaultPool)
    	/* If we don't have defaultPool, it's early boot mode */
       	res = allocBootMem((struct MemHeader *)SysBase->MemList.lh_Head, byteSize);
    else
    {
	APTR pool;

	/* TODO: in future we will have separate pool for MEMF_EXECUTABLE memory */
	pool = PrivExecBase(SysBase)->defaultPool;

    	res = AllocPooled(pool, byteSize);
    }

    if (res)
    {    	
    	if (requirements & MEMF_CLEAR)
	    memset(res, 0, byteSize);
#if ENABLE_RT
	RT_Add(RTT_MEMORY, res, byteSize);
#endif
    }
    else
    {
        /* Set DOS error if called from a process */
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
