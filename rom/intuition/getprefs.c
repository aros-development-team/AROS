/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Intuition function GetPrefs()
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH2(struct Preferences *, GetPrefs,

/*  SYNOPSIS */
	AROS_LHA(struct Preferences * , prefbuffer, A0),
	AROS_LHA(WORD                 , size, D0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 22, Intuition)

/*  FUNCTION
	Gets a copy of the current Preferences structure.

    INPUTS
	prefbuffer - The buffer which contains your settings for the
		preferences.
	size - The number of bytes of the buffer you want to be copied.

    RESULT
	Returns your parameter buffer.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	GetDefPrefs(), SetPrefs()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

#warning TODO: Write intuition/GetPrefs()
    aros_print_not_implemented ("GetPrefs");

    return NULL;

    AROS_LIBFUNC_EXIT
} /* GetPrefs */
