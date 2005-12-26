/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function CWait()
    Lang: english
*/
#include <exec/types.h>
#include <graphics/copper.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH3(void, CWait,

/*  SYNOPSIS */
	AROS_LHA(struct UCopList *, ucl, A1),
	AROS_LHA(WORD             , v,   D0),
	AROS_LHA(WORD             , h,   D1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 63, Graphics)

/*  FUNCTION
	Add a copper wait instruction to the given user copper list.
	The copper is told to wait for a vertical beam position v and
	a horizontal beam position h.
	If you are using CWAIT() a call to CWait() and CBump() will
	be made.

    INPUTS
	ucl - pointer to a UCopList structure
	v   - vertical beam position (relative to top of viewport)
	h   - horizontal beam position

    RESULT

    NOTES

    EXAMPLE

    BUGS
	It's illegal to wait for horizontal values greater than 222 decimal!

    SEE ALSO
	CINIT CMOVE CEND graphics/copper.h

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

  struct CopIns * copIns = ucl->CopList->CopPtr;
  copIns->OpCode = COPPER_WAIT;
  copIns->u3.u4.u1.VWaitPos = v;
  copIns->u3.u4.u2.HWaitPos = h;

  AROS_LIBFUNC_EXIT
} /* CMove */
