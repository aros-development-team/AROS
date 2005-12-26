/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function CMove()
    Lang: english
*/
#include <exec/types.h>
#include <graphics/copper.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH3(void, CMove,

/*  SYNOPSIS */
	AROS_LHA(struct UCopList *, ucl,   A1),
	AROS_LHA(void *           , reg,   D0),
	AROS_LHA(WORD             , value, D1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 62, Graphics)

/*  FUNCTION
	Add a copper move instruction to the given user copper list.
	The copper is told to move a value to register reg.
	If you are using CMOVE() a call to CMove() and CBump() will
	be made.

    INPUTS
	ucl   - pointer to a UCopList structure
	reg   - hardware register
	value - 16 bit value to be moved to the hardware register

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	CINIT CWAIT CEND graphics/copper.h

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

  struct CopIns * copIns = ucl->CopList->CopPtr;
  copIns->OpCode = COPPER_MOVE;
  copIns->u3.u4.u1.DestAddr = (WORD)(ULONG)reg;
  copIns->u3.u4.u2.DestData = value;

  AROS_LIBFUNC_EXIT
} /* CMove */
