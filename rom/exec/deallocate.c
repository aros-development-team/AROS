/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Free memory allocated by Allocate().
    Lang: english
*/

#include <aros/debug.h>
#include <exec/execbase.h>
#include <exec/alerts.h>
#include <aros/libcall.h>
#include <exec/memory.h>
#include <exec/memheaderext.h>
#include <proto/exec.h>

#include "exec_util.h"
#include "memory.h"

/*****************************************************************************

    NAME */

	AROS_LH3(void, Deallocate,

/*  SYNOPSIS */
	AROS_LHA(struct MemHeader *, freeList,    A0),
	AROS_LHA(APTR,               memoryBlock, A1),
	AROS_LHA(IPTR,               byteSize,    D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 32, Exec)

/*  FUNCTION
	Free block of memory associated with a given MemHandler structure.

    INPUTS
	freeList    - Pointer to the MemHeader structure
	memoryBlock - Pointer to the memory to be freed
	byteSize    - Size of the block

    RESULT

    NOTES
	The start and end borders of the block are aligned to
	a multiple of sizeof(struct MemChunk) and to include the block.

    EXAMPLE

    BUGS

    SEE ALSO
	Allocate()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if ((freeList->mh_Node.ln_Type == NT_MEMORY) && IsManagedMem(freeList))
    {
        struct MemHeaderExt *mhe = (struct MemHeaderExt *)freeList;

        if (mhe->mhe_Free)
            mhe->mhe_Free(mhe, memoryBlock, byteSize);
    }
    else
    {
        struct TraceLocation tp = CURRENT_LOCATION("Deallocate");

        /* If there is no memory free nothing */
        if(!byteSize || !memoryBlock)
            return;

#if !defined(NO_CONSISTENCY_CHECKS)
        /* Test if our block really fits into this MemHeader. */
        if ((memoryBlock < freeList->mh_Lower) || (memoryBlock + byteSize > freeList->mh_Upper + 1))
        {
            /* Something is completely wrong. */
            bug("[MM] Memory allocator error\n");
            bug("[MM] Attempt to free %u bytes at 0x%p from MemHeader 0x%p\n", byteSize, memoryBlock, freeList);
            bug("[MM] Block does not fit into MemHeader (0x%p - 0x%p)\n", freeList->mh_Lower, freeList->mh_Upper);

            Alert(AN_BadFreeAddr);
            return;
        }
#endif

        stdDealloc(freeList, NULL /* by design */, memoryBlock, byteSize, &tp, SysBase);

    }

    AROS_LIBFUNC_EXIT
} /* Deallocate */
