/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "camd_intern.h"
#include "endpoint_registry.h"
#include "provider_contract.h"

/*****************************************************************************

    NAME */

	AROS_LH1(LONG, CloseEndpointSession,

/*  SYNOPSIS */
	AROS_LHA(struct CAMDHandleV1 *, session, A0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 50, Camd)

/*  FUNCTION
		Closes a session. Messages not yet sent are dropped; call
		DrainEndpointSession() first to have them sent.

    INPUTS
		session - from OpenEndpointSession().

    RESULT
		CAMD_REGISTRY_OK, CAMD_REGISTRY_STALE for a session already
		closed, or another CAMD_REGISTRY_ value.

    NOTES
		New in V43 and provisional: the endpoint functions can still
		change.

    EXAMPLE

    BUGS

    SEE ALSO
		OpenEndpointSession(), DrainEndpointSession()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	if(session==NULL) return CAMD_REGISTRY_INVALID;

	return camd_registry_session_close(CB(CamdBase)->EndpointRegistry,*session);

    AROS_LIBFUNC_EXIT
}
