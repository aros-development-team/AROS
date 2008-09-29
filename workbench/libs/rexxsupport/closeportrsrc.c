/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include <aros/debug.h>
#include "rexxsupport_intern.h"
#include "portnode.h"

/*****************************************************************************

    NAME */

	AROS_LH1(VOID, ClosePortRsrc,

/*  SYNOPSIS */
	AROS_LHA(struct RexxRsrc *, rsrc, A0),

/*  LOCATION */
	struct Library *, RexxSupportBase, 6, RexxSupport)

/*  FUNCTION
         Deletes a RexxMsg structure

    INPUTS
         packet - The RexxMsg to delete.

    RESULT
         void

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
         rexxsyslib.library/CreateRexxMsg()

    INTERNALS


*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    portcleanup(RexxSupportBase, (struct PortNode *)rsrc);
  
    ReturnVoid("ClosePortRsrc");
    AROS_LIBFUNC_EXIT
} /* DeleteRexxMsg */
