/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
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
 
    IDCMP   flags   Action
      0   0 Nothing happens
      0  !=0    The flags are copied in the IDCMPFlags of the window
            and a MessagePort is created and stored in the
            UserPort of the window.
     !=0      0 The IDCMPFlags are cleared and the MessagePort in the
            UserPort is deleted.
     !=0     !=0    The flags are copied to the IDCMPFlags of the
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
    the Intuition message port half of it.  The converse is true
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

    Intuition v50 features WA_UserPort tag, which allows to set
    the UserPort at OpenWindow stage. Please note that using this tag
    changes the behaviour of ModifyIDCMP() slightly. Creating/disposing
    message ports is now up to the app. ModifyIDCMP(win,0) still clears
    win->UserPort pointer, but the message port is NOT disposed - you
    need to store it and dispose yourself! Also calling
    ModifyIDCMP(win,someidcmp) on a window with NULL win->UserPort will
    NOT create a new port!
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    OpenWindow(), CloseWindow(), intuition_extend.h
 
    INTERNALS
 
    HISTORY
    29-10-95    digulla automatically created from
                intuition_lib.fd and clib/intuition_protos.h
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    DEBUG_MODIFYIDCMP(dprintf("ModifyIDCMP: Window 0x%lx IDCMP 0x%lx Old 0x%lx\n",
                              window, flags, window->IDCMPFlags));

    SANITY_CHECKR(window,FALSE)

#ifdef SKINS
    if (((struct IntWindow *)window)->specialflags & SPFLAG_USERPORT)
    {
        window->IDCMPFlags = flags;
        if (!flags)
        {
            Forbid();
            window->UserPort = NULL;
            Permit();
        }
        return TRUE;
    }
#endif

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
