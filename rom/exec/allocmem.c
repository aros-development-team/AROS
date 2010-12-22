/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
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
#include "memory.h"

struct checkMemHandlersState
{
    struct Node           *cmhs_CurNode;
    struct MemHandlerData  cmhs_Data;
};

static ULONG checkMemHandlers(struct checkMemHandlersState *cmhs);

/*****************************************************************************

    NAME */

	AROS_LH2(APTR, AllocMem,

/*  SYNOPSIS */
	AROS_LHA(ULONG, byteSize,     D0),
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
    ULONG origSize         = byteSize;
    ULONG origRequirements = requirements;

    D(if (SysBase->DebugAROSBase))
    D(bug("Call AllocMem (%d, %08x)\n", byteSize, requirements));

    /* Zero bytes requested? May return everything ;-). */
    if(!byteSize)
	return NULL;

#if AROS_MUNGWALL_DEBUG
    /* Backwards compatibility hack for ports whose exec init code
       does not set this flag. If should be set BEFORE THE FIRST ALLOCMEM,
       otherwise FreeMem() will crash on block allocated without walls */
    PrivExecBase(SysBase)->IntFlags = EXECF_MungWall;
#endif

    /* Make room for safety walls around allocated block and an some more extra space
       for other interesting things, actually --> the size.

       This all will look like this:

       [MEMCHUNK_FOR_EXTRA_STUFF][BEFORE-MUNGWALL][<alloced-memory-for-user>][AFTER_MUNGWALL]

       The first ULONG in MEMCHUNK_FOR_EXTRA_STUFF is used to save the original alloc
       size (byteSize) param. So it is possible in FreeMem to check, if freemem size
       matches allocmem size or not.
    */
    if (PrivExecBase(SysBase)->IntFlags & EXECF_MungWall)
        byteSize += MUNGWALL_SIZE * 2 + MUNGWALLHEADER_SIZE;

    cmhs.cmhs_CurNode                = (struct Node *)SysBase->ex_MemHandlers.mlh_Head;
    cmhs.cmhs_Data.memh_RequestSize  = byteSize;
    cmhs.cmhs_Data.memh_RequestFlags = requirements;
    cmhs.cmhs_Data.memh_Flags        = 0;
    
    do
    {
	res = nommu_AllocMem(byteSize, requirements, SysBase);

    } while (res == NULL && checkMemHandlers(&cmhs) == MEM_TRY_AGAIN);

    if(res && (requirements & MEMF_CLEAR))
        memset(res, 0, byteSize);        

#if ENABLE_RT
    RT_Add (RTT_MEMORY, res, origSize);
#endif  

    res = MungWall_Build(res, origSize, origRequirements, SysBase);

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

ULONG checkMemHandlers(struct checkMemHandlersState *cmhs)
{
    struct Node      *tmp;
    struct Interrupt *lmh;
    
    if (cmhs->cmhs_Data.memh_RequestFlags & MEMF_NO_EXPUNGE)
        return MEM_DID_NOTHING;
   
    /* Loop over low memory handlers. Handlers can remove
       themselves from the list while being invoked, thus
       we need to be careful! */
    for
    (
        lmh = (struct Interrupt *)cmhs->cmhs_CurNode;
        (tmp = lmh->is_Node.ln_Succ);
        lmh = (struct Interrupt *)(cmhs->cmhs_CurNode = tmp)
    )
    {
        ULONG ret;
        
        ret = AROS_UFC3 (LONG, lmh->is_Code,
                   AROS_UFCA(struct MemHandlerData *, &cmhs->cmhs_Data, A0),
                   AROS_UFCA(APTR,                     lmh->is_Data,    A1),
                   AROS_UFCA(struct ExecBase *,        SysBase,         A6)
              );

        if (ret == MEM_TRY_AGAIN)
        {
            /* MemHandler said he did something. Try again. */
            /* Is there any program that depends on this flag??? */
            cmhs->cmhs_Data.memh_Flags |= MEMHF_RECYCLE;
            return MEM_TRY_AGAIN;
        }
        else
        {
            /* Nothing more to expect from this handler. */
            cmhs->cmhs_Data.memh_Flags &= ~MEMHF_RECYCLE;
        }
    }
    
    return MEM_DID_NOTHING;
}
