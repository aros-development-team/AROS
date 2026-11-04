/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "camd_intern.h"
#include "endpoint_registry.h"
#include "provider_contract.h"

/*****************************************************************************

    NAME */

	AROS_LH4(LONG, PutEndpointSysEx,

/*  SYNOPSIS */
	AROS_LHA(struct CAMDHandleV1 *, session, A0),
	AROS_LHA(UBYTE *, bytes, A1),
	AROS_LHA(ULONG, length, D0),
	AROS_LHA(struct CAMDMIDI1EventV1 *, time, A2),

/*  LOCATION */
	struct CamdBase *, CamdBase, 52, Camd)

/*  FUNCTION
		Queues one complete System Exclusive message on an output
		session. The bytes are copied.

    INPUTS
		session - from OpenEndpointSession().
		bytes - the message, from its 0xF0 to its 0xF7.
		length - the number of bytes.
		time - NULL to send at once, or an event whose Flags,
		    TimeHigh, TimeLow and ClockDomain give the message its
		    time as for PutEndpointMidi(); its other fields are not
		    read.

    RESULT
		CAMD_REGISTRY_OK, CAMD_REGISTRY_TOO_LARGE past the session's
		MaxSysExBytes, CAMD_REGISTRY_QUEUE_FULL, or another
		CAMD_REGISTRY_ value.

    NOTES
		New in V43 and provisional: the endpoint functions can still
		change. Does not wait.

    EXAMPLE

    BUGS

    SEE ALSO
		PutEndpointMidi(), DrainEndpointSession()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	if(session==NULL) return CAMD_REGISTRY_INVALID;

	if(time==NULL){
		return camd_registry_session_send_midi1_sysex(
			CB(CamdBase)->EndpointRegistry,*session,bytes,length,0,0,0,0);
	}

	return camd_registry_session_send_midi1_sysex(
		CB(CamdBase)->EndpointRegistry,*session,bytes,length,
		time->TimeHigh,time->TimeLow,time->ClockDomain,time->Flags);

    AROS_LIBFUNC_EXIT
}
