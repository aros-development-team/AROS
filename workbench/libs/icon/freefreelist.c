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

	AROS_LH1(void, FreeFreeList,

/*  SYNOPSIS */
	AROS_LHA(struct FreeList *, freelist, A0),

/*  LOCATION */
	struct IconBase *, IconBase, 9, Icon)

/*  FUNCTION
	Frees all memory chunks in the freelist (previously inserted into
	it via AddFreeList()).

    INPUTS
	freelist  - pointer to FreeList struct. It is safe to use NULL.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	AddFreeList()

    INTERNALS

    HISTORY
	2006-04-10 Test for NULL pointer added
	
*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if ( ! freelist) return;
    
    while (!IsListEmpty(&freelist->fl_MemList))
    {
	struct MemList *node = (struct MemList*)freelist->fl_MemList.lh_Head;
	Remove ((struct Node*)node);
	FreeEntry (node);
    }

    AROS_LIBFUNC_EXIT
} /* FreeFreeList */
