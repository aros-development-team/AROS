/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <devices/timer.h>
#include <proto/exec.h>

#include "camd_intern.h"
#include "endpoint_registry.h"
#include "provider_contract.h"

/*****************************************************************************

    NAME */

	AROS_LH2(LONG, DrainEndpointSession,

/*  SYNOPSIS */
	AROS_LHA(struct CAMDHandleV1 *, session, A0),
	AROS_LHA(ULONG, milliseconds, D0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 53, Camd)

/*  FUNCTION
		Waits until everything queued on an output session has been
		handed over: to its port, or to the publisher of a published
		endpoint. Messages that wait for their time count as queued.

    INPUTS
		session - from OpenEndpointSession().
		milliseconds - how long to wait at most: 0 only looks, and
		    CAMD_WAIT_FOREVER has no limit.

    RESULT
		CAMD_REGISTRY_OK when nothing is queued any more,
		CAMD_REGISTRY_QUEUE_FULL when the time is up and something
		still is, or another CAMD_REGISTRY_ value.

    NOTES
		New in V43 and provisional: the endpoint functions can still
		change. It looks again every two milliseconds; the time is
		measured with CamdTime(). Without memory for its timer it
		returns CAMD_REGISTRY_QUEUE_FULL early.

    EXAMPLE

    BUGS

    SEE ALSO
		PutEndpointMidi(), CloseEndpointSession()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	struct timerequest *pause=NULL;
	ULONG start=CamdClock();
	LONG result;

	if(session==NULL) return CAMD_REGISTRY_INVALID;

	/* The session's worker or the publisher empties the queue;
	   QUEUE_FULL means something is still in it. */
	for(;;){
		result=camd_registry_session_drain(CB(CamdBase)->EndpointRegistry,
			*session);
		if(result!=CAMD_REGISTRY_QUEUE_FULL) break;
		if(milliseconds!=CAMD_WAIT_FOREVER &&
		   CamdClock()-start>=milliseconds) break;
		/* A real wait. CamdWait() returns at once when its helper
		   process has not run since the last call, which is always
		   so for a caller that holds Forbid(). */
		if(pause==NULL) pause=OpenEndpointPause();
		if(pause==NULL) break;
		EndpointPause(pause);
	}
	CloseEndpointPause(pause);

	return result;

    AROS_LIBFUNC_EXIT
}
