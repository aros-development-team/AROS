/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intuition function SetPrefs()
    Lang: english
*/
#include "intuition_intern.h"
#include <intuition/preferences.h>
#include <string.h>

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

	if (size > 0 && NULL != prefbuffer)
	{
		LockIBase(0);
		memcpy(GetPrivIBase(IntuitionBase)->ActivePreferences,
		       prefbuffer,
		       size <= sizeof(struct Preferences) ? size : sizeof(struct Preferences));

		UnlockIBase(0);
		/*
		** If inform == TRUE then notify all windows that want to know about 	
		** an update on the preferences.
		*/

		AllocAndSendIntuiActionMsg(AMCODE_NEWPREFS, NULL, IntuitionBase);

	}

#warning Is there any further immediate action to be taken when the prefences are updated?

	return (struct Preferences *) prefbuffer;

	AROS_LIBFUNC_EXIT
  
} /* SetPrefs */
