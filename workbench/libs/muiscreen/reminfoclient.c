/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <libraries/muiscreen.h>
#include <exec/nodes.h>
#include <proto/exec.h>
#define DEBUG 1
#include <aros/debug.h>

#include "muiscreen_intern.h"

/*****************************************************************************

    NAME */
#include <proto/muiscreen.h>

        AROS_LH1(void, MUIS_RemInfoClient,

/*  SYNOPSIS */
	AROS_LHA(struct MUIS_InfoClient *, sic,  A0),

/*  LOCATION */
	struct Library *, MUIScreenBase_intern, 0x54, MUIScreen)

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

    D(bug("MUIS_RemInfoClient(%p)\n", sic));

    Remove((struct Node*) sic);

    AROS_LIBFUNC_EXIT
}
