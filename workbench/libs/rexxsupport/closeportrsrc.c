/*
    Copyright (C) 1995-2002, The AROS Development Team. All rights reserved.

    Desc:
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
    Removes and releases a RexxSupport-managed message port resource.
    Returns outstanding tracked or queued messages to their senders,
    then releases the port and its tracking structures.

    INPUTS
    rsrc - Pointer to a RexxRsrc embedded in a PortNode registered by
           RexxSupport. It must refer to a live managed resource.

    RESULT
    None.

    NOTES
    This cleanup entry point is used for resource disposal. It must not
    be passed an unrelated RexxRsrc or an arbitrary RexxMsg pointer.

    SEE ALSO
    CLOSEPORT

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    portcleanup(RexxSupportBase, (struct PortNode *)rsrc);
  
    ReturnVoid("ClosePortRsrc");
    AROS_LIBFUNC_EXIT
} /* ClosePortRsrc */
