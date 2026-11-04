/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "camd_intern.h"
#include "endpoint_registry.h"
#include "provider_contract.h"

/*****************************************************************************

    NAME */

	AROS_LH3(LONG, PutEndpointMidi,

/*  SYNOPSIS */
	AROS_LHA(struct CAMDHandleV1 *, session, A0),
	AROS_LHA(struct CAMDMIDI1EventV1 *, events, A1),
	AROS_LHA(ULONG, count, D0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 51, Camd)

/*  FUNCTION
		Queues complete MIDI 1.0 messages on an output session. Either
		all of them are queued, in order, or none.

    INPUTS
		session - from OpenEndpointSession().
		events - count records, each with Size and Version set.
		count - at least 1.

    RESULT
		CAMD_REGISTRY_OK, CAMD_REGISTRY_QUEUE_FULL when the session's
		queue has no room for all of them, or another CAMD_REGISTRY_
		value.

    NOTES
		New in V43 and provisional: the endpoint functions can still
		change. Does not wait. SysEx goes through
		PutEndpointSysEx().

    EXAMPLE

    BUGS

    SEE ALSO
		PutEndpointSysEx(), DrainEndpointSession()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	if(session==NULL) return CAMD_REGISTRY_INVALID;

	return camd_registry_session_send_midi1(CB(CamdBase)->EndpointRegistry,
		*session,events,count);

    AROS_LIBFUNC_EXIT
}
