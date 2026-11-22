/*
    Copyright (C) 2011-2026, The AROS Development Team. All rights reserved.

    Desc: Create a seglist for ROM code.
*/

#define AROS_LIBREQ(base,ver)   /* We test for versions manually */

#include <aros/debug.h>
#include <aros/kernel.h>
#include <proto/exec.h>
#include <proto/kernel.h>

/*****************************************************************************

    NAME */
#include <proto/arossupport.h>

        BPTR __CreateSegList(

/*  SYNOPSIS */
        APTR function, struct ExecBase *SysBase )

/*  LOCATION */

/*  FUNCTION
        Create a SegList, which contains a call to 'function'

    INPUTS
        function - Function to call when the SegList is executed

    RESULT
        BPTR to the SegList that was allocated. This SegList can
             be freed by DOS/UnloadSeg. If not enough memory,
             BNULL will be returned.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

        dos.library/UnloadSeg()

    INTERNALS

*****************************************************************************/
{
    /* One ordinary DOS segment (dos/bptr.h) holding the jump vector, so
       that dos.library/UnLoadSeg() can release it again */
    struct AROSSegment *segtmp;
    struct FullJumpVec *Code;   /* Code to jump to the offset */
    ULONG segsize = AROS_SEG_MEMSIZE4LEN(sizeof(*Code));
#if defined(__AROSPLATFORM_WXSEG__)
    /*
     * The seg's jump vector is executed, so it must live in executable memory.
     * On hosts that enforce W^X the general RAM pool is read/write only, and a
     * writable+executable mapping is refused -- so allocate a dedicated page
     * R/W, build the vector, then switch it to R/X. KrnAllocPages returns
     * page-granular memory, which keeps the protection change from affecting
     * other allocations. Falls back to AllocMem if kernel.resource isn't up;
     * UnLoadSeg tells the two allocation kinds apart via TypeOfMem().
     */
    struct KernelBase *KernelBase = OpenResource("kernel.resource");

    segtmp = NULL;
    if (KernelBase)
        segtmp = KrnAllocPages(NULL, segsize, MEMF_ANY);
    if (segtmp == NULL)
    {
        KernelBase = NULL;
        segtmp = AllocMem(segsize, MEMF_ANY);
    }
#else
    /* Regular memory is executable on this platform */
    segtmp = AllocMem(segsize, MEMF_ANY);
#endif
    if (!segtmp)
        return BNULL;

    Code = (struct FullJumpVec *)segtmp->seg_Data;
    segtmp->seg_Size = segsize;
    segtmp->seg_Next = BNULL;
    __AROS_SET_FULLJMP(Code, function);

    if (SysBase->LibNode.lib_Version >= 36)
        CacheClearE(Code, sizeof(*Code), CACRF_ClearI | CACRF_ClearD);

#if defined(__AROSPLATFORM_WXSEG__)
    /* Flip the populated page from R/W to R/X */
    if (KernelBase)
        KrnSetProtection(segtmp, segsize, MAP_Readable | MAP_Executable);
#endif

    D(bug("[CreateSegList] Created jump segment 0x%p, code 0x%p, target 0x%p\n", AROS_SEG_TOBPTR(segtmp), Code, function));

    return AROS_SEG_TOBPTR(segtmp);
}
