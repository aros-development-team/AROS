/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/debug.h>
#include <cybergraphx/cybergraphics.h>
#include <exec/types.h>
#include <proto/cybergraphics.h>
#include <proto/utility.h>

#include "cybergraphics_intern.h"

/*****************************************************************************

    NAME */
#include <clib/cybergraphics_protos.h>

	AROS_LH8(VOID, ProcessPixelArray,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(ULONG, destX, D0),
	AROS_LHA(ULONG, destY, D1),
	AROS_LHA(ULONG, sizeX, D2),
	AROS_LHA(ULONG, sizeY, D3),
	AROS_LHA(ULONG, operation, D4),
	AROS_LHA(LONG, value, D5),
	AROS_LHA(struct TagItem *, taglist, A2),

/*  LOCATION */
	struct Library *, CyberGfxBase, 38, Cybergraphics)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES
	Not implemented.
	This function exists to get Scalos compiled. Because Scalos
	has its own fallback code for the case that lib_Version < 50
	it's not so urgent to implement it.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* TODO: Implement me */

    bug("ProcessPixelArray not implemented\n");

#if 0
    struct TagItem *tstate;
    struct TagItem *tag;

    for (tstate = tags; (tag = NextTagItem(&tstate)); ) {
	switch (tag->ti_Tag) {
	    case aaa:
	    	minwidth = (ULONG)tag->ti_Data;
		break;
		

	    default:
	    	D(bug("!!! UNKNOWN TAG PASSED TO ProcessPixelArray\n"));
		break;
	} 	
    }

#endif

    AROS_LIBFUNC_EXIT
}
