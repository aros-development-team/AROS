/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1996/10/21 17:06:49  aros
    A couple of new functions


    Desc:
    Lang: english
*/
#include "intuition_intern.h"
#include <string.h>
#include <clib/exec_protos.h>

/*****************************************************************************

    NAME */
	#include <intuition/screens.h>
	#include <clib/intuition_protos.h>

	__AROS_LH4(LONG, GetScreenData,

/*  SYNOPSIS */
	__AROS_LHA(APTR           , buffer, A0),
	__AROS_LHA(unsigned long  , size, D0),
	__AROS_LHA(unsigned long  , type, D1),
	__AROS_LHA(struct Screen *, screen, A1),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 71, Intuition)

/*  FUNCTION
	Copy part or all infos about a screen into a private buffer.

	To copy the Workbench, one would call

	    GetScreenData (buffer, sizeof(struct Screen), WBENCHSCREEN, NULL)

	If the screen is not open, this call will open it. You can use
	this function for these purposes:

	1) Get information about the workbench in order to open a window
	   on it (eg. size).
	2) Clone a screen.

    INPUTS
	buffer - The data gets copied here
	size - The size of the buffer in bytes
	type - The type of the screen as in OpenWindow().
	screen - Ignored unless type is CUSTOMSCREEN.

    RESULT
	TRUE if successful, FALSE if the screen could not be opened.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    if (type == WBENCHSCREEN)
	screen = GetPrivIBase(IntuitionBase)->WorkBench;
    else if (type != CUSTOMSCREEN) /* TODO */
	screen = NULL;

    if (screen)
	CopyMem (screen, buffer, size);

    return (screen != NULL);
    __AROS_FUNC_EXIT
} /* GetScreenData */
