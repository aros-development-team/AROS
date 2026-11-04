/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "camd_intern.h"
#include "endpoint_registry.h"
#include "provider_contract.h"

/*****************************************************************************

    NAME */

	AROS_LH2(LONG, GetEndpointWatchEvent,

/*  SYNOPSIS */
	AROS_LHA(struct CAMDHandleV1 *, watch, A0),
	AROS_LHA(struct CAMDEndpointWatchEventV1 *, event, A1),

/*  LOCATION */
	struct CamdBase *, CamdBase, 56, Camd)

/*  FUNCTION
		Takes the oldest change from an endpoint watch.

    INPUTS
		watch - from StartEndpointWatch().
		event - filled in with the change.

    RESULT
		CAMD_REGISTRY_OK with a change, CAMD_REGISTRY_EMPTY when none
		is left, CAMD_REGISTRY_STALE for an ended watch, or another
		CAMD_REGISTRY_ value.

    NOTES
		New in V43 and provisional: the endpoint functions can still
		change. Read until CAMD_REGISTRY_EMPTY after
		each signal. CAMD_ENDPOINT_EVENT_LOST means that changes were
		dropped: take a new snapshot.

    EXAMPLE

    BUGS

    SEE ALSO
		StartEndpointWatch()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	if(watch==NULL || event==NULL) return CAMD_REGISTRY_INVALID;

	return ReadEndpointWatcher(watch,event,CamdBase);

    AROS_LIBFUNC_EXIT
}
