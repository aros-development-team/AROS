/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

    AROS_LH1(void, ClearMenuStrip,

/*  SYNOPSIS */
         AROS_LHA(struct Window *, window, A0),

/*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 9, Intuition)

/*  FUNCTION
	Detach menu stript from a window.
	Call this function before you change memu data.
	
    INPUTS
	window - the window from which the menu bar should be detached

    RESULT
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
	SetMenuStrip()
    
    INTERNALS
 
    HISTORY
    29-10-95    digulla automatically created from
                intuition_lib.fd and clib/intuition_protos.h
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    SANITY_CHECK(window)

    ObtainSemaphore(&GetPrivIBase(IntuitionBase)->MenuLock);
    window->MenuStrip = NULL;
    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->MenuLock);

    return;

    AROS_LIBFUNC_EXIT
} /* ClearMenuStrip */
