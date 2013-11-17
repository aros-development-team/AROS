/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "requesters.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>
#include <exec/types.h>
#include <intuition/intuition.h>

        AROS_LH3(LONG, SysReqHandler,

/*  SYNOPSIS */
        AROS_LHA(struct Window  *, window, A0),
        AROS_LHA(ULONG   *,        IDCMPFlagsPtr, A1),
        AROS_LHA(BOOL            , WaitInput, D0),

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 100, Intuition)

/*  FUNCTION
	Handles a requester, which was opened with BuildSysRequest() or
	BuildEasyRequestArgs(). When this function is called all outstanding
	IDCMP requests are processed. If an IDCMP request that would close
	a normal EasyRequestArgs() is encountered, SysReqHandler() returns
	with a return code equally to the return code EasyRequestArgs()
	would have returned. You may call this function in synchronous or
	asynchronous mode, by setting the WaitInput parameter.

    INPUTS
	window - The window pointer returned by either BuildSysRequest() or
		 BuildEasyRequestArgs().
	IDCMPFlagsPtr - Pointer to a ULONG to store the IDCMP flag that was
			received by the window. This will be set if you
			provided additional IDCMP flags to BuildSysRequest() or
			BuildEasyRequest(). You may set this to NULL. You must
			initialize the pointed to ULONG every time you call
			SysReqHandler().
	WaitInput - Set this to TRUE, if you want this function to wait for
		    the next IDCMP request, if there is none at the moment
		    the function is called.

    RESULT
	-2, if the requester was not satisfied. Normally you want to call
	    this function at least until this function returns something
	    different than -2.
	-1, if one of the IDCMP flags of idcmpPTR was set.
	 0, if the rightmost button was clicked or an error occured.
	 n, if the n-th button from the left was clicked.

    NOTES

    EXAMPLE

    BUGS
	Gadget placing is still untidy.
	Does not support BuildSysRequest() requesters, yet.

    SEE ALSO
	BuildSysRequest(), BuildEasyRequestArgs()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return sysreqhandler_intern(window, IDCMPFlagsPtr, WaitInput, IntuitionBase);

    AROS_LIBFUNC_EXIT
}
