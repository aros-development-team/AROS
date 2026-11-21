/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "camd_intern.h"
#include "endpoint_registry.h"
#include "provider_contract.h"

/*****************************************************************************

    NAME */

	AROS_LH3(LONG, PutEndpointUMP,

/*  SYNOPSIS */
	AROS_LHA(struct CAMDHandleV1 *, session, A0),
	AROS_LHA(struct CAMDUMPEventV1 *, events, A1),
	AROS_LHA(ULONG, count, D0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 66, Camd)

/*  FUNCTION
		Queues complete Universal MIDI Packet messages on a UMP
		output session. Either all of them are queued, in order, or
		none.

    INPUTS
		session - from OpenEndpointSession() with CAMD_DATA_FORMAT_UMP.
		events - count records, each with Size, Version and WordCount
		    set.
		count - at least 1.

    RESULT
		CAMD_RESULT_OK, CAMD_RESULT_QUEUE_FULL, CAMD_RESULT_INVALID
		for a WordCount that does not fit the message type,
		CAMD_RESULT_UNSUPPORTED on a MIDI 1.0 session, or another
		CAMD_RESULT_ value.

    NOTES
		New in V43 and provisional: the endpoint functions can still
		change. Does not wait. CAMD carries the
		words as they are and converts nothing.

    EXAMPLE

    BUGS

    SEE ALSO
		GetEndpointUMP(), PutEndpointMidi()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	if(session==NULL) return camd_public_result(CAMD_REGISTRY_INVALID);

	return camd_public_result(camd_registry_session_send_ump(CB(CamdBase)->EndpointRegistry,
		*session,events,count));

    AROS_LIBFUNC_EXIT
}
