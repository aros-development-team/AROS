/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "camd_intern.h"
#include "endpoint_registry.h"
#include "provider_contract.h"

/*****************************************************************************

    NAME */

	AROS_LH5(LONG, GetEndpointMidi,

/*  SYNOPSIS */
	AROS_LHA(struct CAMDHandleV1 *, session, A0),
	AROS_LHA(struct CAMDMIDI1EventV1 *, event, A1),
	AROS_LHA(UBYTE *, sysex, A2),
	AROS_LHA(ULONG, size, D0),
	AROS_LHA(ULONG *, length, A3),

/*  LOCATION */
	struct CamdBase *, CamdBase, 54, Camd)

/*  FUNCTION
		Takes the oldest message an input session received: either
		one short MIDI 1.0 message or one complete System Exclusive
		message.

    INPUTS
		session - from OpenEndpointSession().
		event - filled in with a short message.
		sysex - where a System Exclusive message is copied to.
		size - the room at sysex.
		length - filled in with 0 for a short message, or with the
		    number of bytes of a System Exclusive message.

    RESULT
		CAMD_REGISTRY_OK with a message.
		CAMD_REGISTRY_EMPTY when there is none.
		CAMD_REGISTRY_TOO_LARGE when the oldest message is System
		Exclusive and does not fit in size: length tells how much it
		needs and the message stays.
		CAMD_REGISTRY_QUEUE_FULL once after messages were lost because
		the session's queue was full; no message is taken.
		Another CAMD_REGISTRY_ value otherwise.

    NOTES
		New in V43 and provisional: the endpoint functions can still
		change. Does not wait. Read until CAMD_REGISTRY_EMPTY after
		each signal.

    EXAMPLE
		for(;;){
			result=GetEndpointMidi(&session,&event,buffer,sizeof(buffer),&length);
			if(result==CAMD_REGISTRY_EMPTY) break;
			if(result==CAMD_REGISTRY_QUEUE_FULL) note_loss();
			else if(result==CAMD_REGISTRY_OK && length==0) handle(&event);
			else if(result==CAMD_REGISTRY_OK) handle_sysex(buffer,length);
			else break;
		}

    BUGS

    SEE ALSO
		OpenEndpointSession()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	if(session==NULL || event==NULL || length==NULL){
		return CAMD_REGISTRY_INVALID;
	}
	*length=0;

	return ReadEndpointReceiver(session,event,sysex,size,length,CamdBase);

    AROS_LIBFUNC_EXIT
}
