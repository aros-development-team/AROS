/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.7  1999/09/12 01:48:58  bernie
    more public screens support

    Revision 1.6  1999/01/16 23:19:48  hkiel
    Added aros_print_not_implemented()

    Revision 1.5  1998/10/20 16:46:07  hkiel
    Amiga Research OS

    Revision 1.4  1997/01/27 00:36:44  ldp
    Polish

    Revision 1.3  1996/12/10 14:00:10  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.2  1996/10/24 15:51:25  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.1  1996/09/21 15:48:47  digulla
    New functions to handle Public Screens


    Desc: Intuition function UnlockPubScreen()
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH2(void, UnlockPubScreen,

/*  SYNOPSIS */
	AROS_LHA(UBYTE         *, name, A0),
	AROS_LHA(struct Screen *, screen, A1),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 86, Intuition)

/*  FUNCTION
	Release a lock to a screen locked by LockPubScreen().
	Identify screen by the pointer returned from LockPubScreen()
	and pass NULL name in normal cases.
	Sometimes it might be useful to specify the name string. In
	this case the screen pointer will be ignored.

    INPUTS
	name - Name of the public screen to unlock
	screen - Pointer to the screen to unlock

    RESULT

    NOTES
	The screen parameter will be ignored if name is non-NULL

    EXAMPLE

    BUGS

    SEE ALSO
	LockPubScreen()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    struct List *publist;

    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    publist = LockPubScreenList();

    if (name != NULL)
    {
	struct PubScreenNode *psn;

	/* Get screen by its name */
        if ((psn = (struct PubScreenNode *)FindName(publist, name)))
		screen = psn->psn_Screen;
	else
		screen = NULL;
    }

    if (screen != NULL)
    {
	//ASSERT(GetPrivScreen(screen)->pubScrNode != NULL);
	//ASSERT(GetPrivScreen(screen)->pubScrNode->psn_VisitorCount > 0);

	/* Unlock screen */
	GetPrivScreen(screen)->pubScrNode->psn_VisitorCount--;
    }

    UnlockPubScreenList();
    AROS_LIBFUNC_EXIT
} /* UnlockPubScreen */
