/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.8  2000/01/30 23:45:34  bernie
    Increment visitor count even for LockPubScreen(NULL); Don't expect that IntuitionBase->DefaultScreen is always non-NULL (the AutoDoc says LockPubScreen() should open the Workbench in that case, but we still don't); Add pointer checking assertions.

    Revision 1.7  1999/10/12 17:45:44  SDuvan
    Added docs, minor updates

    Revision 1.6  1999/09/12 01:48:58  bernie
    more public screens support

    Revision 1.5  1998/10/20 16:45:59  hkiel
    Amiga Research OS

    Revision 1.4  1997/01/27 00:36:40  ldp
    Polish

    Revision 1.3  1996/12/10 14:00:05  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.2  1996/10/24 15:51:22  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.1  1996/09/21 15:48:47  digulla
    New functions to handle Public Screens


    Desc:
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH1(struct Screen *, LockPubScreen,

/*  SYNOPSIS */
	AROS_LHA(UBYTE *, name, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 85, Intuition)

/*  FUNCTION

    Prevents a public screen from closing.
    This is useful if you want to put up a visitor window on a public screen
    and need to check some of the public screen's field first -- not locking
    the screen may lead to the public screen not existing when your visitor
    window is ready.

    INPUTS

    Name   --  Name of the public screen or NULL for the default public
               screen. The name "Workbench" refers to the Workbench screen.

    RESULT

    A pointer to the screen or NULL if something went wrong. Failure can
    happen for instance when the public screen is in private state or doesn't
    exist.

    NOTES

    You don't need to hold the lock when your visitor window is opened as
    the pubscreen cannot be closed as long as there are visitor windows
    on it.

    EXAMPLE

    To open a visitor window which needs information from the screen structure
    of the public screen to open on, do this:

    if((pubscreen = LockPubScreen("PubScreentoOpenon")) != NULL)
    {
        ...check pubscreen's internal data...
	OpenWindow(VisitorWindow, pubscreen);
	UnlockPubScreen(NULL, pubscreen);
	...use your visitor window...
	CloseWindow(VisitorWindow);
    }

    BUGS

    SEE ALSO

    OpenWindow(), UnlockPubScreen(), GetScreenData()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    struct Screen *scr = NULL;

    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)


    if(!name)
    {
	LockPubScreenList();
	if ((scr = GetPrivIBase(IntuitionBase)->DefaultPubScreen))
	{
	    ASSERT_VALID_PTR(scr);
	    GetPrivScreen(scr)->pubScrNode->psn_VisitorCount++;
	}
	UnlockPubScreenList();
    }
    else
    {
	struct PubScreenNode *psn;
	ASSERT_VALID_PTR(name);

	/* Browse the public screen list */
	if ((psn = (struct PubScreenNode *)FindName(LockPubScreenList(), name)))
	{
	    ASSERT_VALID_PTR(psn);

	    /* Don't lock screens in private state */
	    if((psn != NULL) && !(psn->psn_Flags & PSNF_PRIVATE))
	    {
		/* Increment screen lock count */
		psn->psn_VisitorCount++;
		scr = psn->psn_Screen;
		ASSERT_VALID_PTR(scr);
	    }
	}
	
	UnlockPubScreenList();
    }
    
    return scr;

    AROS_LIBFUNC_EXIT
} /* LociPubScreen */
