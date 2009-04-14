/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <libraries/muiscreen.h>
#include <proto/exec.h>
#define DEBUG 1
#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <proto/muiscreen.h>

        AROS_LH1(BOOL, MUIS_FreePubScreenDesc,

/*  SYNOPSIS */
	AROS_LHA(struct MUI_PubScreenDesc *, psd,  A0),

/*  LOCATION */
	struct Library *, MUIScreenBase, 0x1e, MUIScreen)

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

    D(bug("MUIS_FreePubScreenDesc(%p)\n", psd));

    FreeMem(psd, sizeof(struct MUI_PubScreenDesc));

    return TRUE;
    
    AROS_LIBFUNC_EXIT
}
