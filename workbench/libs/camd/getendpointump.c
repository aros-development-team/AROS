/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "camd_intern.h"
#include "endpoint_registry.h"
#include "provider_contract.h"

/*****************************************************************************

    NAME */

	AROS_LH2(LONG, GetEndpointUMP,

/*  SYNOPSIS */
	AROS_LHA(struct CAMDHandleV1 *, session, A0),
	AROS_LHA(struct CAMDUMPEventV1 *, event, A1),

/*  LOCATION */
	struct CamdBase *, CamdBase, 67, Camd)

/*  FUNCTION
		Takes the oldest Universal MIDI Packet message a UMP input
		session received.

    INPUTS
		session - from OpenEndpointSession() with CAMD_DATA_FORMAT_UMP.
		event - filled in with the message.

    RESULT
		CAMD_RESULT_OK with a message, CAMD_RESULT_EMPTY when
		there is none, CAMD_RESULT_QUEUE_FULL once after messages
		were lost, CAMD_RESULT_UNSUPPORTED on a MIDI 1.0 session, or
		another CAMD_RESULT_ value.

    NOTES
		New in V43 and provisional: the endpoint functions can still
		change. Does not wait.

    EXAMPLE

    BUGS

    SEE ALSO
		PutEndpointUMP(), GetEndpointMidi()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	if(session==NULL || event==NULL) return camd_public_result(CAMD_REGISTRY_INVALID);

	return camd_public_result(ReadEndpointReceiverUMP(session,event,CamdBase));

    AROS_LIBFUNC_EXIT
}
