/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "camd_intern.h"
#include "endpoint_registry.h"
#include "provider_contract.h"

/*****************************************************************************

    NAME */

	AROS_LH3(LONG, PutPublishedSysEx,

/*  SYNOPSIS */
	AROS_LHA(struct CAMDHandleV1 *, endpoint, A0),
	AROS_LHA(UBYTE *, bytes, A1),
	AROS_LHA(ULONG, length, D0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 62, Camd)

/*  FUNCTION
		Gives one complete System Exclusive message to every client
		that receives from a published endpoint.

    INPUTS
		endpoint - from PublishEndpoint().
		bytes - the message, from its 0xF0 to its 0xF7.
		length - the number of bytes.

    RESULT
		CAMD_RESULT_OK, CAMD_RESULT_STALE for a withdrawn endpoint,
		or another CAMD_RESULT_ value.

    NOTES
		New in V43 and provisional: the endpoint functions can still
		change. Does not wait.

    EXAMPLE

    BUGS

    SEE ALSO
		PutPublishedMidi()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	if(endpoint==NULL) return camd_public_result(CAMD_REGISTRY_INVALID);

	return camd_public_result(PutAppEndpointSysEx(endpoint,bytes,length,CamdBase));

    AROS_LIBFUNC_EXIT
}
