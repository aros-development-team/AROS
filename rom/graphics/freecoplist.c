/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function FreeCopList()
    Lang: english
*/
#include <exec/types.h>
#include <graphics/copper.h>
#include <proto/exec.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH1(void, FreeCopList,

/*  SYNOPSIS */
	AROS_LHA(struct CopList *, coplist, A0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 91, Graphics)

/*  FUNCTION
	Deallocate all memory associated with this copper list.

    INPUTS
	coplist - pointer to a CopList structure

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	graphics/copper.h

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

  struct CopList * NextCopList = coplist;
  while (NULL != NextCopList)
  {
    coplist = NextCopList;
    /*  get the pointer to the next CopList BEFORE we return it to free
     *  memory
     */
    NextCopList = coplist -> Next;
    FreeMem(coplist->CopIns, sizeof(struct CopIns) * coplist->MaxCount);
    FreeMem(coplist, sizeof(struct CopList));
  }

  AROS_LIBFUNC_EXIT
} /* FreeCopList */
