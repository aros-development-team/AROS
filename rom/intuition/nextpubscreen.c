/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Intuition function NextPubScreen()
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH2(UBYTE *, NextPubScreen,

/*  SYNOPSIS */
	AROS_LHA(struct Screen *, screen, A0),
	AROS_LHA(UBYTE         *, namebuf, A1),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 89, Intuition)

/*  FUNCTION
	Returns the name of the name of the next pub screen in system rotation.
	This allows visitor windows to jump among public screens in a cycle.

    INPUTS
	screen - The screen your window is currently open on.
	namebuf - Pointer to a buffer of MAXPUBSCREENNAME+1 characters.
		This function will fill in the name of the next public screen.
	mousepoint - Pointer to an array of two WORDs or a structure of type Point

    RESULT
	NULL if there are no public screens, otherwise the pointer to your
	namebuf.

    NOTES
	Note that the public screen may be closed or not remain public before
	having called LockPubScreen(), so consider the case that
	LockPubScreen() may fail.

    EXAMPLE

    BUGS

    SEE ALSO
	OpenScreen(), LockPubScreen()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

#warning TODO: Write intuition/NextPubScreen()
    aros_print_not_implemented ("NextPubScreen");

    return NULL;

    AROS_LIBFUNC_EXIT
} /* NextPubScreen */
