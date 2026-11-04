/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "camd_intern.h"
#include "endpoint_registry.h"
#include "provider_contract.h"

/*****************************************************************************

    NAME */

	AROS_LH1(void, EndEndpointWatch,

/*  SYNOPSIS */
	AROS_LHA(struct CAMDHandleV1 *, watch, A0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 57, Camd)

/*  FUNCTION
		Ends a watch from StartEndpointWatch(). Its signal is not
		sent any more once this returns.

    INPUTS
		watch - the watch's handle. NULL and a handle already ended
		    are ignored.

    RESULT
		None.

    NOTES
		New in V43 and provisional: the endpoint functions can still
		change.

    EXAMPLE

    BUGS

    SEE ALSO
		StartEndpointWatch()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	if(watch!=NULL) EndEndpointWatcher(watch,CamdBase);

    AROS_LIBFUNC_EXIT
}
