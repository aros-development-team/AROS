/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

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

    Forbid();

    if (!window->IDCMPFlags && flags && !window->UserPort)
    {
	window->UserPort = CreateMsgPort ();

	if (!window->UserPort)
	{
	    Permit();
	    return FALSE;
	}
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

    Permit();
    
    return TRUE;
    
    AROS_LIBFUNC_EXIT
} /* ModifyIDCMP */
