/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "camd_intern.h"
#include "endpoint_registry.h"
#include "provider_contract.h"

/*****************************************************************************

    NAME */

	AROS_LH5(LONG, GetPublishedMidi,

/*  SYNOPSIS */
	AROS_LHA(struct CAMDHandleV1 *, endpoint, A0),
	AROS_LHA(struct CAMDMIDI1EventV1 *, event, A1),
	AROS_LHA(UBYTE *, sysex, A2),
	AROS_LHA(ULONG, size, D0),
	AROS_LHA(ULONG *, length, A3),

/*  LOCATION */
	struct CamdBase *, CamdBase, 63, Camd)

/*  FUNCTION
		Takes the oldest message a client sent to a published
		endpoint: one short MIDI 1.0 message or one complete System
		Exclusive message. Clients are served in turn.

    INPUTS
		endpoint - from PublishEndpoint().
		event - filled in with a short message.
		sysex - where a System Exclusive message is copied to.
		size - the room at sysex.
		length - filled in with 0 for a short message, or with the
		    number of bytes of a System Exclusive message.

    RESULT
		CAMD_REGISTRY_OK with a message, CAMD_REGISTRY_EMPTY when
		there is none, CAMD_REGISTRY_TOO_LARGE when a System Exclusive
		message does not fit in size (length tells how much it needs
		and the message stays), CAMD_REGISTRY_STALE for a withdrawn
		endpoint, or another CAMD_REGISTRY_ value.

    NOTES
		New in V43 and provisional: the endpoint functions can still
		change. Does not wait. Read until
		CAMD_REGISTRY_EMPTY after each signal.

    EXAMPLE

    BUGS

    SEE ALSO
		PublishEndpoint(), PutPublishedMidi()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	if(endpoint==NULL || event==NULL || length==NULL){
		return CAMD_REGISTRY_INVALID;
	}
	*length=0;

	return GetAppEndpointMidi(endpoint,event,sysex,size,length,CamdBase);

    AROS_LIBFUNC_EXIT
}
