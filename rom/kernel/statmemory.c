#include <exec/execbase.h>
#include <proto/exec.h>
#include <utility/tagitem.h>

#include <inttypes.h>

#include <kernel_base.h>
#include <kernel_tagitems.h>
#include "memory_intern.h"

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH2(void *, KrnStatMemory,

/*  SYNOPSIS */
	AROS_LHA(struct TagItem *, query, A0),
	AROS_LHA(ULONG, flags, D0),

/*  LOCATION */
	struct KernelBase *, KernelBase, 32, Kernel)

/*  FUNCTION
	Get various statistics about memory usage

    INPUTS
	query - An array of TagItems containing query specification. Each
		TagItem consists of tag ID and a pointer to IPTR value
		which will contain the result of the query.

		Available tag IDs are:

		KMS_Free          - Get amount of free memory in bytes
		KMS_Total         - Get total amount of memory in bytes
		KMS_LargestAlloc  - Get size of the largest allocated chunk in bytes
		KMS_SmallestAlloc - Get size of the smallest allocated chunk in bytes
		KMS_LargestFree   - Get size of the largest free chunk in bytes
		KMS_SmallestFree  - Get size of the smallest free chunk in bytes
		KMS_NumAlloc	  - Get number of allocated chunks
		KMS_NumFree	  - Get number of free chunks

	flags - Flags which specify physical properties of the memory to query.
		These are the same flags as passed to exec.library/AllocMem().

    RESULT
	None.

    NOTES
	For all unknown tag IDs result values will be set to 0.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    const struct TagItem *tstate = query;
    struct TagItem *tag;
    struct MemHeader *mh;

    /* Initialize all return values to zero */
    while ((tag = krnNextTagItem(&tstate)))
	*((IPTR *)tag->ti_Data) = 0;

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
	krnStatMemHeader(mh, query);
    }

    AROS_LIBFUNC_EXIT
}
