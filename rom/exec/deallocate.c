/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Free memory allocated by Allocate().
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

        Freeing blocks back to a private region restores its original free
        count, including a terminal remainder smaller than the alignment.

        Calls on an unmanaged region use the same task synchronisation as
        Allocate(), preserving the caller's task-switch nesting without
        explicitly rescheduling on exit. Interrupt access requires external
        exclusion; see Allocate().

        On SMP builds this function serialises through the header's
        embedded spinlock (mh_SpinLock) - see the corresponding note in
        Allocate(): a hand-built MemHeader must be zeroed before its
        fields are filled in, or the lock word contains garbage and
        this call spins forever.

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

        stdDealloc(freeList, NULL /* by design */, memoryBlock, byteSize, &tp, SysBase);

    }

    AROS_LIBFUNC_EXIT
} /* Deallocate */
