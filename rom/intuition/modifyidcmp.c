/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.9  1999/03/19 10:38:08  nlorentz
    Bugfix (fixes memleak in Open/CloseWindow(): Only create new window->UserPort when window->UserPort == NULL

    Revision 1.8  1998/10/20 16:45:59  hkiel
    Amiga Research OS

    Revision 1.7  1997/01/27 00:36:40  ldp
    Polish

    Revision 1.6  1996/12/10 14:00:05  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.5  1996/11/08 11:28:03  aros
    All OS function use now Amiga types

    Moved intuition-driver protos to intuition_intern.h

    Revision 1.4  1996/10/24 15:51:22  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.3  1996/08/29 13:33:31  digulla
    Moved common code from driver to Intuition
    More docs

    Revision 1.2  1996/08/23 17:25:30  digulla
    Added include intuition/intuition.h to user-docs

    Revision 1.1  1996/08/13 15:37:27  digulla
    First function for intuition.library


    Desc:
    Lang: english
*/
#include "intuition_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <intuition/intuition.h>
#include <proto/intuition.h>

	AROS_LH2(BOOL, ModifyIDCMP,

/*  SYNOPSIS */
	AROS_LHA(struct Window *, window, A0),
	AROS_LHA(ULONG          , flags, D0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 25, Intuition)

/*  FUNCTION
	This routine modifies the state of your window's IDCMP (Intuition
	Direct Communication Message Port).

	Depending on the current state in the IDCMPFlags of the window and
	the specified flags these actions are possible:

	IDCMP	flags	Action
	  0	  0	Nothing happens
	  0	 !=0	The flags are copied in the IDCMPFlags of the window
			and a MessagePort is created and stored in the
			UserPort of the window.
	 !=0	  0	The IDCMPFlags are cleared and the MessagePort in the
			UserPort is deleted.
	 !=0	 !=0	The flags are copied to the IDCMPFlags of the
			window.

    INPUTS
	window - The window to change the IDCMPFlags in.
	flags - New flags for the IDCMPFlags of the window. See
		intuition/intuition.h for the available flags.

    RESULT
	TRUE if the change could be made and FALSE otherwise.

    NOTES
	You can set up the Window->UserPort to any port of your own
	before you call ModifyIDCMP().  If IDCMPFlags is non-null but
	your UserPort is already initialized, Intuition will assume that
	it's a valid port with task and signal data preset and Intuition
	won't disturb your set-up at all, Intuition will just allocate
	the Intuition message port half of it.	The converse is true
	as well:  if UserPort is NULL when you call here with
	IDCMPFlags == NULL, Intuition will deallocate only the Intuition
	side of the port.

	This allows you to use a port that you already have allocated:

	- OpenWindow() with IDCMPFlags equal to NULL (open no ports)
	- set the UserPort variable of your window to any valid port of your
	  own choosing
	- call ModifyIDCMP with IDCMPFlags set to what you want
	- then, to clean up later, set UserPort equal to NULL before calling
	  CloseWindow() (leave IDCMPFlags alone)  BUT FIRST: you must make
	  sure that no messages sent your window are queued at the port,
	  since they will be returned to the memory free pool.

	For an example of how to close a window with a shared IDCMP,
	see the description for CloseWindow().

    EXAMPLE

    BUGS

    SEE ALSO
	OpenWindow(), CloseWindow()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    if (!window->IDCMPFlags && flags && !window->UserPort)
    {
	window->UserPort = CreateMsgPort ();

	if (!window->UserPort)
	    return FALSE;
    }

    window->IDCMPFlags = flags;

    if (!flags)
    {
	if (window->UserPort)
	{
	    DeleteMsgPort (window->UserPort);
	    window->UserPort = NULL;
	}
    }

    return TRUE;
    AROS_LIBFUNC_EXIT
} /* ModifyIDCMP */
