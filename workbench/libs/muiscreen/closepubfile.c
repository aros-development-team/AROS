/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <libraries/muiscreen.h>
#include <proto/dos.h>
#include <proto/iffparse.h>
#define DEBUG 1
#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <proto/muiscreen.h>

        AROS_LH1(void, MUIS_ClosePubFile,

/*  SYNOPSIS */
	AROS_LHA(APTR, pf,  A0),

/*  LOCATION */
	struct Library *, MUIScreenBase, 0x30, MUIScreen)

/*  FUNCTION

    INPUTS

    RESULT
    
    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    D(bug("MUIS_ClosePubFile(%p)\n", pf));

    struct IFFHandle *iff = (struct IFFHandle *) pf;

    if (iff)
    {
	if(iff->iff_Flags & IFFF_WRITE)
	    PopChunk(iff);

	CloseIFF(iff);
        Close((BPTR) iff->iff_Stream);
	FreeIFF(iff);
    }

    AROS_LIBFUNC_EXIT
}
