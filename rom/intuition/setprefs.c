/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Intuition function SetPrefs()
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH3(struct Preferences *, SetPrefs,

/*  SYNOPSIS */
	AROS_LHA(struct Preferences * , prefbuffer, A0),
	AROS_LHA(LONG                 , size, D0),
	AROS_LHA(BOOL                 , inform, D1),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 54, Intuition)

/*  FUNCTION
	Sets the current Preferences structure.

    INPUTS
	prefbuffer - The buffer which contains your settings for the
		preferences.
	size - The number of bytes of the buffer you want to be copied.
	inform - If TRUE, all windows with IDCMP_NEWPREFS IDCMPFlags set
		get an IDCMP_NEWPREFS message.

    RESULT
	Returns your parameter buffer.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	GetDefPrefs(), GetPrefs()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

#warning TODO: Write intuition/SetPrefs()
    aros_print_not_implemented ("SetPrefs");

    return NULL;

    AROS_LIBFUNC_EXIT
} /* SetPrefs */
