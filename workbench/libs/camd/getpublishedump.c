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
		CAMD_REGISTRY_OK with a message, CAMD_REGISTRY_EMPTY when
		there is none, CAMD_REGISTRY_UNSUPPORTED for a MIDI 1.0
		endpoint, CAMD_REGISTRY_STALE for a withdrawn one, or another
		CAMD_REGISTRY_ value.

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

	if(endpoint==NULL || event==NULL) return CAMD_REGISTRY_INVALID;

	return GetAppEndpointUMP(endpoint,event,CamdBase);

    AROS_LIBFUNC_EXIT
}
