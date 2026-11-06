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
		CAMD_RESULT_OK when nothing is queued any more,
		CAMD_RESULT_QUEUE_FULL when the time is up and something
		still is, or another CAMD_RESULT_ value.

    NOTES
		New in V43 and provisional: the endpoint functions can still
		change. It is woken when the queue has been emptied; the time
		is measured with CamdTime(). Without memory for its timer it
		returns CAMD_RESULT_QUEUE_FULL early.

    EXAMPLE

    BUGS

    SEE ALSO
		PutEndpointMidi(), CloseEndpointSession()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	struct timerequest *pause;
	ULONG start=CamdClock(),elapsed,wait,signals=0;
	LONG result,slot=-1;
	BYTE bit;

	if(session==NULL) return camd_public_result(CAMD_REGISTRY_INVALID);

	result=camd_registry_session_drain(CB(CamdBase)->EndpointRegistry,*session);
	if(result!=CAMD_REGISTRY_QUEUE_FULL || milliseconds==0) return camd_public_result(result);

	/* The session's worker, or the publisher's read, signals this task
	   when a queue has been emptied; the timer is for the limit. Without
	   a signal or a place among the waiting tasks it looks every two
	   milliseconds instead. Every wait really blocks, so the worker runs
	   also when the caller holds Forbid(). */
	pause=OpenEndpointPause();
	if(pause==NULL) return camd_public_result(result);
	bit=AllocSignal(-1);
	if(bit>=0){
		signals=1UL<<bit;
		SetSignal(0,signals);
		slot=AddEndpointDrainer(signals,CamdBase);
		if(slot<0) signals=0;
	}

	for(;;){
		result=camd_registry_session_drain(CB(CamdBase)->EndpointRegistry,
			*session);
		if(result!=CAMD_REGISTRY_QUEUE_FULL) break;
		elapsed=CamdClock()-start;
		if(milliseconds!=CAMD_WAIT_FOREVER && elapsed>=milliseconds) break;
		wait=signals!=0 ? 1000 : 2;
		if(milliseconds!=CAMD_WAIT_FOREVER && milliseconds-elapsed<wait){
			wait=milliseconds-elapsed;
		}
		if(wait==0) wait=1;
		EndpointPause(pause,wait,signals);
		SetSignal(0,signals);
	}

	RemoveEndpointDrainer(slot,CamdBase);
	if(bit>=0) FreeSignal(bit);
	CloseEndpointPause(pause);

	return camd_public_result(result);

    AROS_LIBFUNC_EXIT
}
