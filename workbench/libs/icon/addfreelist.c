/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "icon_intern.h"

/*****************************************************************************

    NAME */
#include <proto/icon.h>

	AROS_LH3(BOOL, AddFreeList,

/*  SYNOPSIS */
	AROS_LHA(struct FreeList *, freelist, A0),
	AROS_LHA(APTR             , mem, A1),
	AROS_LHA(unsigned long    , size, A2),

/*  LOCATION */
	struct IconBase *, IconBase, 12, Icon)

/*  FUNCTION
	Adds supplied memory chunk to the supplied freelist. The memory chunk
	must have been allocated by AllocMem(). All memory added into the
	freelist can later be deallocated through a single FreeFreeList() call.

    INPUTS
	freelist - pointer to freelist struct previously allocated by
	    the programmer.
	mem - memory to add to the freelist.
	size - size of memory chunk to add to the freelist.

    RESULT
	FALSE on failure, else TRUE.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	FreeFreeList()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct MemList  *memlist;
    struct MemEntry *mementry;
    /* We get hold of the last memlist node inside the list */
    memlist = (struct MemList*)freelist->fl_MemList.lh_TailPred;

    /* Is this memlist full? */
    if (freelist->fl_NumFree == 0)
    {
	/* No more room, we must allocate a new entry */
	if (!(memlist = AllocMem (sizeof(struct IconInternalMemList),
		    MEMF_CLEAR)
	) )
	    return FALSE;

	freelist->fl_NumFree = FREELIST_MEMLISTENTRIES;
	memlist->ml_NumEntries = FREELIST_MEMLISTENTRIES;

	AddTail ((struct List*)&freelist->fl_MemList, (struct Node*)memlist);
    }

    /* Insert the supplied parameters */
    mementry = &memlist->ml_ME[freelist->fl_NumFree - 1];

    mementry->me_Un.meu_Addr = mem;
    mementry->me_Length      = size;

    freelist->fl_NumFree--;

    return TRUE;
    AROS_LIBFUNC_EXIT
} /* AddFreeList */
