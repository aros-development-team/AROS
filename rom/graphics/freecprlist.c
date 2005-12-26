/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function FreeCprList()
    Lang: english
*/
#include <graphics/copper.h>
#include <proto/exec.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH1(void, FreeCprList,

/*  SYNOPSIS */
	AROS_LHA(struct cprlist *, cprList, A0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 94, Graphics)

/*  FUNCTION
	Deallocate all memory associated with this cprlist structure

    INPUTS
	cprlist - pointer to a cprlist structure

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

  struct cprlist * NextCprList = cprList;
  while (NULL != NextCprList)
  {
    cprList = NextCprList;
    NextCprList = NextCprList->Next;
    FreeMem(cprList->start, cprList->MaxCount << 2  );
    FreeMem(cprList, sizeof(struct cprlist));
  }

  AROS_LIBFUNC_EXIT
} /* FreeCprList */
