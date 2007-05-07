/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$	$Log

    Desc: Graphics function InitRastPort()
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/rastport.h>
#include <proto/exec.h>
#include <proto/graphics.h>

/*****************************************************************************

    NAME */

	AROS_LH3(void, InitTmpRas,

/*  SYNOPSIS */
	AROS_LHA(struct TmpRas  *, tmpras, A0),
	AROS_LHA(void *          , buffer, A1),
	AROS_LHA(ULONG           , size  , D0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 78, Graphics)

/*  FUNCTION
	Initializes a TmpRas structure. The user has to connect the
	TmpRas structure to the rastport.
	Some routines need extra memory in order to be able to operate
	properly.

    INPUTS
	tmpras - pointer to a TmpRas structure to be initialized
	buffer - pointer to a piece of chip memory.
	size   - size in bytes of buffer.

    RESULT
        Properly initialized TmpRas structure to link to RastPort structure
        for use with functions like Flood(), Text() and AreaEnd().

    NOTES

    EXAMPLE

    BUGS
        The function itself is a bug.
        Why does this function exist at all? The necessary memory should 
        be allocated in InitRastPort() or the functions that need it.

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT

  tmpras->RasPtr = buffer;
  tmpras->Size   = size;

  AROS_LIBFUNC_EXIT
} /* InitTmpRas */
