/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "camd_intern.h"
#include "endpoint_registry.h"
#include "provider_contract.h"

/*****************************************************************************

    NAME */

	AROS_LH3(LONG, PutPublishedMidi,

/*  SYNOPSIS */
	AROS_LHA(struct CAMDHandleV1 *, endpoint, A0),
	AROS_LHA(struct CAMDMIDI1EventV1 *, events, A1),
	AROS_LHA(ULONG, count, D0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 61, Camd)

/*  FUNCTION
		Gives complete MIDI 1.0 messages to every client that receives
		from a published endpoint.

    INPUTS
		endpoint - from PublishEndpoint().
		events - count records, each with Size and Version set.
		count - at least 1.

    RESULT
		CAMD_RESULT_OK, CAMD_RESULT_STALE for a withdrawn endpoint,
		or another CAMD_RESULT_ value.

    NOTES
		New in V43 and provisional: the endpoint functions can still
		change. Does not wait. A client whose queue
		is full loses the messages and is told so.

    EXAMPLE

    BUGS

    SEE ALSO
		PutPublishedSysEx(), GetPublishedMidi()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	if(endpoint==NULL) return camd_public_result(CAMD_REGISTRY_INVALID);

	return camd_public_result(PutAppEndpointMidi(endpoint,events,count,CamdBase));

    AROS_LIBFUNC_EXIT
}
