/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
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
	struct Library *, IconBase, 12, Icon)

/*  FUNCTION
	Adds supplied memory chunk to the supplied freelist.
	All memory added in to the freelist can later be deallocated through
	one single FreeFreeList() call.

    INPUTS
	freelist - pointer to freelist struct previously allocated by
		the programmer.
	mem - memory to add to the freelist.
	size - size of memory chunk to add to the frelist.

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

    /* Is this memlist full ? */
    if (memlist->ml_NumEntries == FREELIST_MEMLISTENTRIES
	|| freelist->fl_NumFree == 0
    )
    {
	/* No more room, we must allocate a new entry */
	if (!(memlist = AllocMem (sizeof(struct IconInternalMemList),
		    MEMF_ANY)
	) )
	    return FALSE;

	memlist->ml_NumEntries = 0;

	AddTail ((struct List*)&freelist->fl_MemList, (struct Node*)memlist);
    }

    /* Insert the the supplied parameters */
    mementry = &memlist->ml_ME[ memlist->ml_NumEntries ];

    mementry->me_Un.meu_Addr = mem;
    mementry->me_Length      = size;

    memlist->ml_NumEntries ++;

    freelist->fl_NumFree ++;

    return TRUE;
    AROS_LIBFUNC_EXIT
} /* AddFreeList */
