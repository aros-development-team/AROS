/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intuition function GetPrefs()
    Lang: english
*/
#include "intuition_intern.h"
#include <intuition/preferences.h>
#include <string.h>

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

	if (NULL != prefbuffer && 0 != size) {
		LockIBase(0);
		memcpy(prefbuffer,
		       GetPrivIBase(IntuitionBase)->ActivePreferences, 
		       size <= sizeof(struct Preferences) ? size : sizeof(struct Preferences));
		UnlockIBase(0);
	}
	return (struct Preferences *)prefbuffer;

	AROS_LIBFUNC_EXIT
} /* GetPrefs */
