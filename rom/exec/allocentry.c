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
#ifndef DEBUG_AllocEntry
#   define DEBUG_AllocEntry 0
#endif
#undef DEBUG
#if DEBUG_AllocEntry
#   define DEBUG 1
#endif
#include <aros/debug.h>
#undef kprintf

/*****************************************************************************

    NAME */

	AROS_LH1(struct MemList *, AllocEntry,

/*  SYNOPSIS */
	AROS_LHA(struct MemList *, entry, A0),

/*  LOCATION */
	struct ExecBase *, SysBase, 37, Exec)

/*  FUNCTION
	Allocate a number of memory blocks through a MemList structure.

    INPUTS
	entry - The MemList with one MemEntry for each block you want to get

    RESULT
	The allocation was successful if the most significant bit of the
	result is 0. The result then contains a pointer to a copy of
	the MemList structure with the me_Addr fields filled.
	If the most significant bit is set the result contains the type of
	memory that couldn't be allocated.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	FreeEntry()

    INTERNALS

    HISTORY
	18-10-95    created by m. fleischer

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct MemList *ret;
    ULONG mlsize,i;

    D(bug("AllocEntry $%lx num=%d\ttask=\"%s\"\n", entry, entry->ml_NumEntries, SysBase->ThisTask->tc_Node.ln_Name));
#if DEBUG > 0
    for(i=0; i<entry->ml_NumEntries; i++)
	kprintf("\treq $%lx\tsize $%lx\n", entry->ml_ME[i].me_Reqs, entry->ml_ME[i].me_Length);
#endif

    /* Calculate size of a MemList with ml_NumEntries MemEntries. */
    mlsize=sizeof(struct MemList)-sizeof(struct MemEntry)+
	   sizeof(struct MemEntry)*entry->ml_NumEntries;

    /* Get the MemList structure */
    ret=(struct MemList *)AllocMem(mlsize,MEMF_PUBLIC);

    /* Check nasty case where the returncode is misleading :-( */
    if(ret && !AROS_CHECK_ALLOCENTRY(ret))
    {
	FreeMem(ret,mlsize);
	ret=NULL;
    }

    /* The allocation failed? Return "no public memory" */
    if(ret==NULL)
	return AROS_ALLOCENTRY_FAILED(MEMF_PUBLIC);

    /* Init new struct */
    ret->ml_NumEntries=entry->ml_NumEntries;
    ret->ml_Node.ln_Type=0;
    ret->ml_Node.ln_Pri =0;
    ret->ml_Node.ln_Name=NULL;

    /* Fill all entries */
    for(i=0;i<entry->ml_NumEntries;i++)
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
	    ret->ml_ME[i].me_Addr=AllocMem(entry->ml_ME[i].me_Length,
				       entry->ml_ME[i].me_Reqs);
	    /* Got it? */
	    if(ret->ml_ME[i].me_Addr==NULL)
	    {
		/* No. Set returncode to "none of the 'ml_ME[i].me_Reqs' memory". */
		entry=AROS_ALLOCENTRY_FAILED(entry->ml_ME[i].me_Reqs);

		/* Free everything allocated until now... */
		for(;i-->0;)
		    FreeMem(ret->ml_ME[i].me_Addr,ret->ml_ME[i].me_Length);

		/* ...including the MemList */
		FreeMem(ret,mlsize);

		/* All done */
		return entry;
	    }
#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
	}
	else /* if length = 0 */
	{
	    ret->ml_ME[i].me_Addr = NULL;
	}
#endif
	/* Copy the Length field */
	ret->ml_ME[i].me_Length=entry->ml_ME[i].me_Length;
    }
    /* Everything filled. Return OK. */
    return ret;
    AROS_LIBFUNC_EXIT
} /* AllocEntry */

