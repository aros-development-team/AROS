/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.13  2000/01/19 19:04:28  stegerg
    don't call intui_activatewindow

    Revision 1.12  1999/10/13 21:08:13  stegerg
    action message goes to deferedactionport now

    Revision 1.11  1999/10/06 19:55:27  stegerg
    int_activatewindow may have window = NULL.
    protect window->Flags modification with Forbid(), Permit()

    Revision 1.10  1999/03/26 10:37:44  nlorentz
    Set WFLG_WINDOWACTIVE flag

    Revision 1.9  1999/03/25 04:26:23  bergers
    Update for deffered treatment of windows.

    Revision 1.8  1999/03/24 20:05:26  nlorentz
    Handle window activation on input.devices context

    Revision 1.7  1998/10/20 16:45:50  hkiel
    Amiga Research OS

    Revision 1.6  1997/01/27 00:36:35  ldp
    Polish

    Revision 1.5  1996/12/10 13:59:59  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.4  1996/11/08 11:28:00  aros
    All OS function use now Amiga types

    Moved intuition-driver protos to intuition_intern.h

    Revision 1.3  1996/10/24 15:51:17  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.2  1996/08/29 13:33:30  digulla
    Moved common code from driver to Intuition
    More docs

    Revision 1.1  1996/08/23 17:28:17  digulla
    Several new functions; some still empty.


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
    
    struct DeferedActionMessage *msg;
    
    msg = AllocMem( sizeof (struct DeferedActionMessage), MEMF_PUBLIC);
    if (msg)
    {
	msg->Code  	= AMCODE_ACTIVATEWINDOW;
	msg->Window	= window;

        PutMsg(GetPrivIBase(IntuitionBase)->IntuiDeferedActionPort, (struct Message *)msg);
	
    }
    

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
    
    if (oldactive)
    {
        Forbid();
	oldactive->Flags &= ~WFLG_WINDOWACTIVE;
	Permit();
	
	RefreshWindowFrame(oldactive);
    }    

    
    if (window) RefreshWindowFrame(window);
}

