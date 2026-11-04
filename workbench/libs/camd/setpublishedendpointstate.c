/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "camd_intern.h"
#include "endpoint_registry.h"
#include "provider_contract.h"

/*****************************************************************************

    NAME */

	AROS_LH2(LONG, SetPublishedEndpointState,

/*  SYNOPSIS */
	AROS_LHA(struct CAMDHandleV1 *, endpoint, A0),
	AROS_LHA(ULONG, state, D0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 60, Camd)

/*  FUNCTION
		Tells whether a published endpoint can carry anything now.
		Offline it keeps its ID and its place in the snapshots, no
		session opens and open sessions report CAMD_REGISTRY_STATE.

    INPUTS
		endpoint - from PublishEndpoint().
		state - CAMD_ENDPOINT_AVAILABLE or CAMD_ENDPOINT_OFFLINE.

    RESULT
		CAMD_REGISTRY_OK, CAMD_REGISTRY_STALE for a withdrawn endpoint,
		or another CAMD_REGISTRY_ value.

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

	if(endpoint==NULL) return CAMD_REGISTRY_INVALID;

	return SetAppEndpointState(endpoint,state,CamdBase);

    AROS_LIBFUNC_EXIT
}
