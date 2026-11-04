/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "camd_intern.h"
#include "endpoint_registry.h"
#include "provider_contract.h"

/*****************************************************************************

    NAME */

	AROS_LH3(LONG, PublishEndpoint,

/*  SYNOPSIS */
	AROS_LHA(struct CAMDPublishRequestV1 *, request, A0),
	AROS_LHA(struct CAMDHandleV1 *, endpoint, A1),
	AROS_LHA(struct CAMDEndpointIDV1 *, id, A2),

/*  LOCATION */
	struct CamdBase *, CamdBase, 58, Camd)

/*  FUNCTION
		Publishes an endpoint of the calling program: other programs
		see it in their snapshots and open sessions with it, as with a
		driver's port. The publisher reads what they send with
		GetPublishedMidi() and sends to them with PutPublishedMidi()
		and PutPublishedSysEx().

    INPUTS
		request - the endpoint's name, directions and signal.
		endpoint - filled in with the publisher's handle.
		id - NULL, or filled in with the endpoint's stable ID.

    RESULT
		CAMD_REGISTRY_OK, CAMD_REGISTRY_DUPLICATE while an endpoint of
		that name is published, or another CAMD_REGISTRY_ value.

    NOTES
		New in V43 and provisional: the endpoint functions can still
		change. The same name gets the same ID
		again, also after a reboot. Withdraw every endpoint before
		closing camd.library.

    EXAMPLE

    BUGS

    SEE ALSO
		WithdrawEndpoint(), SetPublishedEndpointState(), GetPublishedMidi()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	if(endpoint!=NULL){
		endpoint->slot=0;
		endpoint->generation=0;
	}
	if(request==NULL || endpoint==NULL) return CAMD_REGISTRY_INVALID;

	return PublishAppEndpoint(request,endpoint,id,CamdBase);

    AROS_LIBFUNC_EXIT
}
