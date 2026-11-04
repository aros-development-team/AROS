/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "camd_intern.h"
#include "endpoint_registry.h"
#include "provider_contract.h"

/*****************************************************************************

    NAME */

	AROS_LH1(LONG, CloseEndpointSession,

/*  SYNOPSIS */
	AROS_LHA(struct CAMDHandleV1 *, session, A0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 50, Camd)

/*  FUNCTION
		Closes a session. Messages not yet sent are dropped; call
		DrainEndpointSession() first to have them sent. Messages
		received and not yet read are dropped too.

    INPUTS
		session - from OpenEndpointSession().

    RESULT
		CAMD_RESULT_OK, CAMD_RESULT_STALE for a session already
		closed, or another CAMD_RESULT_ value.

    NOTES
		New in V43 and provisional: the endpoint functions can still
		change.

    EXAMPLE

    BUGS

    SEE ALSO
		OpenEndpointSession(), DrainEndpointSession()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	LONG result;

	if(session==NULL) return camd_public_result(CAMD_REGISTRY_INVALID);

	StopEndpointReceiver(session,CamdBase);
	result=camd_registry_session_close(CB(CamdBase)->EndpointRegistry,*session);
	if(result==CAMD_REGISTRY_OK){
		DisownEndpointObject(ENDPOINT_OWNED_SESSION,session,CamdBase);
	}

	return camd_public_result(result);

    AROS_LIBFUNC_EXIT
}
