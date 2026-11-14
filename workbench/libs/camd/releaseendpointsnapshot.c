/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "camd_intern.h"
#include "endpoint_registry.h"
#include "provider_contract.h"

/*****************************************************************************

    NAME */

	AROS_LH1(void, ReleaseEndpointSnapshot,

/*  SYNOPSIS */
	AROS_LHA(struct CAMDHandleV1 *, snapshot, A0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 47, Camd)

/*  FUNCTION
		Frees a snapshot from ObtainEndpointSnapshot().

    INPUTS
		snapshot - the snapshot's handle. NULL and a handle already
		    released are ignored.

    RESULT
		None.

    NOTES
		New in V43 and provisional: the endpoint functions can still
		change.

    EXAMPLE

    BUGS

    SEE ALSO
		ObtainEndpointSnapshot()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	if(snapshot!=NULL){
		DisownEndpointObject(ENDPOINT_OWNED_SNAPSHOT,snapshot,CamdBase);
		RemoveEndpointSnapshot(snapshot,CamdBase);
	}

    AROS_LIBFUNC_EXIT
}
