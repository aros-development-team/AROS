/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Close a screen opened via OpenScreen and the like
    Lang: english
*/
#include "intuition_intern.h"
#include <proto/exec.h>
#include <proto/graphics.h>

#ifndef DEBUG_CloseScreen
#   define DEBUG_CloseScreen 0
#endif
#if DEBUG_CloseScreen
#   undef DEBUG
#   define DEBUG 1
#endif
#	include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <intuition/screens.h>
#include <proto/intuition.h>

	AROS_LH1(BOOL, CloseScreen,

/*  SYNOPSIS */
	AROS_LHA(struct Screen *, screen, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 11, Intuition)

/*  FUNCTION

    INPUTS

    RESULT

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
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)
    struct Screen * parent;

    D(bug("CloseScreen (%p)\n", screen));

    /* Trick: Since NextScreen is the first field of the structure,
	we can use the pointer in the IntuitionBase as a screen with
	the structure-size of one pointer */
    parent = (struct Screen *)&(IntuitionBase->FirstScreen);

    /* For all screens... */
    while (parent->NextScreen)
    {
	/* If the screen to close is the next screen... */
	if (parent->NextScreen == screen)
	{
	    /* Unlink it */
	    parent->NextScreen = screen->NextScreen;

	    /* Check ActiveScreen */
	    if (IntuitionBase->ActiveScreen == screen)
	    {
		if (screen->NextScreen)
		    IntuitionBase->ActiveScreen = screen->NextScreen;
		else if (IntuitionBase->FirstScreen)
		    IntuitionBase->ActiveScreen = parent;
		else
		    IntuitionBase->ActiveScreen = NULL;
	    }

	    /* Free the RastPort's contents */
	    DeinitRastPort (&screen->RastPort);

	    /* Free the memory */
	    FreeMem (screen, sizeof (struct Screen));

	    ReturnBool("CloseScreen",TRUE);
	}
    }

    ReturnBool("CloseScreen",FALSE);
    AROS_LIBFUNC_EXIT
} /* CloseScreen */
