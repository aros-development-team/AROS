/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "camd_intern.h"
#include "endpoint_registry.h"

/*****************************************************************************

    NAME */

	AROS_LH1(LONG, ForgetEndpointIdentity,

/*  SYNOPSIS */
	AROS_LHA(struct CAMDEndpointIDV1 *, id, A0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 76, Camd)

/*  FUNCTION
		Removes a stored endpoint ID. What it was stored for, a
		published name or a driver's port, gets a new ID the next time
		it appears. Saved routes that name the old ID no longer find
		anything.

    INPUTS
		id - the ID to forget.

    RESULT
		CAMD_RESULT_OK; CAMD_RESULT_BUSY while an endpoint or what
		offers it has the ID; CAMD_RESULT_STALE when no such ID is
		stored; CAMD_RESULT_WRONG_STATE when the store is unreadable;
		CAMD_RESULT_FAILED when the store could not be written, in
		which case the ID stays.

    NOTES
		New in V43 and provisional: the endpoint functions can still
		change.
		An endpoint and what offers it, a driver or a publisher, have
		an ID each: forget both to start afresh. A loaded driver's IDs
		stay in use until the library is expunged.

    EXAMPLE

    BUGS

    SEE ALSO
		GetStoredEndpointIdentity()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	if(id==NULL) return camd_public_result(CAMD_REGISTRY_INVALID);

	return camd_public_result(ForgetStoredIdentity(id,CamdBase));

    AROS_LIBFUNC_EXIT
}
