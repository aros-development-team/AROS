/*
	(C) 1995-99 AROS - The Amiga Research OS
	$Id$
 
	Desc: Intuition function MakeScreen()
	Lang: english
*/
#include <proto/graphics.h>
#include "intuition_intern.h"

/*****************************************************************************
 
	NAME */
#include <proto/intuition.h>

AROS_LH1(LONG, MakeScreen,

		 /*  SYNOPSIS */
		 AROS_LHA(struct Screen *, screen, A0),

		 /*  LOCATION */
		 struct IntuitionBase *, IntuitionBase, 63, Intuition)

/*  FUNCTION
 
	INPUTS
	Pointer to your custom screen.
 
	RESULT
	Zero for success, non-zero for failure.
 
	NOTES
 
	EXAMPLE
 
	BUGS
 
	SEE ALSO
	RemakeDisplay(), RethinkDisplay(), graphics.library/MakeVPort(),
 
	INTERNALS
 
	HISTORY
 
*****************************************************************************/
{
	AROS_LIBFUNC_INIT
	AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

	LONG failure = TRUE;
	ULONG ilock = LockIBase(0);

	if (screen)
	{
		if ((screen->ViewPort.Modes ^ IntuitionBase->ViewLord.Modes) & LACE)
		{
			failure = RemakeDisplay();
		}
		else
		{
			failure = MakeVPort(&IntuitionBase->ViewLord, &screen->ViewPort);
		}
	}

	UnlockIBase(ilock);

	return failure;

	AROS_LIBFUNC_EXIT
} /* MakeScreen */
