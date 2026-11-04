/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "camd_intern.h"
#include "endpoint_registry.h"
#include "provider_contract.h"

/*****************************************************************************

    NAME */

	AROS_LH1(LONG, DrainEndpointSession,

/*  SYNOPSIS */
	AROS_LHA(struct CAMDHandleV1 *, session, A0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 53, Camd)

/*  FUNCTION
		Waits until everything queued on an output session has been
		handed to its port.

    INPUTS
		session - from OpenEndpointSession().

    RESULT
		CAMD_REGISTRY_OK, or another CAMD_REGISTRY_ value.

    NOTES
		New in V43 and provisional: the endpoint functions can still
		change. Waits as long as the port takes, like PutSysEx().

    EXAMPLE

    BUGS

    SEE ALSO
		PutEndpointMidi(), CloseEndpointSession()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	LONG result;

	if(session==NULL) return CAMD_REGISTRY_INVALID;

	/* The session's worker empties the queue; QUEUE_FULL means it still
	   has something to hand over. */
	for(;;){
		result=camd_registry_session_drain(CB(CamdBase)->EndpointRegistry,
			*session);
		if(result!=CAMD_REGISTRY_QUEUE_FULL) break;
		CamdWait();
	}

	return result;

    AROS_LIBFUNC_EXIT
}
