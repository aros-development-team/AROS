/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "camd_intern.h"
#include "endpoint_registry.h"
#include "provider_contract.h"

/*****************************************************************************

    NAME */

	AROS_LH2(LONG, GetPublishedUMP,

/*  SYNOPSIS */
	AROS_LHA(struct CAMDHandleV1 *, endpoint, A0),
	AROS_LHA(struct CAMDUMPEventV1 *, event, A1),

/*  LOCATION */
	struct CamdBase *, CamdBase, 69, Camd)

/*  FUNCTION
		Takes the oldest Universal MIDI Packet message a client sent
		to a published UMP endpoint. Clients are served in turn.

    INPUTS
		endpoint - from PublishEndpoint() with CAMD_DATA_FORMAT_UMP.
		event - filled in with the message.

    RESULT
		CAMD_RESULT_OK with a message, CAMD_RESULT_EMPTY when
		there is none, CAMD_RESULT_UNSUPPORTED for a MIDI 1.0
		endpoint, CAMD_RESULT_STALE for a withdrawn one, or another
		CAMD_RESULT_ value.

    NOTES
		New in V43 and provisional: the endpoint functions can still
		change. Does not wait.

    EXAMPLE

    BUGS

    SEE ALSO
		PutPublishedUMP(), GetPublishedMidi()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	if(endpoint==NULL || event==NULL) return camd_public_result(CAMD_REGISTRY_INVALID);

	return camd_public_result(GetAppEndpointUMP(endpoint,event,CamdBase));

    AROS_LIBFUNC_EXIT
}
