/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function SetCollision()
    Lang: english
*/
#include <graphics/gels.h>
#include <graphics/rastport.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH3(void, SetCollision,

/*  SYNOPSIS */
	AROS_LHA(ULONG            , num    , D0),
	AROS_LHA(VOID_FUNC        , routine, A0),
	AROS_LHA(struct GelsInfo *, GInfo  , A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 24, Graphics)

/*  FUNCTION
	Call this function to set a specified entry (num) in the
	user's collision vector table with the address of the
	routine to be called by DoCollision().

    INPUTS
	num	= number of collsion vector
	routine = pointer to user's collision routine
	GInfo	= pointer to a GelsInfo structure

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	InitGels() graphics/rastport.h graphics/gels.h

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

  GInfo -> collHandler -> collPtrs[num] = (APTR)routine;


  AROS_LIBFUNC_EXIT
} /* SetCollision */
