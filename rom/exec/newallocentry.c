/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Allocate memory.
    Lang: english
*/
#include <aros/config.h>
#include "exec_intern.h"
#include <aros/libcall.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include "exec_debug.h"
#ifndef DEBUG_NewAllocEntry
#   define DEBUG_NewAllocEntry 0
#endif
#undef DEBUG
#if DEBUG_NewAllocEntry
#   define DEBUG 1
#endif
#include <aros/debug.h>
#undef kprintf

/*****************************************************************************

    NAME */

	AROS_LH2(struct MemList *, NewAllocEntry,

/*  SYNOPSIS */
	AROS_LHA(struct MemList *, entry, A0),
	AROS_LHA(ULONG *, return_flags, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 174, Exec)

/*  FUNCTION
	Allocate a number of memory blocks through a MemList structure.

    INPUTS
	entry        - The MemList with one MemEntry for each block you want to get
	return_entry - Pointer to struct MemList *variable where the address
	               of the MemList allocated by this function will be stored.
	return_flags - Pointer to ULONG variable where upon failure the type of
	               memory that could not be allocated is stored. You may pass
		       NULL here.

    RESULT
    	Address of the allocated MemList if the allocation was successful. In this
    	case *return_flags will be set to 0.

	NULL if the allocation failed. In this case *return_flags will contain the
	type of memory that couldn't be allocated.

    NOTES
    	This function is AROS-specific.

    EXAMPLE

    BUGS

    SEE ALSO
	AllocEntry(), FreeEntry()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct MemList *ret;
    ULONG   	    mlsize, i;

    D(bug("NewAllocEntry $%lx num=%d\ttask=\"%s\"\n", entry, entry->ml_NumEntries, SysBase->ThisTask->tc_Node.ln_Name));

    D(
      for(i = 0; i < entry->ml_NumEntries; i++)
      {
          kprintf("\treq $%lx\tsize $%lx\n", entry->ml_ME[i].me_Reqs, entry->ml_ME[i].me_Length);
      }
    )

    /* Calculate size of a MemList with ml_NumEntries MemEntries. */
    mlsize = sizeof(struct MemList) - sizeof(struct MemEntry) +
	     sizeof(struct MemEntry) * entry->ml_NumEntries;

    /* Get the MemList structure */
    ret = (struct MemList *)AllocMem(mlsize, MEMF_PUBLIC);

    /* The allocation failed? Return "no public memory" */
    if(ret == NULL)
    {
	if (return_flags) *return_flags = MEMF_PUBLIC;
	return NULL;
    }
    
    /* Init new struct */
    ret->ml_NumEntries	 = entry->ml_NumEntries;
    ret->ml_Node.ln_Type = 0;
    ret->ml_Node.ln_Pri  = 0;
    ret->ml_Node.ln_Name = NULL;

    /* Fill all entries */
    for(i = 0; i < entry->ml_NumEntries; i++)
    {
	/*
	    A compatibility kludge: some programs rely that
	    AllocEntry() doesn't fail if the length field is 0.

	    E.g. CrossDos' PCx wants to allocate 7 memory regions, but the
	    last two fields are empty.

            Don't depend on this feature.
	*/
	if(entry->ml_ME[i].me_Length)
	{
	    /* Get one */
	    ret->ml_ME[i].me_Addr = AllocMem(entry->ml_ME[i].me_Length,
				             entry->ml_ME[i].me_Reqs);
	    /* Got it? */
	    if(ret->ml_ME[i].me_Addr == NULL)
	    {
		/* No. Set return flags to "none of the 'ml_ME[i].me_Reqs' memory". */
		if (return_flags) *return_flags = entry->ml_ME[i].me_Reqs;

		/* Free everything allocated until now... */
		for( ; i-->0; )
		{
		    FreeMem(ret->ml_ME[i].me_Addr, ret->ml_ME[i].me_Length);
		}

		/* ...including the MemList */
		FreeMem(ret, mlsize);

		/* All done */
		return NULL;
	    }
	}
	else /* if length = 0 */
	{
	    ret->ml_ME[i].me_Addr = NULL;
	}

	/* Copy the Length field */
	ret->ml_ME[i].me_Length = entry->ml_ME[i].me_Length;

        D(bug("[NewAllocEntry] Allocated size %d at 0x%p\n",
              ret->ml_ME[i].me_Length, ret->ml_ME[i].me_Addr
        ));
    }
    
    /* Everything filled. Return OK. */
    if (return_flags) *return_flags = 0;
    
    return ret;
    
    AROS_LIBFUNC_EXIT
} /* AllocEntry */

