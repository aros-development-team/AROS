/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "camd_intern.h"
#include "endpoint_registry.h"
#include "provider_contract.h"

/*****************************************************************************

    NAME */

	AROS_LH3(LONG, PutPublishedUMP,

/*  SYNOPSIS */
	AROS_LHA(struct CAMDHandleV1 *, endpoint, A0),
	AROS_LHA(struct CAMDUMPEventV1 *, events, A1),
	AROS_LHA(ULONG, count, D0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 68, Camd)

/*  FUNCTION
		Gives complete Universal MIDI Packet messages to every client
		that receives from a published UMP endpoint.

    INPUTS
		endpoint - from PublishEndpoint() with CAMD_DATA_FORMAT_UMP.
		events - count records, each with Size, Version and WordCount
		    set.
		count - at least 1.

    RESULT
		CAMD_REGISTRY_OK, CAMD_REGISTRY_STALE for a withdrawn endpoint,
		CAMD_REGISTRY_INVALID also for a MIDI 1.0 endpoint, or another
		CAMD_REGISTRY_ value.

    NOTES
		New in V43 and provisional: the endpoint functions can still
		change. Does not wait.

    EXAMPLE

    BUGS

    SEE ALSO
		GetPublishedUMP(), PutPublishedMidi()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	if(endpoint==NULL) return CAMD_REGISTRY_INVALID;

	return PutAppEndpointUMP(endpoint,events,count,CamdBase);

    AROS_LIBFUNC_EXIT
}
