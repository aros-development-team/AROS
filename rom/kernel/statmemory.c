#include <aros/config.h>
#include <exec/execbase.h>
#include <proto/arossupport.h>
#include <proto/exec.h>
#include <utility/tagitem.h>

#include <inttypes.h>

#include <kernel_base.h>
#include <kernel_mm.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH2(ULONG, KrnStatMemoryA,

/*  SYNOPSIS */
	AROS_LHA(ULONG, flags, D0),
	AROS_LHA(struct TagItem *, query, A0),

/*  LOCATION */
	struct KernelBase *, KernelBase, 32, Kernel)

/*  FUNCTION
	Get various statistics about memory usage

    INPUTS
	query - An array of TagItems containing query specification. Each
		TagItem consists of tag ID and a pointer to a value of
		specified type which will contain the result of the query.

		Available tag IDs are:

		KMS_Free          (IPTR)  - Get amount of free memory in bytes
		KMS_Total         (IPTR)  - Get total amount of memory in bytes
		KMS_LargestAlloc  (IPTR)  - Get size of the largest allocated chunk in bytes
		KMS_SmallestAlloc (IPTR)  - Get size of the smallest allocated chunk in bytes
		KMS_LargestFree   (IPTR)  - Get size of the largest free chunk in bytes
		KMS_SmallestFree  (IPTR)  - Get size of the smallest free chunk in bytes
		KMS_NumAlloc	  (IPTR)  - Get number of allocated chunks
		KMS_NumFree	  (IPTR)  - Get number of free chunks
		KMS_PageSize	  (ULONG) - Get memory page size

	flags - Flags which specify physical properties of the memory to query.
		These are the same flags as passed to exec.library/AllocMem().

    RESULT
	TRUE if the function worked, FALSE if MMU is not up and running on the system.
	If the function returns FALSE, values will stay uninitialized.

    NOTES
	For all unknown tag IDs result values will be set to 0.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

#if USE_MMU
    if (KernelBase->kb_PageSize)
    {
        struct TagItem *tag, *tstate = query;
	struct MemHeader *mh;
	BOOL do_traverse = FALSE;

	while ((tag = LibNextTagItem(&tstate)))
	{
	    switch (tag->ti_Tag)
	    {
	    case KMS_PageSize:
		*((ULONG *)tag->ti_Data) = KernelBase->kb_PageSize;
		break;

	    default:
	        /* Initialize all accumulated values to zero */
		*((IPTR *)tag->ti_Data) = 0;
		do_traverse = TRUE;
	    }
	}

	/* If we needed only page size, just return */
	if (!do_traverse)
	    return TRUE;

	/* Leave only flags that describe physical properties of the memory */
	flags &= MEMF_PHYSICAL_MASK;

        /*
         * Loop over MemHeader structures.
         * We only add MemHeaders and never remove them, so i hope Forbid()/Permit()
         * is not really necessary here.
         */
	ForeachNode(&SysBase->MemList, mh)
	{
	    /*
	     * Check for the right requirements and enough free memory.
	     * The requirements are OK if there's no bit in the
	     * 'flags' that isn't set in the 'mh->mh_Attributes'.
	     */
	    if (flags & ~mh->mh_Attributes)
		continue;

	    /* Get statistics. Total values will be summed up. */
	    mm_StatMemHeader(mh, query, KernelBase);
	}
    
	return TRUE;
    }
#endif
    return FALSE;

    AROS_LIBFUNC_EXIT
}
