/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
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
	struct Library *, IconBase, 9, Icon)

/*  FUNCTION
	Frees all memory chunks in the freelist (previously inserted into
	it via AddFreeList()).

    INPUTS
	freelist  - pointer to FreeList struct.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	AddFreeList()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IconBase)
    struct MemList * node,
		   * nextnode;

    node = (struct MemList*)freelist->fl_MemList.lh_Head;

    while ( (nextnode = (struct MemList*)node->ml_Node.ln_Succ) )
    {
	FreeEntry (node);

	node = nextnode;
    }

    FreeMem (freelist, sizeof(struct FreeList));

    AROS_LIBFUNC_EXIT
} /* FreeFreeList */
