/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "camd_intern.h"
#include "endpoint_registry.h"
#include "provider_contract.h"

/*****************************************************************************

    NAME */

	AROS_LH1(LONG, CancelEndpointSession,

/*  SYNOPSIS */
	AROS_LHA(struct CAMDHandleV1 *, session, A0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 64, Camd)

/*  FUNCTION
		Drops what an output session has queued and not yet handed
		over, including messages waiting for their time.

    INPUTS
		session - from OpenEndpointSession().

    RESULT
		CAMD_REGISTRY_OK, or another CAMD_REGISTRY_ value.

    NOTES
		New in V43 and provisional: the endpoint functions can still
		change. A message already on its way to the
		port is not taken back.

    EXAMPLE

    BUGS

    SEE ALSO
		DrainEndpointSession(), CloseEndpointSession()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	if(session==NULL) return CAMD_REGISTRY_INVALID;

	return camd_registry_session_cancel(CB(CamdBase)->EndpointRegistry,*session);

    AROS_LIBFUNC_EXIT
}
