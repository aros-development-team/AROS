/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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

	AROS_LH3(BOOL, NewAllocEntry,

/*  SYNOPSIS */
	AROS_LHA(struct MemList *, entry, A0),
	AROS_LHA(struct MemList **, return_entry, A1),
	AROS_LHA(ULONG *, return_flags, D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 151, Exec)

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
    	TRUE if the allocation was successful. In this case *return_entry will
	be set to the address of the allocated MemList. *return_flags will be set
	to 0.
	
	FALSE if the allocation failed. In this case *return_entry will be set
	to NULL and *return_flags will be set to contain the type of memory that
	couldn't be allocated.
	
    NOTES

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

#if DEBUG > 0
    for(i = 0; i < entry->ml_NumEntries; i++)
    {
	kprintf("\treq $%lx\tsize $%lx\n", entry->ml_ME[i].me_Reqs, entry->ml_ME[i].me_Length);
    }
#endif

    /* Calculate size of a MemList with ml_NumEntries MemEntries. */
    mlsize = sizeof(struct MemList) - sizeof(struct MemEntry) +
	     sizeof(struct MemEntry) * entry->ml_NumEntries;

    /* Get the MemList structure */
    ret = (struct MemList *)AllocMem(mlsize, MEMF_PUBLIC);

    /* The allocation failed? Return "no public memory" */
    if(ret == NULL)
    {
    	*return_entry = NULL;
	if (return_flags) *return_flags = MEMF_PUBLIC;
	return FALSE;
    }
    
    /* Init new struct */
    ret->ml_NumEntries	 = entry->ml_NumEntries;
    ret->ml_Node.ln_Type = 0;
    ret->ml_Node.ln_Pri  = 0;
    ret->ml_Node.ln_Name = NULL;

    /* Fill all entries */
    for(i = 0; i < entry->ml_NumEntries; i++)
    {
#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
	/*
	    Somewhat of a compatibility kludge: some programs rely that
	    AllocEntry() doesn't fail if the length field is 0.

	    E.g. CrossDos' PCx wants to allocate 7 memory regions, but the
	    last two fields are empty.

	    Should this be turned into a feature and be noted in the autodoc?
	    The original behaves this way, but it is, AFAIK, not documented to do
	    so. Comments?
	*/
	if(entry->ml_ME[i].me_Length)
	{
#endif
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
		*return_entry = NULL;
		
		return FALSE;
	    }
#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
	}
	else /* if length = 0 */
	{
	    ret->ml_ME[i].me_Addr = NULL;
	}
#endif
	/* Copy the Length field */
	ret->ml_ME[i].me_Length = entry->ml_ME[i].me_Length;
    }
    
    /* Everything filled. Return OK. */
    *return_entry = ret;
    if (return_flags) *return_flags = 0;
    
    return TRUE;
    
    AROS_LIBFUNC_EXIT
} /* AllocEntry */

