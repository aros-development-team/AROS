/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH1(void, ActivateWindow,

/*  SYNOPSIS */
	AROS_LHA(struct Window *, window, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 75, Intuition)

/*  FUNCTION
	Activates the specified window. The window gets the focus
	and all further input it sent to that window. If the window
	requested it, it will get a IDCMP_ACTIVEWINDOW message.

    INPUTS
	window - The window to activate

    RESULT
	None.

    NOTES
	If the user has an autopointer tool (sunmouse), the call will
	succeed, but the tool will deactivate the window right after
	this function has activated it. It is no good idea to try to
	prevent this by waiting for IDCMP_INACTIVEWINDOW and activating
	the window again since that will produce an anoying flicker and
	it will slow down the computer a lot.

    EXAMPLE

    BUGS

    SEE ALSO
	ModiyIDCMP(), OpenWindow(), CloseWindow()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)
    
    AllocAndSendIntuiActionMsg(AMCODE_ACTIVATEWINDOW, window, IntuitionBase);

    AROS_LIBFUNC_EXIT
    
} /* ActivateWindow */

/* This is calles on the input.device's context */

VOID int_activatewindow(struct Window *window, struct IntuitionBase *IntuitionBase)
{

    ULONG lock;
    struct Window *oldactive;
    
    lock = LockIBase(0UL);
    
    oldactive = IntuitionBase->ActiveWindow;
    IntuitionBase->ActiveWindow = window;

    if (window)
    {
    	/* App task is allowed to modify window->Flags, for example
	   set/clear WFLG_RMBTRAP. It is noid said that every compiler
	   on every machine produces an atomic instruction, so some
	   kind of semaphore is needed */
	   
    	Forbid();
	window->Flags |= WFLG_WINDOWACTIVE;
	Permit();
    }
    
    UnlockIBase(lock);
    
    if (oldactive && oldactive != window)
    {
        Forbid();
	oldactive->Flags &= ~WFLG_WINDOWACTIVE;
	Permit();
	
	RefreshWindowFrame(oldactive);
    }    

    
    if (window) RefreshWindowFrame(window);
}

