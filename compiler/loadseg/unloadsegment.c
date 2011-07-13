/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <exec/libraries.h>
#include <proto/debug.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include "loadseg_intern.h"

/*****************************************************************************

    NAME */
#include <loadseg/loadseg.h>

        BOOL UnLoadSegment(

/*  SYNOPSIS */
        BPTR      seglist ,
        VOID_FUNC freefunc,
        struct DosLibrary *DOSBase)

/*  LOCATION

        loadseg.lib

    FUNCTION
	Unloads a seglist loaded with LoadSegment().

    INPUTS
	seglist  - Seglist
	freefunc - Function to be called to free memory
	DOSBase  - Required for AOS HUNK overlays only, otherwise NULL

    RESULT
	DOSTRUE if everything wents O.K.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    BPTR next;
    SIPTR funcarray[] = { (SIPTR)NULL, (SIPTR)NULL, (SIPTR)freefunc };

    if (seglist)
    {
        APTR DebugBase;

        if ((DebugBase = OpenLibrary("debug.library", 0))) {
            UnregisterModule(seglist);
            CloseLibrary(DebugBase);
        }

#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
	if (DOSBase != NULL) {
    	    /* free overlay structures */
    	    ULONG *seg = BADDR(seglist);
    	    if (seg[2] == 0x0000abcd && seg[6] == (ULONG)DOSBase->dl_GV) {
    	    	Close((BPTR)seg[3]); /* file handle */
    	    	ilsFreeVec((void*)seg[4]); /* overlay table, APTR */
    	    	ilsFreeVec(BADDR(seg[5])); /* hunk table, BPTR */
    	    }
    	}
#endif

	while (seglist)
	{
	    next = *(BPTR *)BADDR(seglist);
	    ilsFreeVec(BADDR(seglist));
	    seglist = next;
	}

	return DOSTRUE;
    }
    else
	return DOSFALSE;

} /* UnLoadSegment */
