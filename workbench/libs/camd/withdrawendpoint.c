/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "camd_intern.h"
#include "endpoint_registry.h"
#include "provider_contract.h"

/*****************************************************************************

    NAME */

	AROS_LH1(void, WithdrawEndpoint,

/*  SYNOPSIS */
	AROS_LHA(struct CAMDHandleV1 *, endpoint, A0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 59, Camd)

/*  FUNCTION
		Takes a published endpoint away. Its clients' sessions stop
		carrying anything and report CAMD_REGISTRY_RETIRED; they still
		have to be closed. The publisher's signal is not sent any more
		once this returns.

    INPUTS
		endpoint - from PublishEndpoint(). NULL and a handle already
		    withdrawn are ignored.

    RESULT
		None.

    NOTES
		New in V43 and provisional: the endpoint functions can still
		change.

    EXAMPLE

    BUGS

    SEE ALSO
		PublishEndpoint()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	if(endpoint!=NULL) WithdrawAppEndpoint(endpoint,CamdBase);

    AROS_LIBFUNC_EXIT
}
